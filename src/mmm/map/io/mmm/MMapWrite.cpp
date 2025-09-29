#include <filesystem>
#include <fstream>
#include <mmm/map/MMap.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void MMap::writeMMM(const std::string& desfile) {
    auto des_path = std::filesystem::path(desfile);
    // 写出为mmm-json
    //
    // 更新json
    //
    // 谱面数据json
    json mapdata_json;

    // 基本共通数据
    mapdata_json["title"] = basemeta.title;
    mapdata_json["title-unicode"] = basemeta.title_unicode;
    mapdata_json["artist"] = basemeta.artist;
    mapdata_json["artist-unicode"] = basemeta.artist_unicode;
    mapdata_json["version"] = basemeta.version;
    mapdata_json["author"] = basemeta.author;
    mapdata_json["preference-bpm"] = basemeta.preference_bpm;
    mapdata_json["maplength"] = basemeta.map_length;
    mapdata_json["track-count"] = basemeta.track_count;
    mapdata_json["music"] =
        std::filesystem::relative(basemeta.main_audio_path,
                                  basemeta.map_path.parent_path())
            .generic_string();
    mapdata_json["bg"] =
        std::filesystem::relative(basemeta.main_cover_path,
                                  basemeta.map_path.parent_path())
            .generic_string();
    mapdata_json["name"] = basemeta.name;

    // 时间点数据
    auto& timings_json = mapdata_json["timings"];
    int index{0};
    for (const auto& [time, timing_vec] :
         timing_set().get_all_timing_points()) {
        for (const auto& timing : timing_vec) {
            timings_json[index] = timing->toJson();
            ++index;
        }
    }

    // 物件数据
    auto& notes_json = mapdata_json["notes"];
    index = 0;
    for (const auto& notehandle : note_set().get_all_notes_ordered()) {
        const auto& note = note_set().get_note(notehandle);
        notes_json[index] = note->toJson();
        ++index;
    }

    // 写出到文件
    // 打开文件输出流-覆盖模式
    std::ofstream out(des_path);
    out << mapdata_json.dump(4);
    out.close();
}
