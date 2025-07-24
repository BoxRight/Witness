#include <iostream>
#include <string>
#include "driver.hpp"

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [options] <filename>" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --solver=exhaustive|external  Solver mode (default: exhaustive)" << std::endl;
    std::cerr << "  --verbose                    Show detailed output (AST, warnings, debug info)" << std::endl;
    std::cerr << "  --quiet                      Suppress all non-error output" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string solverMode = "exhaustive"; // Default solver
    std::string filename;
    bool verbose = false;
    bool quiet = false;
    
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
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--quiet") {
            quiet = true;
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
    
    // Set solver mode and verbosity in driver
    driver.setSolverMode(solverMode);
    driver.setVerbose(verbose);
    driver.setQuiet(quiet);
    
    if (!quiet) {
        std::cout << "Using solver mode: " << solverMode << std::endl;
    }
    
    int result = driver.parse(filename);

    if (result == 0) {
        if (!quiet) {
            std::cout << "Parsing successful!" << std::endl;
        }
        
        if (driver.get_program()) {
            if (verbose) {
                std::cout << "--- AST ---" << std::endl;
                driver.print_ast();
                std::cout << "-----------" << std::endl;
            }
            
            // Run semantic analysis
            if (!quiet) {
                std::cout << std::endl;
            }
            driver.analyze();
            if (!quiet) {
                std::cout << std::endl;
            }
        }
    } else {
        std::cerr << "Parsing failed." << std::endl;
    }

    return result;
} 