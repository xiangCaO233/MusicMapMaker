#ifndef M_MAPMETADATA_H
#define M_MAPMETADATA_H

#include <sstream>
#include <string>
#include <unordered_map>

enum class MapMetadataType {
    MOSU,
    MMALODY,
    MIMD,
};

class MapMetadata {
   public:
    // 构造MapMetadata
    MapMetadata() {};
    // 析构MapMetadata
    virtual ~MapMetadata() = default;

    // 元数据类型
    MapMetadataType type;

    // 属性表
    std::unordered_map<std::string, std::string> map_properties;

    // 获取数据
    template <typename T>
    T get_value(const std::string& key, T default_value = T()) {
        auto key_it = map_properties.find(key);
        if (key_it == map_properties.end()) return default_value;
        if constexpr (std::is_same_v<T, std::string>) {
            // 类型为字符串时整个返回
            return key_it->second;
        } else {
            std::istringstream iss(key_it->second);
            T value;
            iss >> value;
            return value;
        }
    }
};

enum class NoteMetadataType {
    NOSU,
    NMALODY,
};

class NoteMetadata {
   public:
    // 构造NoteMetadata
    NoteMetadata() {};
    // 析构NoteMapMetadata
    virtual ~NoteMetadata() = default;

    // 元数据类型
    NoteMetadataType type;

    // 属性表
    std::unordered_map<std::string, std::string> note_properties;

    // 获取数据
    template <typename T>
    T get_value(const std::string& key, T default_value = T()) {
        auto key_it = note_properties.find(key);
        if (key_it == note_properties.end()) return default_value;
        if constexpr (std::is_same_v<T, std::string>) {
            // 类型为字符串时整个返回
            return key_it->second;
        } else {
            std::istringstream iss(key_it->second);
            T value;
            iss >> value;
            return value;
        }
    }
};

enum class TimingMetadataType {
    TOSU,
    TMALODY,
};

class TimingMetadata {
   public:
    // 构造NoteMetadata
    TimingMetadata() {};
    // 析构NoteMapMetadata
    virtual ~TimingMetadata() = default;

    // 元数据类型
    TimingMetadataType type;

    // 属性表
    std::unordered_map<std::string, std::string> note_properties;

    // 获取数据
    template <typename T>
    T get_value(const std::string& key, T default_value = T()) {
        auto key_it = note_properties.find(key);
        if (key_it == note_properties.end()) return default_value;
        if constexpr (std::is_same_v<T, std::string>) {
            // 类型为字符串时整个返回
            return key_it->second;
        } else {
            std::istringstream iss(key_it->second);
            T value;
            iss >> value;
            return value;
        }
    }
};

#endif  // M_MAPMETADATA_H
