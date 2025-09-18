#include <QDebug>
#include <fstream>
#include <mmm/info/osu/OsuMapInfo.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/osu/OsuHold.hpp>
#include <mmm/obj/osu/OsuNote.hpp>
#include <mmm/timing/osu/OsuTiming.hpp>

class OsuFileReader {
   public:
    // 构造OsuFileReader
    OsuFileReader() = default;
    // 析构OsuFileReader
    ~OsuFileReader() = default;

    // 格式化过的属性
    std::unordered_map<std::string,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>,
                       StringHash, std::equal_to<>>
        map_properties;

    // 当前索引
    uint32_t current_timing_index{0};
    uint32_t current_hitobject_index{0};
    uint32_t current_breaks_index{0};

    // 当前章节
    std::string current_chapter;

    // 格式化行
    void parse_line(const std::string& line) {
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
                                   std::to_string(current_breaks_index++)] =
                                      line;
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
                              [std::to_string(current_hitobject_index++)] =
                                  line;
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

    // 获取数据
    template <typename T>
    T get_value(const std::string& chapter, const std::string& key,
                T default_value = T()) {
        auto chapter_it = map_properties.find(chapter);
        if (chapter_it == map_properties.end()) return default_value;

        auto key_it = chapter_it->second.find(key);
        if (key_it == chapter_it->second.end()) return default_value;

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

void MMap::readOsu() {
    // 切换为绝对路径
    if (basemeta.map_path.is_relative()) {
        basemeta.map_path = std::filesystem::absolute(basemeta.map_path);
    }
    auto fname = basemeta.map_path.filename();
    qDebug() << "路径:" << basemeta.map_path.string();
    if (basemeta.map_path.extension() == ".osu") {
        // XINFO("load_osu:" + p.extension().string());
        std::ifstream ifs(basemeta.map_path);
        if (!ifs.is_open()) {
            qDebug() << "打开文件[" << basemeta.map_path.string() << "]失败";
            return;
        }
        // 创建osu元数据
        auto osumeta = std::make_shared<OsuMapMetadata>();
        metadatas[MapMetadataType::OSU] = osumeta;
        OsuFileReader osureader;
        std::string read_buffer;
        std::getline(ifs, read_buffer);
        auto cpos = read_buffer.find("format");
        if (cpos != std::string::npos) {
            // 取出osu版本
            auto vnum = read_buffer.substr(cpos + 8, read_buffer.size() - 1);
            osumeta->file_format_version = std::stoi(vnum);
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
        auto general = osumeta->get_chapter<OsuMapChapterGeneral>("General");

        // general
        general->AudioFilename =
            osureader.get_value("General", "AudioFilename", std::string(""));
        // 初始化音频绝对路径
        // XINFO("map file parent:" + map_file_path.parent_path().string());
        // XINFO(AudioFilename);
        while (general->AudioFilename.starts_with(' ')) {
            general->AudioFilename.erase(0, 1);
        }

        basemeta.main_audio_path =
            std::filesystem::weakly_canonical(std::filesystem::absolute(
                basemeta.map_path.parent_path() / general->AudioFilename));
        // audio_file_rpath = std::filesystem::relative(
        //     audio_file_abs_path, map_file_path.parent_path());

        // XINFO("audio path:" + audio_file_abs_path.string());

        general->AudioLeadIn = osureader.get_value("General", "AudioLeadIn", 0);
        general->AudioHash =
            osureader.get_value("General", "AudioHash", std::string(""));
        general->PreviewTime =
            osureader.get_value("General", "PreviewTime", -1);
        general->Countdown = osureader.get_value("General", "Countdown", 1);

        auto sample_set_str =
            osureader.get_value("General", "SampleSet", std::string("None"));
        while (sample_set_str.starts_with(' ')) {
            sample_set_str.erase(0, 1);
        }

        using enum SampleSet;
        if (sample_set_str == "None") {
            general->sample_set = NONE;
        } else if (sample_set_str == "Soft") {
            general->sample_set = SOFT;
        } else if (sample_set_str == "Normal") {
            general->sample_set = NORMAL;
        } else if (sample_set_str == "Drum") {
            general->sample_set = DRUM;
        } else {
            general->sample_set = NONE;
        }

        general->StackLeniency =
            osureader.get_value("General", "StackLeniency", 0.0);
        general->Mode = osureader.get_value("General", "Mode", 0);
        general->LetterboxInBreaks =
            osureader.get_value("General", "LetterboxInBreaks", false);
        general->StoryFireInFront =
            osureader.get_value("General", "StoryFireInFront", true);
        general->UseSkinSprites =
            osureader.get_value("General", "UseSkinSprites", false);
        general->AlwaysShowPlayfield =
            osureader.get_value("General", "AlwaysShowPlayfield", false);
        general->OverlayPosition = osureader.get_value(
            "General", "OverlayPosition", std::string("NoChange"));
        general->SkinPreference =
            osureader.get_value("General", "SkinPreference", std::string(""));
        general->EpilepsyWarning =
            osureader.get_value("General", "EpilepsyWarning", false);
        general->CountdownOffset =
            osureader.get_value("General", "CountdownOffset", 0);
        general->SpecialStyle =
            osureader.get_value("General", "SpecialStyle", false);
        general->WidescreenStoryboard =
            osureader.get_value("General", "WidescreenStoryboard", false);
        general->SamplesMatchPlaybackRate =
            osureader.get_value("General", "SamplesMatchPlaybackRate", false);

        // editor
        auto editor = osumeta->get_chapter<OsuMapChapterEditor>("Editor");
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
            if (token != "") {
                editor->Bookmarks.push_back(std::stoi(token));
            }
        }
        editor->DistanceSpacing =
            osureader.get_value("Editor", "DistanceSpacing", 0.0);
        editor->BeatDivisor = osureader.get_value("Editor", "BeatDivisor", 0);
        editor->GridSize = osureader.get_value("Editor", "GridSize", 0);
        editor->TimelineZoom =
            osureader.get_value("Editor", "TimelineZoom", 0.0);

        // metadata
        auto metadata = osumeta->get_chapter<OsuMapChapterMetadata>("Metadata");
        metadata->Title =
            osureader.get_value("Metadata", "Title", std::string(""));
        metadata->TitleUnicode =
            osureader.get_value("Metadata", "TitleUnicode", std::string(""));
        metadata->Artist =
            osureader.get_value("Metadata", "Artist", std::string(""));
        // artist = Artist;
        metadata->ArtistUnicode =
            osureader.get_value("Metadata", "ArtistUnicode", std::string(""));
        // artist_unicode = ArtistUnicode;
        metadata->Creator =
            osureader.get_value("Metadata", "Creator", std::string("mmm"));
        // author = Creator;
        metadata->Version =
            osureader.get_value("Metadata", "Version", std::string("[mmm]"));
        basemeta.version = metadata->Version;
        // XWARN("载入osu谱面Version:" + Version);
        metadata->Source =
            osureader.get_value("Metadata", "Source", std::string(""));
        // ***Tags	空格分隔的 String（字符串）数组	易于搜索的标签
        auto tags = osureader.get_value("Editor", "Tags", std::string(""));
        std::istringstream tiss(tags);
        while (std::getline(tiss, token, ' ')) {
            metadata->Tags.push_back(token);
        }
        metadata->BeatmapID = osureader.get_value("Metadata", "BeatmapID", -1);
        metadata->BeatmapSetID =
            osureader.get_value("Metadata", "BeatmapSetID", -1);

        // difficulty
        auto difficulty =
            osumeta->get_chapter<OsuMapChapterDifficulty>("Difficulty");
        difficulty->HPDrainRate =
            osureader.get_value("Difficulty", "HPDrainRate", 5.0);
        difficulty->CircleSize =
            osureader.get_value("Difficulty", "CircleSize", 4.0);

        basemeta.track_count = difficulty->CircleSize;

        difficulty->OverallDifficulty =
            osureader.get_value("Difficulty", "OverallDifficulty", 8.0);
        difficulty->ApproachRate =
            osureader.get_value("Difficulty", "ApproachRate", 0.0);
        difficulty->SliderMultiplier =
            osureader.get_value("Difficulty", "SliderMultiplier", 0.0);
        difficulty->SliderTickRate =
            osureader.get_value("Difficulty", "SliderTickRate", 0.0);

        // 生成图名
        basemeta.name = "[o!m] [" +
                        std::to_string(int(difficulty->CircleSize)) + "k] " +
                        metadata->Version;

        // colour--- 不写

        // event
        auto event = osumeta->get_chapter<OsuMapChapterEvents>("Events");
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
            event->background_type = 0;
        } else {
            // 是视频
            event->background_type = 1;
        }
        event->video_starttime = std::stoi(background_paras.at(1));
        event->bg_file_name = background_paras.at(2);
        // 去引号
        if (event->bg_file_name.starts_with('\"')) {
            event->bg_file_name.replace(event->bg_file_name.begin(),
                                        event->bg_file_name.begin() + 1, "");
            event->bg_file_name.replace(event->bg_file_name.end() - 1,
                                        event->bg_file_name.end(), "");
        }

        basemeta.main_cover_path = std::filesystem::absolute(
            basemeta.map_path.parent_path() / event->bg_file_name);
        if (background_paras.size() >= 5) {
            event->bgxoffset = std::stoi(background_paras.at(3));
            event->bgyoffset = std::stoi(background_paras.at(4));
        } else {
            event->bgxoffset = 0;
            event->bgyoffset = 0;
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
            event->breaks.emplace_back(std::stoi(breaks_paras.at(1)),
                                       std::stoi(breaks_paras.at(2)));
        }

        // 读取创建物件
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

            // 创建物件
            if (std::stoi(note_paras.at(3)) == 128) {
                auto hold = std::make_unique<OsuHold>(this);
                // 使用读取出的参数初始化物件
                hold->from_osu_description(note_paras, difficulty->CircleSize);
                // 更新谱面时长
                if (hold->timestamp() + hold->duration() > basemeta.map_length)
                    basemeta.map_length = hold->timestamp() + hold->duration();
                // 把长条物件加入缓存
                auto handle = note_set().add_note(std::move(hold));
                noteUUIDManager.register_new_note(handle);
            } else {
                auto note = std::make_unique<OsuNote>(this);
                // 使用读取出的参数初始化物件
                note->from_osu_description(note_paras, difficulty->CircleSize);
                // 更新谱面时长
                if (note->timestamp() > basemeta.map_length)
                    basemeta.map_length = note->timestamp();

                // 加入物件列表
                auto handle = note_set().add_note(std::move(note));
                noteUUIDManager.register_new_note(handle);
            }
        }

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
            auto osu_timing = std::make_unique<OsuTiming>();
            // 使用读取出的参数初始化timing
            osu_timing->from_osu_description(timing_point_paras);
            // 添加到timing表
            timing_set().add_timing_point(std::move(osu_timing));
        }

        bool finded{false};
        // 读取全图参考bpm
        for (const auto& [time, timings] :
             timing_set().get_all_timing_points()) {
            for (const auto& timing : timings) {
                // 使用第一个不带变速的绝对bpm
                if (timing->is_base_timing) {
                    basemeta.preference_bpm = timing->bpm;
                    finded = true;
                    break;
                }
            }
            if (finded) {
                break;
            }
        }

        // 再没找到就用第一个timing的绝对bpm-没有用200
        if (!finded) {
            if (timing_set().get_all_timing_points().empty()) {
                basemeta.preference_bpm = 200;
            } else {
                basemeta.preference_bpm = timing_set()
                                              .get_all_timing_points()
                                              .begin()
                                              ->second.begin()
                                              ->get()
                                              ->bpm;
            }
        }

        // 最后生成全部拍
        analyzeBeatInfo();

        std::map<uint32_t, Beat> sorted_beats(beatInfo.begin(), beatInfo.end());
        for (const auto& pair : sorted_beats) {
            const Beat& b = pair.second;
            qDebug() << "Beat at " << b.beat_start << "ms "
                     << "(length: " << b.beat_length << "ms): "
                     << "Best division found -> 1/" << b.divisors;
        }

        // 填充元数据
        // general
        using enum MapMetadataType;
        metadatas[OSU]->map_properties[OSU]["AudioFilename"] =
            general->AudioFilename;
        metadatas[OSU]->map_properties[OSU]["AudioLeadIn"] =
            general->AudioLeadIn;
        metadatas[OSU]->map_properties[OSU]["AudioLeadHash"] =
            general->AudioHash;
        metadatas[OSU]->map_properties[OSU]["PreviewTime"] =
            std::to_string(general->PreviewTime);
        metadatas[OSU]->map_properties[OSU]["Countdown"] =
            std::to_string(general->Countdown);
        metadatas[OSU]->map_properties[OSU]["SampleSet"] =
            std::to_string(static_cast<uint32_t>(general->sample_set));
        metadatas[OSU]->map_properties[OSU]["StackLeniency"] =
            std::to_string(general->StackLeniency);
        metadatas[OSU]->map_properties[OSU]["LetterboxInBreaks"] =
            std::to_string(int(general->LetterboxInBreaks));
        metadatas[OSU]->map_properties[OSU]["StoryFireInFront"] =
            std::to_string(int(general->StoryFireInFront));
        metadatas[OSU]->map_properties[OSU]["UseSkinSprites"] = "0";
        metadatas[OSU]->map_properties[OSU]["AlwaysShowPlayfield"] =
            std::to_string(int(general->AlwaysShowPlayfield));
        metadatas[OSU]->map_properties[OSU]["OverlayPosition"] =
            general->OverlayPosition;
        metadatas[OSU]->map_properties[OSU]["SkinPreference"] =
            general->SkinPreference;
        metadatas[OSU]->map_properties[OSU]["EpilepsyWarning"] =
            std::to_string(int(general->EpilepsyWarning));
        metadatas[OSU]->map_properties[OSU]["CountdownOffset"] =
            std::to_string(general->CountdownOffset);
        metadatas[OSU]->map_properties[OSU]["SpecialStyle"] =
            std::to_string(int(general->SpecialStyle));
        metadatas[OSU]->map_properties[OSU]["WidescreenStoryboard"] =
            std::to_string(general->WidescreenStoryboard);
        metadatas[OSU]->map_properties[OSU]["SamplesMatchPlaybackRate"] =
            std::to_string(int(general->SamplesMatchPlaybackRate));

        // editor
        std::stringstream sstream;
        for (const auto& val : editor->Bookmarks) {
            sstream << val << ',';
        }
        auto Bookmarks_str = sstream.str();
        if (!Bookmarks_str.empty()) {
            Bookmarks_str.pop_back();
        }

        metadatas[OSU]->map_properties[OSU]["Bookmarks"] = Bookmarks_str;
        metadatas[OSU]->map_properties[OSU]["DistanceSpacing"] =
            std::to_string(editor->DistanceSpacing);
        metadatas[OSU]->map_properties[OSU]["BeatDivisor"] =
            std::to_string(editor->BeatDivisor);
        metadatas[OSU]->map_properties[OSU]["GridSize"] =
            std::to_string(editor->GridSize);
        metadatas[OSU]->map_properties[OSU]["TimelineZoom"] =
            std::to_string(editor->TimelineZoom);

        // metadata
        metadatas[OSU]->map_properties[OSU]["Title"] = metadata->Title;
        metadatas[OSU]->map_properties[OSU]["TitleUnicode"] =
            metadata->TitleUnicode;
        metadatas[OSU]->map_properties[OSU]["Artist"] = metadata->Artist;
        metadatas[OSU]->map_properties[OSU]["ArtistUnicode"] =
            metadata->ArtistUnicode;
        metadatas[OSU]->map_properties[OSU]["Creator"] = metadata->Creator;
        metadatas[OSU]->map_properties[OSU]["Version"] = metadata->Version;
        metadatas[OSU]->map_properties[OSU]["Source"] = metadata->Source;

        sstream.clear();
        for (const auto& tag : metadata->Tags) {
            sstream << tag << ' ';
        }
        auto Tags_str = sstream.str();
        if (!Tags_str.empty()) {
            Tags_str.pop_back();
        }
        metadatas[OSU]->map_properties[OSU]["Tags"] = Tags_str;
        metadatas[OSU]->map_properties[OSU]["BeatmapID"] =
            std::to_string(metadata->BeatmapID);
        metadatas[OSU]->map_properties[OSU]["BeatmapSetID"] =
            std::to_string(metadata->BeatmapSetID);

        // difficulty
        metadatas[OSU]->map_properties[OSU]["HPDrainRate"] =
            std::to_string(difficulty->HPDrainRate);
        metadatas[OSU]->map_properties[OSU]["CircleSize"] =
            std::to_string(difficulty->CircleSize);
        metadatas[OSU]->map_properties[OSU]["OverallDifficulty"] =
            std::to_string(difficulty->OverallDifficulty);
        metadatas[OSU]->map_properties[OSU]["ApproachRate"] =
            std::to_string(difficulty->ApproachRate);
        metadatas[OSU]->map_properties[OSU]["SliderMultiplier"] =
            std::to_string(difficulty->SliderMultiplier);
        metadatas[OSU]->map_properties[OSU]["SliderTickRate"] =
            std::to_string(difficulty->SliderTickRate);

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
        if (event->background_type == 0) {
            // 图片
        } else {
            // 视频
        }
        metadatas[OSU]->map_properties[OSU]["background"] =
            std::to_string(event->background_type) + ",0,\"" +
            event->bg_file_name + "\"," + std::to_string(event->bgxoffset) +
            "," + std::to_string(event->bgyoffset);

        // breaks

    } else {
        qDebug() << "非.osu格式,读取失败";
    }
}
