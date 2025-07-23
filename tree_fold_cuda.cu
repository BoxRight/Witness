#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <vector>
#include <algorithm>
#include <set>
#include <cmath>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/sort.h>
#include <thrust/unique.h>
#include <thrust/execution_policy.h>
#include <thrust/copy.h>
#include <thrust/transform.h>
#include <thrust/functional.h>
#include <unordered_map>
#include <fstream>
#include <string>
#include <map>
#include <cctype>
#include <sstream>
#include <cassert>
#include <cstdint>
#include <execution>
#include <functional>
#include <malloc.h>
#include <zstd.h>
#include "json.hpp"

using json = nlohmann::json;

// Error checking macro
#define CHECK_CUDA_ERROR(call) { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        fprintf(stderr, "CUDA error in %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
        exit(EXIT_FAILURE); \
    } \
}

// Configuration parameters
#define MAX_ELEMENTS_PER_VECTOR 128
#define BLOCK_SIZE 256
#define TILE_SIZE_A 256  // Tile size for set A in tiled processing
#define TILE_SIZE_B 3072 // Tile size for set B in tiled processing
#define RESULTS_FLUSH_THRESHOLD 10000 // In-memory result limit before flushing to disk
#define CHUNK_SIZE (1024 * 4) // Number of items to load from a stream at a time

// --- Forward Declarations ---
typedef struct {
    int8_t* data;         // Flattened array of all elements
    int* offsets;      // Starting index for each vector/set
    int* sizes;        // Size of each vector/set
    int numItems;      // Number of vectors/sets
    int totalElements; // Total number of elements
    int8_t* deviceBuffer; // Reusable device buffer for operations
    int bufferSize;    // Size of the device buffer
} CudaSet;
struct LevelItem;
struct ProcessResult;
ProcessResult processPair_inMemory(const CudaSet& setA, const CudaSet& setB, int threshold, int level, bool verbose);
void processLargePair(const CudaSet& setA, const CudaSet& setB, int threshold, int level, bool verbose, 
                     std::unordered_map<size_t, std::vector<int>>& uniqueResults,
                     std::function<void()> flushCallback = nullptr);
ProcessResult processPair(const CudaSet& setA, const CudaSet& setB, int threshold, int level, bool verbose, bool allowStreaming);
LevelItem processStreamedPair(LevelItem& itemA, LevelItem& itemB, int threshold, int level, bool verbose);

// Absolute value functor for Thrust
struct AbsoluteFunctor {
    __host__ __device__
    int operator()(const int x) const {
        return x < 0 ? -x : x;
    }
};

// Function to read Witness JSON and generate test sets
std::vector<std::vector<std::vector<int>>> generateWitnessSetsFromJSON(const std::string& filename) {
    // Read JSON file
    std::ifstream file(filename);
    if (!file.is_open()) {
        printf("Error: Could not open file: %s\n", filename.c_str());
        return {};
    }
    
    // Parse JSON
    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        printf("Error parsing JSON: %s\n", e.what());
        return {};
    }
    
    // Validate JSON structure
    if (!j.contains("clauses") || !j["clauses"].is_array()) {
        printf("Error: JSON must contain 'clauses' array\n");
        return {};
    }
    
    std::vector<std::vector<std::vector<int>>> testSets;
    
    // Process each clause
    for (const auto& clause : j["clauses"]) {
        if (!clause.contains("name") || !clause.contains("assignments")) {
            printf("Warning: Skipping clause without 'name' or 'assignments'\n");
            continue;
        }
        
        std::string clauseName = clause["name"];
        const auto& assignments = clause["assignments"];
        
        if (!assignments.is_array()) {
            printf("Warning: Skipping clause '%s' - 'assignments' is not an array\n", clauseName.c_str());
            continue;
        }
        
        std::vector<std::vector<int>> clauseSet;
        
        // Process each assignment
        for (const auto& assignment : assignments) {
            if (!assignment.is_array()) {
                printf("Warning: Skipping non-array assignment in clause '%s'\n", clauseName.c_str());
                continue;
            }
            
            std::vector<int> assignmentVec;
            for (const auto& value : assignment) {
                if (value.is_number()) {
                    assignmentVec.push_back(value.get<int>());
                } else {
                    printf("Warning: Skipping non-numeric value in assignment\n");
                }
            }
            
            if (!assignmentVec.empty()) {
                clauseSet.push_back(assignmentVec);
            }
        }
        
        printf("  Clause '%s': %zu assignments\n", clauseName.c_str(), clauseSet.size());
        testSets.push_back(clauseSet);
    }
    
    printf("\nGenerated %zu test sets from Witness JSON\n\n", testSets.size());
    
    // Print verification of the testSets
    printf("=== Verification of Generated Test Sets ===\n");
    for (size_t i = 0; i < testSets.size(); i++) {
        printf("Test Set %zu:\n", i);
        printf("{\n");
        for (size_t j = 0; j < testSets[i].size(); j++) {
            printf("  {");
            for (size_t k = 0; k < testSets[i][j].size(); k++) {
                printf("%d", testSets[i][j][k]);
                if (k < testSets[i][j].size() - 1) {
                    printf(",");
                }
            }
            printf("}");
            if (j < testSets[i].size() - 1) {
                printf(",");
            }
            printf("\n");
        }
        printf("}\n\n");
    }
    
    return testSets;
}

//-------------------------------------------------------------------------
// Host-side data structures
//-------------------------------------------------------------------------
typedef struct {
    std::vector<std::vector<int>> vectors;  // Original vectors 
} HostSet;

// Result buffer for parallel combination processing
typedef struct {
    int* data;         // Buffer for all potential results
    int* validFlags;   // Flags indicating if each combination is valid
    int* sizes;        // Size of each result set
    int maxResultSize; // Maximum possible size of a result
    int numCombinations; // Total number of combinations
} CombinationResultBuffer;

// Result struct to handle in-memory or streamed results
struct ProcessResult {
    CudaSet set;
    std::string streamPath; // Path to file if results are streamed
    int fromIdA = -1;
    int fromIdB = -1;
    size_t numResultItems = 0;
};

// Allocate memory for a CUDA set with additional buffer space
CudaSet allocateCudaSet(int numItems, int totalElements, int bufferSize = 0) {
    CudaSet set;
    set.numItems = numItems;
    set.totalElements = totalElements;
    
    CHECK_CUDA_ERROR(cudaMalloc(&set.data, totalElements * sizeof(int8_t)));
    CHECK_CUDA_ERROR(cudaMalloc(&set.offsets, numItems * sizeof(int)));
    CHECK_CUDA_ERROR(cudaMalloc(&set.sizes, numItems * sizeof(int)));
    
    // Allocate device buffer if size is specified
    if (bufferSize > 0) {
        CHECK_CUDA_ERROR(cudaMalloc(&set.deviceBuffer, bufferSize * sizeof(int8_t)));
        set.bufferSize = bufferSize;
    } else {
        set.deviceBuffer = nullptr;
        set.bufferSize = 0;
    }
    
    return set;
}

// Free memory for a CUDA set
void freeCudaSet(CudaSet* set) {
    if (set->data) cudaFree(set->data);
    if (set->offsets) cudaFree(set->offsets);
    if (set->sizes) cudaFree(set->sizes);
    if (set->deviceBuffer) cudaFree(set->deviceBuffer);
    set->numItems = 0;
    set->totalElements = 0;
    set->bufferSize = 0;
    set->data = nullptr;
    set->offsets = nullptr;
    set->sizes = nullptr;
    set->deviceBuffer = nullptr;
}

// Allocate result buffer for parallel combination processing
CombinationResultBuffer allocateCombinationResultBuffer(int numItemsA, int numItemsB, int maxElementsPerVector) {
    CombinationResultBuffer buffer;
    buffer.numCombinations = numItemsA * numItemsB;
    buffer.maxResultSize = 2 * maxElementsPerVector; // Worst case: all elements from both vectors
    
    CHECK_CUDA_ERROR(cudaMalloc(&buffer.data, buffer.numCombinations * buffer.maxResultSize * sizeof(int)));
    CHECK_CUDA_ERROR(cudaMalloc(&buffer.validFlags, buffer.numCombinations * sizeof(int)));
    CHECK_CUDA_ERROR(cudaMalloc(&buffer.sizes, buffer.numCombinations * sizeof(int)));
    
    // Initialize all valid flags to 0 (invalid)
    CHECK_CUDA_ERROR(cudaMemset(buffer.validFlags, 0, buffer.numCombinations * sizeof(int)));
    
    return buffer;
}

// Free result buffer
void freeCombinationResultBuffer(CombinationResultBuffer* buffer) {
    if (buffer->data) cudaFree(buffer->data);
    if (buffer->validFlags) cudaFree(buffer->validFlags);
    if (buffer->sizes) cudaFree(buffer->sizes);
    buffer->data = nullptr;
    buffer->validFlags = nullptr;
    buffer->sizes = nullptr;
}

// Host to device copy for a set (optimized to use pinned memory for larger transfers)
void copyHostToDevice(const HostSet& hostSet, CudaSet* cudaSet) {
    int numItems = hostSet.vectors.size();
    
    // Prepare host side arrays
    std::vector<int> hostIntData;
    std::vector<int> hostOffsets(numItems);
    std::vector<int> hostSizes(numItems);
    
    int currentOffset = 0;
    for (int i = 0; i < numItems; i++) {
        hostOffsets[i] = currentOffset;
        hostSizes[i] = hostSet.vectors[i].size();
        
        for (int j = 0; j < hostSet.vectors[i].size(); j++) {
            hostIntData.push_back(hostSet.vectors[i][j]);
        }
        
        currentOffset += hostSet.vectors[i].size();
    }
    
    // Convert to int8_t for device storage
    std::vector<int8_t> hostData(hostIntData.size());
    for (size_t i = 0; i < hostIntData.size(); ++i) {
        assert(hostIntData[i] >= INT8_MIN && hostIntData[i] <= INT8_MAX && "Input data exceeds int8_t range!");
        hostData[i] = static_cast<int8_t>(hostIntData[i]);
    }

    // Use pinned memory for large transfers
    int totalElements = hostData.size();
    int8_t* pinnedData = nullptr;
    int* pinnedOffsets = nullptr;
    int* pinnedSizes = nullptr;
    
    if (totalElements > 1024) {
        CHECK_CUDA_ERROR(cudaMallocHost((void**)&pinnedData, totalElements * sizeof(int8_t)));
        CHECK_CUDA_ERROR(cudaMallocHost(&pinnedOffsets, numItems * sizeof(int)));
        CHECK_CUDA_ERROR(cudaMallocHost(&pinnedSizes, numItems * sizeof(int)));
        
        memcpy(pinnedData, hostData.data(), totalElements * sizeof(int8_t));
        memcpy(pinnedOffsets, hostOffsets.data(), numItems * sizeof(int));
        memcpy(pinnedSizes, hostSizes.data(), numItems * sizeof(int));
    }
    
    // Allocate device memory
    *cudaSet = allocateCudaSet(numItems, totalElements, totalElements * 2);
    
    // Copy data to device
    if (totalElements > 1024) {
        CHECK_CUDA_ERROR(cudaMemcpy(cudaSet->data, pinnedData, totalElements * sizeof(int8_t), cudaMemcpyHostToDevice));
        CHECK_CUDA_ERROR(cudaMemcpy(cudaSet->offsets, pinnedOffsets, numItems * sizeof(int), cudaMemcpyHostToDevice));
        CHECK_CUDA_ERROR(cudaMemcpy(cudaSet->sizes, pinnedSizes, numItems * sizeof(int), cudaMemcpyHostToDevice));
        
        cudaFreeHost(pinnedData);
        cudaFreeHost(pinnedOffsets);
        cudaFreeHost(pinnedSizes);
    } else {
        CHECK_CUDA_ERROR(cudaMemcpy(cudaSet->data, hostData.data(), totalElements * sizeof(int8_t), cudaMemcpyHostToDevice));
        CHECK_CUDA_ERROR(cudaMemcpy(cudaSet->offsets, hostOffsets.data(), numItems * sizeof(int), cudaMemcpyHostToDevice));
        CHECK_CUDA_ERROR(cudaMemcpy(cudaSet->sizes, hostSizes.data(), numItems * sizeof(int), cudaMemcpyHostToDevice));
    }
}

// Device to host copy (optimized with streams for larger data)
HostSet copyDeviceToHost(const CudaSet& cudaSet) {
    HostSet hostSet;
    
    // Copy offsets and sizes
    std::vector<int> hostOffsets(cudaSet.numItems);
    std::vector<int> hostSizes(cudaSet.numItems);
    
    CHECK_CUDA_ERROR(cudaMemcpy(hostOffsets.data(), cudaSet.offsets, cudaSet.numItems * sizeof(int), cudaMemcpyDeviceToHost));
    CHECK_CUDA_ERROR(cudaMemcpy(hostSizes.data(), cudaSet.sizes, cudaSet.numItems * sizeof(int), cudaMemcpyDeviceToHost));
    
    // For large data, use async transfers with streams
    std::vector<int8_t> hostData8(cudaSet.totalElements);
    
    if (cudaSet.totalElements > 1024) {
        cudaStream_t stream;
        CHECK_CUDA_ERROR(cudaStreamCreate(&stream));
        
        int8_t* pinnedData;
        CHECK_CUDA_ERROR(cudaMallocHost((void**)&pinnedData, cudaSet.totalElements * sizeof(int8_t)));
        
        CHECK_CUDA_ERROR(cudaMemcpyAsync(pinnedData, cudaSet.data, cudaSet.totalElements * sizeof(int8_t), 
                                       cudaMemcpyDeviceToHost, stream));
        CHECK_CUDA_ERROR(cudaStreamSynchronize(stream));
        
        memcpy(hostData8.data(), pinnedData, cudaSet.totalElements * sizeof(int8_t));
        
        cudaFreeHost(pinnedData);
        cudaStreamDestroy(stream);
    } else {
        CHECK_CUDA_ERROR(cudaMemcpy(hostData8.data(), cudaSet.data, cudaSet.totalElements * sizeof(int8_t), 
                                  cudaMemcpyDeviceToHost));
    }
    
    // Reconstruct vectors
    hostSet.vectors.resize(cudaSet.numItems);
    
    // Convert back to int
    std::vector<int> hostData(cudaSet.totalElements);
    for (size_t i = 0; i < hostData8.size(); ++i) {
        hostData[i] = hostData8[i];
    }

    for (int i = 0; i < cudaSet.numItems; i++) {
        int offset = hostOffsets[i];
        int size = hostSizes[i];
        
        hostSet.vectors[i].resize(size);
        for (int j = 0; j < size; j++) {
            hostSet.vectors[i][j] = hostData[offset + j];
        }
    }
    
    return hostSet;
}

// Helper function to create a test set
HostSet createTestSet(const std::vector<std::vector<int>>& vectors) {
    HostSet set;
    set.vectors = vectors;
    return set;
}

//-------------------------------------------------------------------------
// CUDA Kernels and Device Functions
//-------------------------------------------------------------------------

// Device function to check if an element is in a set
__device__ bool deviceContains(const int* array, int size, int value) {
    for (int i = 0; i < size; i++) {
        if (array[i] == value) {
            return true;
        }
    }
    return false;
}

// Kernel to convert vector elements to unique elements (for Level 1 carry-over)
__global__ void convertToUniqueKernel(
    int8_t* inputData, int* inputOffsets, int* inputSizes, int numItems,
    int8_t* outputData, int* outputOffsets, int* outputSizes, int maxOutputSize
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx >= numItems) {
        return;
    }
    
    int inputOffset = inputOffsets[idx];
    int inputSize = inputSizes[idx];
    int outputOffset = outputOffsets[idx];
    
    // Local working memory for unique elements
    int localSet[MAX_ELEMENTS_PER_VECTOR];
    int localSetSize = 0;
    
    // Get unique elements
    for (int i = 0; i < inputSize; i++) {
        int val = inputData[inputOffset + i];
        if (!deviceContains(localSet, localSetSize, val)) {
            localSet[localSetSize++] = val;
        }
    }
    
    // Copy result to output
    outputSizes[idx] = localSetSize;
    for (int i = 0; i < localSetSize; i++) {
        outputData[outputOffset + i] = localSet[i];
    }
}

// Kernel that processes all combinations with built-in batching
__global__ void processAllCombinationsKernel(
    int8_t* dataA, int* offsetsA, int* sizesA, int numItemsA,
    int8_t* dataB, int* offsetsB, int* sizesB, int numItemsB,
    int threshold, int level,
    int* resultData, int* resultSizes, int* validFlags, int maxResultSize,
    int combinationsPerThread
) {
    // Calculate global thread ID
    int threadId = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Each thread processes multiple combinations using grid-stride loop
    for (int i = 0; i < combinationsPerThread; i++) {
        // Calculate combination index for this thread and iteration
        int combinationIdx = threadId * combinationsPerThread + i;
        
        // Check if this combination index is valid
        if (combinationIdx >= numItemsA * numItemsB) {
            return;
        }
        
        // Calculate setA and setB indices from the combination index
        int idxA = combinationIdx / numItemsB;
        int idxB = combinationIdx % numItemsB;
        
        // Get vectors from set A and set B
        int offsetA = offsetsA[idxA];
        int sizeA = sizesA[idxA];
        int offsetB = offsetsB[idxB];
        int sizeB = sizesB[idxB];
        
        // Local working memory for unique elements
        int localSet[MAX_ELEMENTS_PER_VECTOR * 2];
        int localSetSize = 0;
        
        // Merge vectors, keeping only unique elements
        for (int j = 0; j < sizeA; j++) {
            int val = dataA[offsetA + j];
            if (!deviceContains(localSet, localSetSize, val)) {
                localSet[localSetSize++] = val;
            }
        }
        
        for (int j = 0; j < sizeB; j++) {
            int val = dataB[offsetB + j];
            if (!deviceContains(localSet, localSetSize, val)) {
                localSet[localSetSize++] = val;
            }
        }
        
        // Check threshold condition
        bool isValid = (threshold == 0 || localSetSize <= threshold);
        
        // If valid, copy result to output buffer
        if (isValid) {
            validFlags[combinationIdx] = 1;
            resultSizes[combinationIdx] = localSetSize;
            
            int resultOffset = combinationIdx * maxResultSize;
            for (int j = 0; j < localSetSize; j++) {
                resultData[resultOffset + j] = localSet[j];
            }
        } else {
            validFlags[combinationIdx] = 0;
            resultSizes[combinationIdx] = localSetSize; // Store size for debugging
        }
    }
}

//-------------------------------------------------------------------------
// Core processing functions
//-------------------------------------------------------------------------

// Represents an item in a processing level of the tree fold
struct LevelItem {
    CudaSet set;
    std::string streamPath;
    int numItems;
    int id;
    bool needsCleanup; // True if this is an intermediate result that should be freed/deleted

    bool isStreamed() const { return !streamPath.empty(); }
};

// Global counter for unique item IDs
static int levelItemCounter = 0;

// Helper to get the first vector from a CudaSet for threshold calculation
std::vector<int> getFirstVectorFromCudaSet(const CudaSet& set) {
    if (set.numItems == 0) return {};
    int size;
    CHECK_CUDA_ERROR(cudaMemcpy(&size, set.sizes, sizeof(int), cudaMemcpyDeviceToHost));
    
    std::vector<int8_t> h_firstVector8(size);
    int offset = 0; // First vector is always at offset 0
    CHECK_CUDA_ERROR(cudaMemcpy(h_firstVector8.data(), set.data + offset, size * sizeof(int8_t), cudaMemcpyDeviceToHost));
    
    std::vector<int> firstVector(size);
    for(int i = 0; i < size; ++i) firstVector[i] = h_firstVector8[i];
    return firstVector;
}

// Helper to get the first vector from a streamed file
std::vector<int> getFirstVectorFromStream(const std::string& filePath) {
    FILE* inFile = fopen(filePath.c_str(), "rb");
    if (!inFile) return {};

    int vecSize = 0;
    size_t elementsRead = fread(&vecSize, sizeof(int), 1, inFile);
    if (elementsRead == 0) {
        fclose(inFile);
        return {};
    }

    std::vector<int> firstVec(vecSize);
    fread(firstVec.data(), sizeof(int), vecSize, inFile);
    fclose(inFile);
    return firstVec;
}

// Modified threshold computation to handle streamed and in-memory sets
int computeThreshold(const LevelItem& itemA, const LevelItem& itemB) {
    if (itemA.numItems == 0 || itemB.numItems == 0) return 0;

    // Get the first vector from item A
    std::vector<int> firstVectorA = itemA.isStreamed() ? 
        getFirstVectorFromStream(itemA.streamPath) : 
        getFirstVectorFromCudaSet(itemA.set);

    // Get the first vector from item B
    std::vector<int> firstVectorB = itemB.isStreamed() ? 
        getFirstVectorFromStream(itemB.streamPath) : 
        getFirstVectorFromCudaSet(itemB.set);

    if (firstVectorA.empty() || firstVectorB.empty()) return 0;
    
    // The rest of the logic is the same: find unique absolute values
    std::set<int> uniqueAbsValues;
    for (int value : firstVectorA) uniqueAbsValues.insert(abs(value));
    for (int value : firstVectorB) uniqueAbsValues.insert(abs(value));
        
    return uniqueAbsValues.size();
}

// Helper function to extract a subset from a CudaSet
CudaSet extractSubset(const CudaSet& set, int startIndex, int count, bool verbose) {
    if (count <= 0) {
        // Return empty set
        CudaSet emptySet;
        emptySet.numItems = 0;
        emptySet.totalElements = 0;
        emptySet.data = nullptr;
        emptySet.offsets = nullptr;
        emptySet.sizes = nullptr;
        emptySet.deviceBuffer = nullptr;
        emptySet.bufferSize = 0;
        return emptySet;
    }
    
    // Copy size and offset information for the slice
    std::vector<int> hostSizes(count);
    std::vector<int> hostOffsets(count);
    
    CHECK_CUDA_ERROR(cudaMemcpy(hostSizes.data(), set.sizes + startIndex, 
                              count * sizeof(int), cudaMemcpyDeviceToHost));
    CHECK_CUDA_ERROR(cudaMemcpy(hostOffsets.data(), set.offsets + startIndex, 
                              count * sizeof(int), cudaMemcpyDeviceToHost));
    
    // Calculate total elements in the subset
    int totalElements = 0;
    for (int i = 0; i < count; i++) {
        totalElements += hostSizes[i];
    }
    
    // Allocate memory for the subset
    CudaSet subSet = allocateCudaSet(count, totalElements);
    
    // Copy offset and size information
    std::vector<int> newOffsets(count);
    int currentOffset = 0;
    for (int i = 0; i < count; i++) {
        newOffsets[i] = currentOffset;
        currentOffset += hostSizes[i];
    }
    
    CHECK_CUDA_ERROR(cudaMemcpy(subSet.sizes, hostSizes.data(), 
                              count * sizeof(int), cudaMemcpyHostToDevice));
    CHECK_CUDA_ERROR(cudaMemcpy(subSet.offsets, newOffsets.data(), 
                              count * sizeof(int), cudaMemcpyHostToDevice));
    
    // Copy data elements for each vector
    for (int i = 0; i < count; i++) {
        int srcOffset = hostOffsets[i];
        int dstOffset = newOffsets[i];
        int size = hostSizes[i];
        
        CHECK_CUDA_ERROR(cudaMemcpy(subSet.data + dstOffset, set.data + srcOffset, 
                                  size * sizeof(int8_t), cudaMemcpyDeviceToDevice));
    }
    
    return subSet;
}

// An internal version of processPair that is guaranteed to run on the GPU without triggering another streaming operation.
// It also contains the full deduplication logic.
ProcessResult processPair_inMemory(const CudaSet& setA, const CudaSet& setB, int threshold, int level, bool verbose) {
    long long totalCombinations = (long long)setA.numItems * (long long)setB.numItems;
    
    // Calculate buffer size needed
    int maxResultsPerThread = 4;
    int threadsNeeded = (totalCombinations + maxResultsPerThread - 1) / maxResultsPerThread;
    
    // Determine thread block configuration
    int threadsPerBlock = 256;
    int blocksNeeded = (threadsNeeded + threadsPerBlock - 1) / threadsPerBlock;
    
    // Limit blocks to avoid excessive memory usage
    const int MAX_BLOCKS = 16384;
    if (blocksNeeded > MAX_BLOCKS) {
        blocksNeeded = MAX_BLOCKS;
        maxResultsPerThread = (totalCombinations + (blocksNeeded * threadsPerBlock) - 1) / (blocksNeeded * threadsPerBlock);
    }
    
    // Allocate result buffer
    CombinationResultBuffer resultBuffer = allocateCombinationResultBuffer(setA.numItems, setB.numItems, MAX_ELEMENTS_PER_VECTOR);
    
    // Launch kernel
    processAllCombinationsKernel<<<blocksNeeded, threadsPerBlock>>>(
        setA.data, setA.offsets, setA.sizes, setA.numItems,
        setB.data, setB.offsets, setB.sizes, setB.numItems,
        threshold, level,
        resultBuffer.data, resultBuffer.sizes, resultBuffer.validFlags, resultBuffer.maxResultSize,
        maxResultsPerThread
    );
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    // Count valid combinations
    std::vector<int> hostValidFlags(resultBuffer.numCombinations);
    CHECK_CUDA_ERROR(cudaMemcpy(hostValidFlags.data(), resultBuffer.validFlags, 
                              resultBuffer.numCombinations * sizeof(int), cudaMemcpyDeviceToHost));
    
    int validCount = 0;
    for (int i = 0; i < resultBuffer.numCombinations; i++) {
        if (hostValidFlags[i]) validCount++;
    }

    if (verbose) {
        printf("    Found %d valid combinations out of %lld total\n", validCount, totalCombinations);
    }

    std::vector<std::vector<int>> validCombinations;
	if (validCount > 0) {
        if (verbose) {
		    printf("    Copying result data for %d valid combinations...\n", validCount);
		}

        std::vector<int> hostSizes(resultBuffer.numCombinations);
        CHECK_CUDA_ERROR(cudaMemcpy(hostSizes.data(), resultBuffer.sizes, 
                                  resultBuffer.numCombinations * sizeof(int), cudaMemcpyDeviceToHost));
		
		std::vector<int> hostResultData(resultBuffer.numCombinations * resultBuffer.maxResultSize);
		CHECK_CUDA_ERROR(cudaMemcpy(hostResultData.data(), resultBuffer.data,
		                          resultBuffer.numCombinations * resultBuffer.maxResultSize * sizeof(int),
		                          cudaMemcpyDeviceToHost));
		
        // Progress reporting variables
		int reportInterval = validCount > 1000 ? validCount / 10 : validCount;
		int lastReportedCount = 0;
		int collectedCount = 0;

		for (int i = 0; i < resultBuffer.numCombinations; i++) {
		    if (hostValidFlags[i]) {
		        int size = hostSizes[i];
		        std::vector<int> combination(size);
		        int offset = i * resultBuffer.maxResultSize;
		        for (int j = 0; j < size; j++) {
		            combination[j] = hostResultData[offset + j];
		        }
		        validCombinations.push_back(combination);
                collectedCount++;
                
                // Progress reporting for large result sets
		        if (verbose && validCount > 1000 && collectedCount - lastReportedCount >= reportInterval) {
		            printf("    Collected %d of %d valid combinations (%.1f%%)\n", 
		                   collectedCount, validCount, 100.0 * collectedCount / validCount);
		            lastReportedCount = collectedCount;
		        }
		    }
		}

        if (verbose && validCount > 1000) {
		    printf("    Collection complete: %d combinations collected\n", collectedCount);
		}
	}

	freeCombinationResultBuffer(&resultBuffer);

    // Remove duplicates
    if (validCombinations.size() > 1) {
        if (verbose) {
            printf("    Deduplicating %zu combinations...\n", validCombinations.size());
        }
        for (auto& combination : validCombinations) {
            std::sort(combination.begin(), combination.end());
        }
        std::sort(std::execution::par, validCombinations.begin(), validCombinations.end());
        validCombinations.erase(std::unique(validCombinations.begin(), validCombinations.end()), validCombinations.end());
        if (verbose) {
            printf("    Deduplication complete: %zu unique combinations.\n", validCombinations.size());
        }
    }

    // Create result set
    HostSet resultHostSet;
    resultHostSet.vectors = validCombinations;
    
    CudaSet resultCudaSet;
    copyHostToDevice(resultHostSet, &resultCudaSet);
    
    return {resultCudaSet, ""};
}

ProcessResult processPair(const CudaSet& setA, const CudaSet& setB, int threshold, int level, bool verbose, bool allowStreaming = true) {
    int numItemsA = setA.numItems;
    int numItemsB = setB.numItems;
    
    if (verbose) {
        printf("  Processing pair at level %d: Set A (%d items) + Set B (%d items), threshold = %d\n", 
               level, numItemsA, numItemsB, threshold);
    }
    
    // Empty result for empty inputs
    if (numItemsA == 0 || numItemsB == 0) {
        CudaSet emptySet = {nullptr, nullptr, nullptr, 0, 0, nullptr, 0};
        return {emptySet, ""};
    }

    // For now, use in-memory processing for all cases
    if (verbose) {
        printf("    Using in-memory GPU processing.\n");
    }
    return processPair_inMemory(setA, setB, threshold, level, verbose);
}

// Special handling for converting a set to unique elements (for level 1 carry-over)
CudaSet convertSetToUnique(const CudaSet& set, bool verbose) {
    int numItems = set.numItems;
    
    // Allocate host vectors 
    std::vector<int> hostOffsets(numItems);
    std::vector<int> hostSizes(numItems);
    
    CHECK_CUDA_ERROR(cudaMemcpy(hostSizes.data(), set.sizes, numItems * sizeof(int), cudaMemcpyDeviceToHost));
    
    // Calculate max possible size for outputs
    int totalOutputSize = 0;
    for (int i = 0; i < numItems; i++) {
        totalOutputSize += hostSizes[i]; // Worst case: all elements are unique
    }
    
    // Create output arrays
    int8_t* d_outputData = nullptr;
    int* d_outputOffsets = nullptr;
    int* d_outputSizes = nullptr;
    
    CHECK_CUDA_ERROR(cudaMalloc((void**)&d_outputData, totalOutputSize * sizeof(int8_t)));
    CHECK_CUDA_ERROR(cudaMalloc(&d_outputOffsets, numItems * sizeof(int)));
    CHECK_CUDA_ERROR(cudaMalloc(&d_outputSizes, numItems * sizeof(int)));
    
    // Calculate output offsets (equivalent to the input offsets)
    CHECK_CUDA_ERROR(cudaMemcpy(d_outputOffsets, set.offsets, numItems * sizeof(int), cudaMemcpyDeviceToDevice));
    
    // Launch parallel kernel
    int threadsPerBlock = 256;
    int blocks = (numItems + threadsPerBlock - 1) / threadsPerBlock;
    
    convertToUniqueKernel<<<blocks, threadsPerBlock>>>(
        set.data, set.offsets, set.sizes, numItems,
        d_outputData, d_outputOffsets, d_outputSizes, MAX_ELEMENTS_PER_VECTOR
    );
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    // Create result set
    CudaSet resultSet;
    resultSet.data = d_outputData;
    resultSet.offsets = d_outputOffsets;
    resultSet.sizes = d_outputSizes;
    resultSet.numItems = numItems;
    resultSet.totalElements = totalOutputSize;
    resultSet.deviceBuffer = nullptr;
    resultSet.bufferSize = 0;
    
    if (verbose) {
        printf("  Converting carried-over set for level 2\n");
        printf("  Carried over the last set with %d items\n", numItems);
    }
    
    return resultSet;
}

// Tree fold operations (simplified version without streaming)
LevelItem treeFoldOperations(const std::vector<CudaSet>& sets, bool verbose) {
    if (sets.empty()) {
        return { {nullptr, nullptr, nullptr, 0, 0, nullptr, 0}, "", 0, -1, false };
    }

    // Initialize the first level with LevelItems
    std::vector<LevelItem> currentLevel;
    for (const auto& s : sets) {
        currentLevel.push_back({s, "", s.numItems, levelItemCounter++, false});
    }

    if (currentLevel.size() == 1) {
        return currentLevel[0];
    }
    
    if (verbose) {
        printf("Starting tree-fold operations with %zu sets\n", sets.size());
        for (const auto& item : currentLevel) {
            printf("  Set %d: %d items\n", item.id, item.numItems);
        }
    }
    
    int level = 0;
    while (currentLevel.size() > 1) {
        level++;
        if (verbose) {
            printf("\nProcessing Level %d with %zu sets\n", level, currentLevel.size());
        }
        
        std::vector<LevelItem> nextLevel;
        
        // Process pairs
        for (size_t i = 0; i < currentLevel.size() - 1; i += 2) {
            LevelItem& itemA = currentLevel[i];
            LevelItem& itemB = currentLevel[i + 1];
            
            int threshold = computeThreshold(itemA, itemB);
            
            if (verbose) {
                printf("  --> Processing pair: Set %d (%d items) + Set %d (%d items) with threshold %d\n", 
                       itemA.id, itemA.numItems, itemB.id, itemB.numItems, threshold);
            }

            ProcessResult res = processPair(itemA.set, itemB.set, threshold, level, verbose, false);
            LevelItem resultItem = { res.set, res.streamPath, res.set.numItems, levelItemCounter++, true };
            nextLevel.push_back(resultItem);
        }
        
        // Handle odd set by carrying it over
        if (currentLevel.size() % 2 == 1) {
            LevelItem& carriedItem = currentLevel.back();
            if (verbose) {
                printf("  --> Carrying over odd set %d (%d items) to next level\n", 
                       carriedItem.id, carriedItem.numItems);
            }

            // For level 1, convert the carried-over set to unique elements
            if (level == 1) {
               CudaSet convertedSet = convertSetToUnique(carriedItem.set, verbose);
               nextLevel.push_back({convertedSet, "", convertedSet.numItems, levelItemCounter++, true});
            } else {
               carriedItem.needsCleanup = false;
               nextLevel.push_back(carriedItem);
            }
        }
        
        // Clean up resources from the completed level
        for(const auto& item : currentLevel) {
            if(item.needsCleanup) {
                freeCudaSet(&const_cast<CudaSet&>(item.set));
            }
        }
        
        currentLevel = nextLevel;
    }
    
    return currentLevel[0];
}

// Kernel to filter out negative values and sort
__global__ void filterAndSortKernel(int8_t* data, int* offsets, int* sizes, int numVectors, int maxLen) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numVectors) return;
    
    int offset = offsets[idx];
    int originalSize = sizes[idx];
    
    // Step 1: Filter out negatives
    int newSize = 0;
    for (int i = 0; i < originalSize; i++) {
        int val = data[offset + i];
        if (val >= 0) {
            // Keep only non-negative values
            data[offset + newSize] = val;
            newSize++;
        }
    }
    
    // Update size
    sizes[idx] = newSize;
    
    // Step 2: Sort (simple insertion sort)
    for (int i = 1; i < newSize; i++) {
        int key = data[offset + i];
        int j = i - 1;
        
        while (j >= 0 && data[offset + j] > key) {
            data[offset + j + 1] = data[offset + j];
            j--;
        }
        
        data[offset + j + 1] = key;
    }
}

// Function to post-process on GPU then complete ordering on CPU
std::vector<std::vector<int>> gpuPostProcess(const CudaSet& resultSet, bool verbose) {
    // Step 1: Run GPU kernel to filter and sort all vectors internally
    int threadsPerBlock = 256;
    int blocks = (resultSet.numItems + threadsPerBlock - 1) / threadsPerBlock;
    
    if (verbose) {
        printf("Running GPU post-processing on %d vectors\n", resultSet.numItems);
    }
    
    filterAndSortKernel<<<blocks, threadsPerBlock>>>(
        resultSet.data, resultSet.offsets, resultSet.sizes, 
        resultSet.numItems, MAX_ELEMENTS_PER_VECTOR);
    
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
    
    if (verbose) {
        printf("GPU internal sorting complete, transferring to host for final sorting\n");
    }
    
    // Step 2: Process in batches to avoid memory issues
    const int BATCH_SIZE = 100000;
    int totalVectors = resultSet.numItems;
    int batches = (totalVectors + BATCH_SIZE - 1) / BATCH_SIZE;
    
    std::vector<std::vector<int>> processedResults;
    processedResults.reserve(std::min(totalVectors, 10000000)); // Reserve reasonable amount
    
    for (int batch = 0; batch < batches; batch++) {
        int start = batch * BATCH_SIZE;
        int end = std::min(start + BATCH_SIZE, totalVectors);
        
        if (verbose) {
            printf("Processing batch %d/%d (vectors %d to %d)\n", batch+1, batches, start, end-1);
        }
        
        // Extract subset of the CudaSet
        CudaSet batchSet = extractSubset(resultSet, start, end - start, false);
        
        // Process this batch - already filtered and sorted internally by GPU
        HostSet hostBatch = copyDeviceToHost(batchSet);
        
        // Add to results
        for (const auto& vector : hostBatch.vectors) {
            processedResults.push_back(vector);
        }
        
        // Free batch resources
        freeCudaSet(&batchSet);
        
        // Sort intermediate results if getting too large
        if (processedResults.size() > 1000000) {
            if (verbose) {
                printf("  Performing intermediate sort of %zu results\n", processedResults.size());
            }
            std::sort(processedResults.begin(), processedResults.end());
        }
    }
    
    // Final lexicographical sorting of all vectors
    if (verbose) {
        printf("Performing final lexicographical sort of %zu vectors\n", processedResults.size());
    }
    std::sort(processedResults.begin(), processedResults.end());
    
    return processedResults;
}

// Run test cases with Witness JSON
void runWitnessTestCases(const std::string& filename) {
    std::vector<std::vector<std::vector<int>>> testSets = 
        generateWitnessSetsFromJSON(filename);
    
    if (testSets.empty()) {
        printf("No test sets generated. Exiting.\n");
        return;
    }
    
    // Show input sets
    for (size_t i = 0; i < testSets.size(); i++) {
        printf("  Set %zu: [", i + 1);
        for (size_t j = 0; j < testSets[i].size() && j < 2; j++) {
            printf("[");
            for (size_t k = 0; k < testSets[i][j].size(); k++) {
                printf("%d", testSets[i][j][k]);
                if (k < testSets[i][j].size() - 1) printf(", ");
            }
            printf("]");
            if (j < testSets[i].size() - 1) printf(", ");
        }
        if (testSets[i].size() > 2) printf("...");
        printf("] (%zu items)\n", testSets[i].size());
    }
    
    // Create host sets
    std::vector<HostSet> hostSets;
    for (const auto& vectors : testSets) {
        hostSets.push_back(createTestSet(vectors));
    }
    
    // Convert host sets to CUDA sets
    std::vector<CudaSet> cudaSets;
    for (const auto& hostSet : hostSets) {
        CudaSet cudaSet;
        copyHostToDevice(hostSet, &cudaSet);
        cudaSets.push_back(cudaSet);
    }
    
    // Record start time
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);
    
    // Run tree-fold operations
    LevelItem finalResult = treeFoldOperations(cudaSets, true);
    
    // Record end time
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    
    printf("\nTree-fold completed in %.2f ms. Total items: %d\n", milliseconds, finalResult.numItems);
    
    std::vector<std::vector<int>> finalVectors;

    if (finalResult.numItems > 0) {
        printf("Final result is in memory (%d items). Post-processing on GPU...\n", finalResult.numItems);
        // Process the results from GPU memory
        finalVectors = gpuPostProcess(finalResult.set, true);
        freeCudaSet(&const_cast<CudaSet&>(finalResult.set));
    } else {
        printf("Final result is empty.\n");
    }
    
    printf("Final processed result contains %zu combinations\n", finalVectors.size());
    
    // Open file for writing
    FILE* outFile = fopen("zdd.bin", "wb");
    if (!outFile) {
        fprintf(stderr, "Error: Could not open zdd.bin for writing\n");
    } else {
        for (const auto& vec : finalVectors) {
            int size = vec.size();
            fwrite(&size, sizeof(int), 1, outFile);
            fwrite(vec.data(), sizeof(int), size, outFile);
        }
        fclose(outFile);
        printf("Results written to zdd.bin\n");
    }
    
    // Clean up original sets
    for (size_t i = 0; i < cudaSets.size(); i++) {
        freeCudaSet(&cudaSets[i]);
    }
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
}

//-------------------------------------------------------------------------
// Main function
//-------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 2) {
        printf("Usage: %s <witness_json_file>\n", argv[0]);
        printf("Example: %s witness_export.json\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    std::string filename = argv[1];
    
    // Initialize CUDA
    int deviceCount;
    CHECK_CUDA_ERROR(cudaGetDeviceCount(&deviceCount));
    if (deviceCount == 0) {
        fprintf(stderr, "No CUDA devices found.\n");
        return EXIT_FAILURE;
    }
    CHECK_CUDA_ERROR(cudaSetDevice(0));
    
    printf("Processing Witness JSON file: %s\n", filename.c_str());
    
    // Run Witness test cases
    runWitnessTestCases(filename);
    
    // Clean up
    CHECK_CUDA_ERROR(cudaDeviceReset());
    
    return 0;
}
