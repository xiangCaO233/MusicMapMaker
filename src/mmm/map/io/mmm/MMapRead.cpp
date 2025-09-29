#include <colorful-log.h>

#include <cstdint>
#include <fstream>
#include <memory>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>
#include <mmm/timing/Timing.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void MMap::readMMM() {
    // 切换为绝对路径
    if (basemeta.map_path.is_relative()) {
        basemeta.map_path = std::filesystem::absolute(basemeta.map_path);
    }
    auto fname = basemeta.map_path.filename();
    XINFO("路径:" + basemeta.map_path.string());
    json map_data_json;
    std::ifstream fis(basemeta.map_path);
    fis >> map_data_json;
    // XINFO("mapdata: " + map_data_json.dump());
    // 读取数据
    // 基本元数据
    basemeta.artist = map_data_json["artist"].get<std::string>();
    basemeta.artist_unicode =
        map_data_json["artist-unicode"].get<std::string>();
    basemeta.title = map_data_json["title"].get<std::string>();
    basemeta.title_unicode = map_data_json["title-unicode"].get<std::string>();
    basemeta.author = map_data_json["author"].get<std::string>();
    basemeta.map_length = map_data_json["maplength"].get<int64_t>();
    auto maintrack_rpath = map_data_json["music"].get<std::string>();
    basemeta.main_audio_path =
        basemeta.map_path.parent_path() / maintrack_rpath;
    auto mainbg_rpath = map_data_json["bg"].get<std::string>();
    basemeta.main_cover_path = basemeta.map_path.parent_path() / mainbg_rpath;
    basemeta.version = map_data_json["version"].get<std::string>();
    basemeta.preference_bpm = map_data_json["preference-bpm"].get<double>();
    basemeta.track_count = map_data_json["track-count"].get<int32_t>();

    // 读取额外元数据

    // 读取物件数据
    std::unique_ptr<Note> reading_note{nullptr};
    auto notes_data = map_data_json["notes"];
    for (auto& data : notes_data) {
        auto note_type = data["type"].get<std::string>();
        if (note_type == "NORMAL") {
            reading_note = std::make_unique<Note>(this);
        } else if (note_type == "HOLD") {
            reading_note = std::make_unique<Hold>(this);
        } else if (note_type == "SLIDE") {
            reading_note = std::make_unique<Slide>(this);
        } else if (note_type == "COMPOSITE") {
            reading_note = std::make_unique<Composite>(this);
        }
        reading_note->fromJson(data);
        auto handle = note_set().add_note(std::move(reading_note));
        noteUUIDManager.register_new_note(handle);
    }

    // 读取timing点信息
    std::unique_ptr<Timing> reading_timing{nullptr};
    auto timings_data = map_data_json["timings"];
    for (auto& data : timings_data) {
        reading_timing = std::make_unique<Timing>();
        reading_timing->fromJson(data);
        timing_set().add_timing_point(std::move(reading_timing));
    }

    basemeta.name = "[mmm] " + basemeta.artist_unicode + "-" +
                    basemeta.title_unicode + "(" + basemeta.author + ") [" +
                    std::to_string(basemeta.track_count) + "k] - " +
                    basemeta.version;

    update_preferenceBPM();

    analyzeBeatInfo();
}
