#include "printf.hpp"
#include <iostream>
#include <vector>

int main() {
    // Test basic string formatting
    sjtu::printf("Hello, %s!\n", "World");

    // Test integer formatting
    sjtu::printf("Number: %d\n", 42);
    sjtu::printf("Unsigned: %u\n", 42u);

    // Test escape
    sjtu::printf("Percentage: %%\n");

    // Test default formatting
    sjtu::printf("Default int: %_\n", 42);
    sjtu::printf("Default unsigned: %_\n", 42u);

    // Test vector
    std::vector<int> vec = {1, 2, 3};
    sjtu::printf("Vector: %_\n", vec);

    return 0;
}
