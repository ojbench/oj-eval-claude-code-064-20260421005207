#include "printf.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // This main.cpp is for demonstration/testing
    // The actual test will be based on what the OJ provides

    // Test basic string formatting
    sjtu::printf("Test 1: %s\n", "Hello");

    // Test integer formatting
    sjtu::printf("Test 2: %d\n", 42);
    sjtu::printf("Test 3: %u\n", 42u);

    // Test escape
    sjtu::printf("Test 4: %%\n");

    // Test default formatting
    sjtu::printf("Test 5: %_\n", 42);
    sjtu::printf("Test 6: %_\n", 42u);
    sjtu::printf("Test 7: %_\n", "String");

    // Test vector
    std::vector<int> vec = {1, 2, 3};
    sjtu::printf("Test 8: %_\n", vec);

    return 0;
}
