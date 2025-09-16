#ifndef MMM_BASEMAPMETA_HPP
#define MMM_BASEMAPMETA_HPP

#include <filesystem>
#include <string>

// 基本谱面信息
struct BaseMapMeta {
    // 谱面名称
    std::string name;
    // 谱面歌曲标题
    std::string title;
    // 谱面歌曲标题(unicode)
    std::string title_unicode;
    // 谱面歌曲艺术家
    std::string artist;
    // 谱面歌曲艺术家(unicode)
    std::string artist_unicode;
    // 谱面文件路径
    std::filesystem::path map_path;
    // 主音频文件路径
    std::filesystem::path main_audio_path;
    // 主背景文件路径
    std::filesystem::path main_cover_path;

    // 谱面版本名
    std::string version;

    // 谱面参考bpm
    double preference_bpm{100.};
    // 谱面轨道数
    uint32_t track_count{4};
    // 谱面总时长
    uint32_t map_length{0};
};

#endif  // MMM_BASEMAPMETA_HPP
