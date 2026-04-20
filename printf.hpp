#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace sjtu {

using sv_t = std::string_view;

struct format_error : std::exception {
public:
    format_error(const char *msg = "invalid format") : msg(msg) {}
    auto what() const noexcept -> const char * override { return msg; }

private:
    const char *msg;
};

template <typename Tp>
struct formatter;

struct format_info {
    inline static constexpr auto npos = static_cast<std::size_t>(-1);
    std::size_t position; // where is the specifier
    std::size_t consumed; // how many characters consumed
};

template <typename... Args>
struct format_string {
public:
    consteval format_string(const char *fmt);
    constexpr auto get_format() const -> std::string_view { return fmt_str; }
    constexpr auto get_index() const -> std::span<const format_info> {
        return fmt_idx;
    }

private:
    inline static constexpr auto Nm = sizeof...(Args);
    std::string_view fmt_str;
    std::array<format_info, Nm> fmt_idx;
};

// Helper to find next specifier starting from given position
consteval auto find_next_spec(sv_t fmt, std::size_t start) -> std::size_t {
    for (std::size_t i = start; i < fmt.size(); ++i) {
        if (fmt[i] == '%') {
            if (i + 1 >= fmt.size()) {
                throw format_error{"missing specifier after '%'"};
            }
            if (fmt[i + 1] != '%') {
                return i;  // Found a real specifier
            }
            // Skip %%
            ++i;
        }
    }
    return sv_t::npos;
}

// Count characters between start and the next specifier (accounting for %%)
consteval auto count_to_spec(sv_t fmt, std::size_t start) -> std::size_t {
    std::size_t count = 0;
    for (std::size_t i = start; i < fmt.size(); ++i) {
        if (fmt[i] == '%') {
            if (i + 1 >= fmt.size()) {
                throw format_error{"missing specifier after '%'"};
            }
            if (fmt[i + 1] != '%') {
                return count;  // Found specifier
            }
            // %% counts as one character
            ++count;
            ++i;  // Skip second %
        } else {
            ++count;
        }
    }
    return count;
}

// Recursive template to parse format string
template <std::size_t N, typename... Args>
consteval auto parse_format_recursive(sv_t fmt, std::size_t pos,
                                       std::array<format_info, sizeof...(Args)> &arr)
    -> void {
    if constexpr (N < sizeof...(Args)) {
        using ArgType = std::tuple_element_t<N, std::tuple<Args...>>;

        std::size_t spec_pos = find_next_spec(fmt, pos);
        if (spec_pos == sv_t::npos) {
            arr[N] = {.position = format_info::npos, .consumed = 0};
            return;
        }

        std::size_t char_count = count_to_spec(fmt, pos);
        std::size_t spec_start = spec_pos + 1;  // After '%'

        formatter<ArgType> parser;
        std::size_t consumed = parser.parse(fmt.substr(spec_start));

        arr[N] = {.position = char_count, .consumed = consumed};

        std::size_t next_pos = spec_start + (consumed > 0 ? consumed : (fmt[spec_start] == '_' ? 1 : 0));
        if (consumed == 0 && fmt[spec_start] != '_') {
            throw format_error{"invalid specifier"};
        }

        parse_format_recursive<N + 1, Args...>(fmt, next_pos, arr);
    }
}

template <typename... Args>
consteval auto compile_time_format_check(sv_t fmt)
    -> std::array<format_info, sizeof...(Args)> {
    std::array<format_info, sizeof...(Args)> result{};

    if constexpr (sizeof...(Args) > 0) {
        parse_format_recursive<0, Args...>(fmt, 0, result);

        // Check for extra specifiers
        std::size_t last_pos = 0;
        for (std::size_t i = 0; i < sizeof...(Args); ++i) {
            if (result[i].position != format_info::npos) {
                std::size_t spec_pos = find_next_spec(fmt, last_pos);
                last_pos = spec_pos + 1 + result[i].consumed;
            }
        }
        if (find_next_spec(fmt, last_pos) != sv_t::npos) {
            throw format_error{"too many specifiers"};
        }
    } else {
        // No args, check no specifiers
        if (find_next_spec(fmt, 0) != sv_t::npos) {
            throw format_error{"too many specifiers"};
        }
    }

    return result;
}

template <typename... Args>
consteval format_string<Args...>::format_string(const char *fmt)
    : fmt_str(fmt), fmt_idx(compile_time_format_check<Args...>(fmt_str)) {}

// String-like types formatter
template <typename StrLike>
    requires(std::same_as<StrLike, std::string> ||
             std::same_as<StrLike, std::string_view> ||
             std::same_as<StrLike, char *> || std::same_as<StrLike, const char *>)
struct formatter<StrLike> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        return fmt.starts_with("s") ? 1 : 0;
    }
    static auto format_to(std::ostream &os, const StrLike &val, sv_t fmt = "s")
        -> void {
        if (fmt.starts_with("s") || fmt.starts_with("_") || fmt.empty()) {
            os << static_cast<sv_t>(val);
        } else {
            throw format_error{};
        }
    }
};

// Integer types formatter
template <typename Int>
    requires std::integral<Int>
struct formatter<Int> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("d"))
            return 1;
        if (fmt.starts_with("u"))
            return 1;
        return 0; // for %_
    }
    static auto format_to(std::ostream &os, const Int &val, sv_t fmt = "_") -> void {
        if (fmt.starts_with("d")) {
            os << static_cast<int64_t>(val);
        } else if (fmt.starts_with("u")) {
            os << static_cast<uint64_t>(val);
        } else if (fmt.starts_with("_") || fmt.empty()) {
            if constexpr (std::is_signed_v<Int>) {
                os << static_cast<int64_t>(val);
            } else {
                os << static_cast<uint64_t>(val);
            }
        } else {
            throw format_error{};
        }
    }
};

// Vector formatter
template <typename T>
struct formatter<std::vector<T>> {
    static constexpr auto parse(sv_t fmt) -> std::size_t { return 0; // for %_
    }
    static auto format_to(std::ostream &os, const std::vector<T> &val,
                          sv_t fmt = "_") -> void {
        if (fmt.starts_with("_") || fmt.empty()) {
            os << "[";
            for (size_t i = 0; i < val.size(); ++i) {
                if (i > 0)
                    os << ",";
                formatter<T>::format_to(os, val[i], "_");
            }
            os << "]";
        } else {
            throw format_error{};
        }
    }
};

template <typename... Args>
using format_string_t = format_string<std::decay_t<Args>...>;

template <typename... Args>
inline auto printf(format_string_t<Args...> fmt, const Args &...args) -> void {
    auto format_str = fmt.get_format();
    auto indices = fmt.get_index();

    std::size_t arg_idx = 0;
    std::size_t current_pos = 0;

    auto format_one = [&](const auto &arg) {
        if (arg_idx >= indices.size()) {
            return;
        }

        const auto &info = indices[arg_idx];

        if (info.position != format_info::npos) {
            // Output text before specifier (handling %%)
            std::size_t chars_output = 0;
            while (chars_output < info.position && current_pos < format_str.size()) {
                if (format_str[current_pos] == '%' &&
                    current_pos + 1 < format_str.size() &&
                    format_str[current_pos + 1] == '%') {
                    std::cout << '%';
                    current_pos += 2;
                } else {
                    std::cout << format_str[current_pos];
                    current_pos++;
                }
                chars_output++;
            }

            // Skip '%' of specifier
            current_pos++;

            // Get format spec
            sv_t spec = format_str.substr(
                current_pos, info.consumed > 0 ? info.consumed : 1);

            // Format argument
            using ArgType = std::decay_t<decltype(arg)>;
            formatter<ArgType>::format_to(std::cout, arg, spec);

            current_pos += info.consumed > 0 ? info.consumed : 1;
        }

        arg_idx++;
    };

    (format_one(args), ...);

    // Output remaining text
    while (current_pos < format_str.size()) {
        if (format_str[current_pos] == '%' && current_pos + 1 < format_str.size() &&
            format_str[current_pos + 1] == '%') {
            std::cout << '%';
            current_pos += 2;
        } else {
            std::cout << format_str[current_pos];
            current_pos++;
        }
    }
}

} // namespace sjtu
