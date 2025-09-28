#ifndef MMM_METADATA_HPP
#define MMM_METADATA_HPP

#include <sstream>
#include <string>
#include <unordered_map>
#include <util/StringHash.hpp>

enum class MapMetadataType {
    OSU,
    MALODY,
    IMD,
};

class MapMetadata {
   public:
    // 构造MapMetadata
    MapMetadata() = default;
    // 析构MapMetadata
    virtual ~MapMetadata() = default;

    // 元数据类型
    MapMetadataType type;

    // 统一通用属性表(来源-[属性名-属性值])
    std::unordered_map<MapMetadataType,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>>
        map_properties;

    // 获取数据
    template <typename T>
    T get_value(MapMetadataType source, const std::string& key,
                T default_value = T()) {
        auto properties_it = map_properties.find(source);
        if (properties_it == map_properties.end()) return default_value;
        auto key_it = properties_it->second.find(key);
        if (key_it == properties_it->second.end()) return default_value;
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
    OSU,
    MALODY,
};

class NoteMetadata {
   public:
    // 构造NoteMetadata
    NoteMetadata() = default;
    // 析构NoteMapMetadata
    virtual ~NoteMetadata() = default;

    // 元数据类型
    NoteMetadataType meta_type;

    // 统一通用属性表(来源-[属性名-属性值])
    std::unordered_map<NoteMetadataType,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>>
        note_properties;

    // 获取数据
    template <typename T>
    T get_value(NoteMetadataType source, const std::string& key,
                T default_value = T()) {
        auto properties_it = note_properties.find(source);
        if (properties_it == note_properties.end()) return default_value;
        auto key_it = properties_it->second.find(key);
        if (key_it == properties_it->second.end()) return default_value;
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
    OSU,
    MALODY,
};

class TimingMetadata {
   public:
    // 构造NoteMetadata
    TimingMetadata() = default;
    // 析构NoteMapMetadata
    virtual ~TimingMetadata() = default;

    // 元数据类型
    TimingMetadataType metatype;

    // 属性表
    std::unordered_map<std::string, std::string, StringHash, std::equal_to<>>
        note_properties;

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

#endif  // MMM_METADATA_HPP
