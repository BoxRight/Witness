#include <iostream>
#include <string>
#include "driver.hpp"

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [--solver=exhaustive|external] <filename>" << std::endl;
    std::cerr << "  --solver=exhaustive  Use built-in exhaustive solver (default)" << std::endl;
    std::cerr << "  --solver=external    Use external solver for large contracts" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string solverMode = "exhaustive"; // Default solver
    std::string filename;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg.substr(0, 9) == "--solver=") {
            solverMode = arg.substr(9);
            if (solverMode != "exhaustive" && solverMode != "external") {
                std::cerr << "Error: Invalid solver mode '" << solverMode << "'" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        } else if (arg[0] == '-') {
            std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
            printUsage(argv[0]);
            return 1;
        } else {
            if (filename.empty()) {
                filename = arg;
            } else {
                std::cerr << "Error: Multiple input files specified" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        }
    }
    
    if (filename.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    witness::Driver driver;
    
    // Set solver mode in driver
    driver.setSolverMode(solverMode);
    
    std::cout << "Using solver mode: " << solverMode << std::endl;
    
    int result = driver.parse(filename);

    if (result == 0) {
        std::cout << "Parsing successful!" << std::endl;
        
        if (driver.get_program()) {
            std::cout << "--- AST ---" << std::endl;
            driver.print_ast();
            std::cout << "-----------" << std::endl;
            
            // Run semantic analysis
            std::cout << std::endl;
            driver.analyze();
            std::cout << std::endl;
        }
    } else {
        std::cerr << "Parsing failed." << std::endl;
    }

    return result;
} 