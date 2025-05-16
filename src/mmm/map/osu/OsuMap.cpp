#include "OsuMap.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../../hitobject/Note/osu/OsuHold.h"
#include "../../hitobject/Note/osu/OsuNote.h"
#include "../../timing/osu/OsuTiming.h"
#include "../MMap.h"
#include "colorful-log.h"
#include "mmm/timing/Timing.h"
#include "src/mmm/hitobject/HitObject.h"
#include "src/mmm/hitobject/Note/Note.h"
#include "util/mutil.h"

OsuFileReader::OsuFileReader() {};

// 格式化行
void OsuFileReader::parse_line(const std::string& line) {
    if (line[0] == '[' && line.back() == ']') {
        current_chapter = line.substr(1, line.size() - 2);
    } else {
        //[Events]	谱面显示设定，故事板事件	逗号分隔的列表
        //[TimingPoints]	时间轴设定	逗号分隔的列表
        //[HitObjects]	击打物件	逗号分隔的列表

        if (current_chapter == "Events") {
            auto start_5_string = line.substr(0, 5);
            auto start_1_char = line.at(0);
            // XINFO(start_5_string);
            // 并非一定五个参数
            if (start_5_string == "Video") {
                map_properties[current_chapter]["background video"] = line;
            } else if (start_5_string == "Break") {
                map_properties[current_chapter]
                              ["Break" +
                               std::to_string(current_breaks_index++)] = line;
            } else if (start_1_char == '0') {
                // general bg
                map_properties[current_chapter]["background"] = line;
            }
            // int commas = std::count(line.begin(), line.end(), ',');
            // byd被wiki骗了，不能用逗号数区分事件类型
            // switch (commas) {
            //   case 4: {
            //     map_properties[current_chapter]["background"] = line;
            //     break;
            //   }
            //   case 2: {
            //     map_properties[current_chapter]
            //                   [std::to_string(current_breaks_index++)] =
            //                   line;
            //     break;
            //   }
            //   default:
            //     map_properties[current_chapter]["unknown"] = line;
            // }
        } else if (current_chapter == "TimingPoints") {
            map_properties[current_chapter]
                          [std::to_string(current_timing_index++)] = line;
        } else if (current_chapter == "HitObjects") {
            map_properties[current_chapter]
                          [std::to_string(current_hitobject_index++)] = line;
        } else {
            size_t eq_pos = line.find(':');
            if (eq_pos != std::string::npos) {
                std::string key = line.substr(0, eq_pos);
                std::string value = line.substr(eq_pos + 1);
                // XWARN("key:" + key);
                // XWARN("value:" + value);
                map_properties[current_chapter][key] = value;
            }
        }
    }
}

OsuMap::OsuMap() : MMap() {
    maptype = MapType::OSUMAP;
    // 注册元数据
    register_metadata(MapMetadataType::OSU);
}

OsuMap::~OsuMap() = default;

// osu格式默认的元数据
std::shared_ptr<MapMetadata> OsuMap::default_metadata() {
    auto meta = std::make_shared<MapMetadata>();
    // general
    meta->map_properties["AudioFilename"] = "";
    meta->map_properties["AudioLeadIn"] = "0";
    meta->map_properties["AudioLeadHash"] = "";
    meta->map_properties["PreviewTime"] = "-1";
    meta->map_properties["Countdown"] = "1";
    meta->map_properties["SampleSet"] = "1";
    meta->map_properties["StackLeniency"] = "0.0";
    meta->map_properties["LetterboxInBreaks"] = "0";
    meta->map_properties["StoryFireInFront"] = "1";
    meta->map_properties["UseSkinSprites"] = "0";
    meta->map_properties["AlwaysShowPlayfield"] = "0";
    meta->map_properties["OverlayPosition"] = "NoChange";
    meta->map_properties["SkinPreference"] = "";
    meta->map_properties["EpilepsyWarning"] = "0";
    meta->map_properties["CountdownOffset"] = "0";
    meta->map_properties["SpecialStyle"] = "0";
    meta->map_properties["WidescreenStoryboard"] = "0";
    meta->map_properties["SamplesMatchPlaybackRate"] = "0";

    // editor
    meta->map_properties["Bookmarks"] = "";
    meta->map_properties["DistanceSpacing"] = "0.0";
    meta->map_properties["BeatDivisor"] = "0";
    meta->map_properties["GridSize"] = "0";
    meta->map_properties["TimelineZoom"] = "0.0";

    // metadata
    meta->map_properties["Title"] = "";
    meta->map_properties["TitleUnicode"] = "";
    meta->map_properties["Artist"] = "";
    meta->map_properties["ArtistUnicode"] = "";
    meta->map_properties["Creator"] = "mmm";
    meta->map_properties["Version"] = "mmm";
    meta->map_properties["Source"] = "";
    meta->map_properties["Tags"] = "";
    meta->map_properties["BeatmapID"] = "-1";
    meta->map_properties["BeatmapSetID"] = "-1";

    // difficulty
    meta->map_properties["HPDrainRate"] = "5.0";
    meta->map_properties["CircleSize"] = "4.0";
    meta->map_properties["OverallDifficulty"] = "8.0";
    meta->map_properties["ApproachRate"] = "0.0";
    meta->map_properties["SliderMultiplier"] = "0.0";
    meta->map_properties["SliderTickRate"] = "0.0";

    // colour--- 不写

    // event
    // bg
    meta->map_properties["background"] = "0,0,\"bg.png\",0,0";

    // breaks
    return meta;
}

// 从文件读取谱面
void OsuMap::load_from_file(const char* path) {
    map_file_path = std::filesystem::path(path);
    if (map_file_path.is_relative()) {
        map_file_path = std::filesystem::absolute(map_file_path);
    }
    auto fname = map_file_path.filename();
    XINFO("路径:" + map_file_path.string());
    if (map_file_path.extension() == ".osu") {
        // XINFO("load_osu:" + p.extension().string());
        std::ifstream ifs(map_file_path);
        if (!ifs.is_open()) {
            XERROR("打开文件[" + map_file_path.string() + "]失败");
            return;
        }
        OsuFileReader osureader;
        std::string read_buffer;
        std::getline(ifs, read_buffer);
        auto cpos = read_buffer.find("format");
        if (cpos != std::string::npos) {
            // 取出osu版本
            auto vnum = read_buffer.substr(cpos + 8, read_buffer.size() - 1);
            file_format_version = std::stoi(vnum);
        } else {
            // 清除可能的 EOF 标志
            ifs.clear();
            // 回到文件开头
            ifs.seekg(0, std::ios::beg);
        }
        while (std::getline(ifs, read_buffer)) {
            if (!read_buffer.empty() && read_buffer.back() == '\r') {
                read_buffer.pop_back();
            }
            // XINFO(read_buffer);
            if (  // 未读到内容
                read_buffer.empty() ||
                // 直接结束
                read_buffer[0] == ';' ||
                // 注释
                (read_buffer[0] == '/' && read_buffer[1] == '/'))
                continue;
            osureader.parse_line(read_buffer);
        }
        // XINFO("parse result:");
        // for (const auto& [chapter, properties_map] :
        // osureader.map_properties) {
        //   if (chapter == "HitObjects" || chapter == "TimingPoints") {
        //     continue;
        //   }
        //   XINFO("-----------chapter:" + chapter + "---------------");
        //   for (const auto& [key, value] : properties_map) {
        //     XINFO("key:" + key + "-->value:" + value);
        //   }
        // }

        // 初始化map

        // general
        AudioFilename =
            osureader.get_value("General", "AudioFilename", std::string(""));
        // 初始化音频绝对路径
        // XINFO("map file parent:" + map_file_path.parent_path().string());
        // XINFO(AudioFilename);
        while (AudioFilename.starts_with(' ')) {
            AudioFilename.erase(0, 1);
        }
        audio_file_abs_path =
            std::filesystem::weakly_canonical(std::filesystem::absolute(
                map_file_path.parent_path() / AudioFilename));

        // XINFO("audio path:" + audio_file_abs_path.string());

        AudioLeadIn = osureader.get_value("General", "AudioLeadIn", 0);
        AudioHash =
            osureader.get_value("General", "AudioHash", std::string(""));
        PreviewTime = osureader.get_value("General", "PreviewTime", -1);
        Countdown = osureader.get_value("General", "Countdown", 1);
        sample_set = static_cast<SampleSet>(
            osureader.get_value("General", "SampleSet", 1));
        StackLeniency = osureader.get_value("General", "StackLeniency", 0.0);
        LetterboxInBreaks =
            osureader.get_value("General", "LetterboxInBreaks", false);
        StoryFireInFront =
            osureader.get_value("General", "StoryFireInFront", true);
        UseSkinSprites =
            osureader.get_value("General", "UseSkinSprites", false);
        AlwaysShowPlayfield =
            osureader.get_value("General", "AlwaysShowPlayfield", false);
        OverlayPosition = osureader.get_value("General", "OverlayPosition",
                                              std::string("NoChange"));
        SkinPreference =
            osureader.get_value("General", "SkinPreference", std::string(""));
        EpilepsyWarning =
            osureader.get_value("General", "EpilepsyWarning", false);
        CountdownOffset = osureader.get_value("General", "CountdownOffset", 0);
        SpecialStyle = osureader.get_value("General", "SpecialStyle", false);
        WidescreenStoryboard =
            osureader.get_value("General", "WidescreenStoryboard", false);
        SamplesMatchPlaybackRate =
            osureader.get_value("General", "SamplesMatchPlaybackRate", false);

        // editor
        // Bookmarks	逗号分隔的 Integer（整型）数组
        // 书签（蓝线）的位置（毫秒）
        auto marks =
            osureader.get_value("Editor", "Bookmarks", std::string(""));
        std::istringstream iss(marks);
        std::string token;
        while (std::getline(iss, token, ',')) {
            // 移除首尾空格
            token.erase(token.begin(),
                        std::find_if(token.begin(), token.end(),
                                     [](int ch) { return !std::isspace(ch); }));
            token.erase(std::find_if(token.rbegin(), token.rend(),
                                     [](int ch) { return !std::isspace(ch); })
                            .base(),
                        token.end());
            Bookmarks.push_back(std::stoi(token));
        }
        DistanceSpacing = osureader.get_value("Editor", "DistanceSpacing", 0.0);
        BeatDivisor = osureader.get_value("Editor", "BeatDivisor", 0);
        GridSize = osureader.get_value("Editor", "GridSize", 0);
        TimelineZoom = osureader.get_value("Editor", "TimelineZoom", 0.0);

        // metadata
        Title = osureader.get_value("Metadata", "Title", std::string(""));
        title = title;
        TitleUnicode =
            osureader.get_value("Metadata", "TitleUnicode", std::string(""));
        title_unicode = TitleUnicode;
        Artist = osureader.get_value("Metadata", "Artist", std::string(""));
        artist = Artist;
        ArtistUnicode =
            osureader.get_value("Metadata", "ArtistUnicode", std::string(""));
        artist_unicode = ArtistUnicode;
        Creator =
            osureader.get_value("Metadata", "Creator", std::string("mmm"));
        author = Creator;
        Version =
            osureader.get_value("Metadata", "Version", std::string("[mmm]"));
        version = Version;
        // XWARN("载入osu谱面Version:" + Version);
        Source = osureader.get_value("Metadata", "Source", std::string(""));
        // ***Tags	空格分隔的 String（字符串）数组	易于搜索的标签
        auto tags = osureader.get_value("Editor", "Tags", std::string(""));
        std::istringstream tiss(tags);
        while (std::getline(tiss, token, ' ')) {
            Tags.push_back(token);
        }
        BeatmapID = osureader.get_value("Metadata", "BeatmapID", -1);
        BeatmapSetID = osureader.get_value("Metadata", "BeatmapSetID", -1);

        // difficulty
        HPDrainRate = osureader.get_value("Difficulty", "HPDrainRate", 5.0);
        CircleSize = osureader.get_value("Difficulty", "CircleSize", 4.0);
        orbits = CircleSize;
        OverallDifficulty =
            osureader.get_value("Difficulty", "OverallDifficulty", 8.0);
        ApproachRate = osureader.get_value("Difficulty", "ApproachRate", 0.0);
        SliderMultiplier =
            osureader.get_value("Difficulty", "SliderMultiplier", 0.0);
        SliderTickRate =
            osureader.get_value("Difficulty", "SliderTickRate", 0.0);

        // 生成图名
        map_name = "[o!m] [" + std::to_string(int(CircleSize)) + "k]" + Version;

        // colour--- 不写

        // event
        // bg
        auto background_des = osureader.get_value(
            "Events", "background", std::string("0,0,\"bg.png\",0,0"));
        std::istringstream biss(background_des);
        std::vector<std::string> background_paras;
        while (std::getline(biss, token, ',')) {
            background_paras.emplace_back(token);
        }
        if (background_paras.at(0) == "0") {
            // 是图片
            background_type = 0;
        } else {
            // 是视频
            background_type = 1;
        }
        bg_file_name = background_paras.at(2);
        // 去引号
        if (bg_file_name.starts_with('\"')) {
            bg_file_name.replace(bg_file_name.begin(), bg_file_name.begin() + 1,
                                 "");
            bg_file_name.replace(bg_file_name.end() - 1, bg_file_name.end(),
                                 "");
        }
        bg_path = std::filesystem::path(map_file_path.parent_path().string() +
                                        "/" + bg_file_name);
        if (background_paras.size() >= 5) {
            bgxoffset = std::stoi(background_paras.at(3));
            bgyoffset = std::stoi(background_paras.at(4));
        } else {
            bgxoffset = 0;
            bgyoffset = 0;
        }

        // breaks
        for (int i = 0; i < osureader.current_breaks_index; i++) {
            auto breaks_des = osureader.get_value("Events", std::to_string(i),
                                                  std::string("2,0,0"));
            std::istringstream breaksiss(breaks_des);
            std::vector<std::string> breaks_paras;
            while (std::getline(breaksiss, token, ',')) {
                breaks_paras.emplace_back(token);
            }
            // 添加一个休息段
            breaks.emplace_back(std::stoi(breaks_paras.at(1)),
                                std::stoi(breaks_paras.at(2)));
        }

        // 创建物件
        for (int i = 0; i < osureader.current_hitobject_index; i++) {
            // 按顺序读取物件
            auto note_des =
                osureader.get_value("HitObjects", std::to_string(i),
                                    std::string("469,192,1846,1,0,0:0:0:0:"));
            std::istringstream noteiss(note_des);
            std::vector<std::string> note_paras;
            while (std::getline(noteiss, token, ',')) {
                note_paras.emplace_back(token);
            }
            std::shared_ptr<Note> osu_note;
            // 创建物件
            if (std::stoi(note_paras.at(3)) == 128) {
                osu_note = std::make_shared<OsuHold>();
                auto hold = std::dynamic_pointer_cast<OsuHold>(osu_note);
                // 使用读取出的参数初始化物件
                hold->from_osu_description(note_paras, CircleSize);
                // 创建一个面尾物件
                auto holdend = std::make_shared<OsuHoldEnd>(hold);
                holdend->is_hold_end = true;
                hitobjects.insert(holdend);
                // 设置面条物件的面尾引用
                hold->hold_end_reference = holdend;

                // 更新谱面时长
                if (holdend->timestamp > map_length)
                    map_length = holdend->timestamp;

                // 把长条物件加入缓存
                temp_hold_list.insert(hold);
            } else {
                osu_note = std::make_shared<OsuNote>();
                auto note = std::dynamic_pointer_cast<OsuNote>(osu_note);
                // 使用读取出的参数初始化物件
                note->from_osu_description(note_paras, CircleSize);
            }

            // 更新谱面时长
            if (osu_note->timestamp > map_length)
                map_length = osu_note->timestamp;

            // 加入物件列表
            hitobjects.insert(osu_note);
        }
        std::set<std::shared_ptr<Timing>, TimingComparator> basetimings;
        std::set<std::shared_ptr<Timing>, TimingComparator> notbasetimings;
        // 创建timing
        for (int i = 0; i < osureader.current_timing_index; i++) {
            // 按顺序读取timing点
            auto timing_point_des =
                osureader.get_value("TimingPoints", std::to_string(i),
                                    std::string("10000,333.33,4,0,0,100,1,1"));
            std::istringstream timingiss(timing_point_des);
            std::vector<std::string> timing_point_paras;
            while (std::getline(timingiss, token, ',')) {
                timing_point_paras.emplace_back(token);
            }
            // 创建timing
            auto osu_timing = std::make_shared<OsuTiming>();
            // 使用读取出的参数初始化timing
            osu_timing->from_osu_description(timing_point_paras);
            if (osu_timing->is_inherit_timing) {
                notbasetimings.insert(osu_timing);
            } else {
                basetimings.insert(osu_timing);
            }
        }

        MMap* ref = this;
        map_pool.enqueue_void([=]() {
            // 先添加全部基准timing--生成分拍
            for (auto begin = basetimings.begin(); begin != basetimings.end();
                 ++begin) {
                ref->insert_timing(*begin);
            }
            // 再倒序添加全部变速timing
            for (auto rbegin = notbasetimings.rbegin();
                 rbegin != notbasetimings.rend(); ++rbegin) {
                ref->insert_timing(*rbegin);
            }

            bool finded{false};
            // 读取全图参考bpm
            for (const auto& [time, timings] : ref->temp_timing_map) {
                // 使用第一个不带变速的绝对bpm
                if (timings.size() == 1 && timings[0]->is_base_timing) {
                    ref->preference_bpm = timings[0]->basebpm;
                    finded = true;
                    break;
                }
            }

            // 没找到单独存在的绝对时间点-找同时存在变速值为1.00的时间点
            if (!finded) {
                for (const auto& [time, timings] : ref->temp_timing_map) {
                    // 使用第一个不带变速的绝对bpm
                    if (timings.size() == 2 &&
                        std::fabs(timings[1]->bpm - 1.00) < 0.0001) {
                        ref->preference_bpm = timings[0]->basebpm;
                        finded = true;
                        break;
                    }
                }
            }

            // 再没找到就用第一个timing的绝对bpm-没有用200
            if (!finded) {
                if (ref->timings.empty()) {
                    ref->preference_bpm = 200;
                } else {
                    ref->preference_bpm = ref->timings.begin()->get()->basebpm;
                }
            }
        });

        // 填充元数据
        // general
        metadatas[MapMetadataType::OSU]->map_properties["AudioFilename"] =
            AudioFilename;
        metadatas[MapMetadataType::OSU]->map_properties["AudioLeadIn"] =
            AudioLeadIn;
        metadatas[MapMetadataType::OSU]->map_properties["AudioLeadHash"] =
            AudioHash;
        metadatas[MapMetadataType::OSU]->map_properties["PreviewTime"] =
            std::to_string(PreviewTime);
        metadatas[MapMetadataType::OSU]->map_properties["Countdown"] =
            std::to_string(Countdown);
        metadatas[MapMetadataType::OSU]->map_properties["SampleSet"] =
            std::to_string(static_cast<uint32_t>(sample_set));
        metadatas[MapMetadataType::OSU]->map_properties["StackLeniency"] =
            std::to_string(StackLeniency);
        metadatas[MapMetadataType::OSU]->map_properties["LetterboxInBreaks"] =
            std::to_string(int(LetterboxInBreaks));
        metadatas[MapMetadataType::OSU]->map_properties["StoryFireInFront"] =
            std::to_string(int(StoryFireInFront));
        metadatas[MapMetadataType::OSU]->map_properties["UseSkinSprites"] = "0";
        metadatas[MapMetadataType::OSU]->map_properties["AlwaysShowPlayfield"] =
            std::to_string(int(AlwaysShowPlayfield));
        metadatas[MapMetadataType::OSU]->map_properties["OverlayPosition"] =
            OverlayPosition;
        metadatas[MapMetadataType::OSU]->map_properties["SkinPreference"] =
            SkinPreference;
        metadatas[MapMetadataType::OSU]->map_properties["EpilepsyWarning"] =
            std::to_string(int(EpilepsyWarning));
        metadatas[MapMetadataType::OSU]->map_properties["CountdownOffset"] =
            std::to_string(CountdownOffset);
        metadatas[MapMetadataType::OSU]->map_properties["SpecialStyle"] =
            std::to_string(int(SpecialStyle));
        metadatas[MapMetadataType::OSU]
            ->map_properties["WidescreenStoryboard"] =
            std::to_string(WidescreenStoryboard);
        metadatas[MapMetadataType::OSU]
            ->map_properties["SamplesMatchPlaybackRate"] =
            std::to_string(int(SamplesMatchPlaybackRate));

        // editor
        std::stringstream sstream;
        for (const auto& val : Bookmarks) {
            sstream << val << ',';
        }
        auto Bookmarks_str = sstream.str();
        if (!Bookmarks_str.empty()) {
            Bookmarks_str.pop_back();
        }

        metadatas[MapMetadataType::OSU]->map_properties["Bookmarks"] =
            Bookmarks_str;
        metadatas[MapMetadataType::OSU]->map_properties["DistanceSpacing"] =
            std::to_string(DistanceSpacing);
        metadatas[MapMetadataType::OSU]->map_properties["BeatDivisor"] =
            std::to_string(BeatDivisor);
        metadatas[MapMetadataType::OSU]->map_properties["GridSize"] =
            std::to_string(GridSize);
        metadatas[MapMetadataType::OSU]->map_properties["TimelineZoom"] =
            std::to_string(TimelineZoom);

        // metadata
        metadatas[MapMetadataType::OSU]->map_properties["Title"] = Title;
        metadatas[MapMetadataType::OSU]->map_properties["TitleUnicode"] =
            TitleUnicode;
        metadatas[MapMetadataType::OSU]->map_properties["Artist"] = Artist;
        metadatas[MapMetadataType::OSU]->map_properties["ArtistUnicode"] =
            ArtistUnicode;
        metadatas[MapMetadataType::OSU]->map_properties["Creator"] = Creator;
        metadatas[MapMetadataType::OSU]->map_properties["Version"] = Version;
        metadatas[MapMetadataType::OSU]->map_properties["Source"] = Source;

        sstream.clear();
        for (const auto& tag : Tags) {
            sstream << tag << ' ';
        }
        auto Tags_str = sstream.str();
        if (!Tags_str.empty()) {
            Tags_str.pop_back();
        }
        metadatas[MapMetadataType::OSU]->map_properties["Tags"] = Tags_str;
        metadatas[MapMetadataType::OSU]->map_properties["BeatmapID"] =
            std::to_string(BeatmapID);
        metadatas[MapMetadataType::OSU]->map_properties["BeatmapSetID"] =
            std::to_string(BeatmapSetID);

        // difficulty
        metadatas[MapMetadataType::OSU]->map_properties["HPDrainRate"] =
            std::to_string(HPDrainRate);
        metadatas[MapMetadataType::OSU]->map_properties["CircleSize"] =
            std::to_string(CircleSize);
        metadatas[MapMetadataType::OSU]->map_properties["OverallDifficulty"] =
            std::to_string(OverallDifficulty);
        metadatas[MapMetadataType::OSU]->map_properties["ApproachRate"] =
            std::to_string(ApproachRate);
        metadatas[MapMetadataType::OSU]->map_properties["SliderMultiplier"] =
            std::to_string(SliderMultiplier);
        metadatas[MapMetadataType::OSU]->map_properties["SliderTickRate"] =
            std::to_string(SliderTickRate);

        // colour--- 不写

        // event
        // bg
        //
        /*
         *背景
         *背景语法：0,0,文件名,x 轴位置,y 轴位置
         *
         *文件名（字符串）：
         *背景图片在谱面文件夹内的文件名或者相对路径。若文件路径周围包含英文双引号，则也可被识别。
         *x 轴位置（整型） 和 y 轴位置（整型）：
         *以屏幕中心为原点的背景图片位置偏移值，单位是 osu! 像素。例如，50,100
         *表示这张背景图片在游玩时，需要移动至屏幕中心向右移动 50 osu!
         *像素，向下移动 100 osu! 像素显示。如果偏移值为 0,0，也可以忽略不写。
         *视频 视频语法：Video,开始时间,文件名,x 轴位置,y 轴位置
         *
         *Video 可用 1 代替。
         *
         *文件名（字符串）、x 轴位置（整型）、 y 轴位置（整型）
         *的效果与背景图片一致。
         */

        // 0为背景图片,1为视频--写出时可写Video或1
        // int32_t background_type{0};
        // 背景文件目录
        // std::string bg_file_name;
        // 背景的位置x偏移
        // int32_t bgxoffset;
        // 背景的位置y偏移
        // int32_t bgyoffset;
        if (background_type == 0) {
            // 图片
        } else {
            // 视频
        }
        metadatas[MapMetadataType::OSU]->map_properties["background"] =
            std::to_string(background_type) + ",0,\"" + bg_file_name + "\"," +
            std::to_string(bgxoffset) + "," + std::to_string(bgyoffset);

        // breaks

    } else {
        XWARN("非.osu格式,读取失败");
    }
}
