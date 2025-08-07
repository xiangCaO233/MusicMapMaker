#ifndef MMM_STRINGHASH_HPP
#define MMM_STRINGHASH_HPP

#include <string>
#include <string_view>

struct StringHash {
    // 这个标签用于开启透明性
    using is_transparent = void;
    [[nodiscard]] size_t operator()(const char* txt) const {
        return std::hash<std::string_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(std::string_view txt) const {
        return std::hash<std::string_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(const std::string& txt) const {
        return std::hash<std::string>{}(txt);
    }
};

#endif  // MMM_STRINGHASH_HPP
