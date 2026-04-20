#include <iostream>

namespace sjtu {
    template<typename... Args>
    struct format_string {
        consteval format_string(const char* fmt) : str(fmt) {
            // Simple validation
        }
        const char* str;
    };

    template<typename... Args>
    using format_string_t = format_string<Args...>;

    template<typename... Args>
    void printf(format_string_t<Args...> fmt, const Args&... args) {
        std::cout << fmt.str << std::endl;
    }
}

int main() {
    sjtu::printf("Hello %s", "world");
    return 0;
}
