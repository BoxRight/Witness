#include <iostream>
#include "driver.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    witness::Driver driver;
    int result = driver.parse(argv[1]);

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