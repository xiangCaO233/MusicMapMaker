#include <QDebug>
#include <fstream>
#include <mmm/info/osu/OsuMapInfo.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/osu/OsuHold.hpp>
#include <mmm/obj/osu/OsuNote.hpp>

class OsuFileReader {
   public:
    // 构造OsuFileReader
    OsuFileReader();
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
    if (map_path.is_relative()) {
        map_path = std::filesystem::absolute(map_path);
    }
    auto fname = map_path.filename();
    qDebug() << "路径:" << map_path.string();
    if (map_path.extension() == ".osu") {
        // XINFO("load_osu:" + p.extension().string());
        std::ifstream ifs(map_path);
        if (!ifs.is_open()) {
            qDebug() << "打开文件[" << map_path.string() << "]失败";
            return;
        }
        // 创建osu元数据
        auto osumeta = std::make_shared<OsuMapMetadata>();
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

        main_audio_path =
            std::filesystem::weakly_canonical(std::filesystem::absolute(
                map_path.parent_path() / general->AudioFilename));
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

        if (sample_set_str == "None") {
            general->sample_set = SampleSet::NONE;
        } else if (sample_set_str == "Soft") {
            general->sample_set = SampleSet::SOFT;
        } else if (sample_set_str == "Normal") {
            general->sample_set = SampleSet::NORMAL;
        } else if (sample_set_str == "Drum") {
            general->sample_set = SampleSet::DRUM;
        } else {
            general->sample_set = SampleSet::NONE;
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
        // version = Version;
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
        // difficulty->orbits = CircleSize;
        difficulty->OverallDifficulty =
            osureader.get_value("Difficulty", "OverallDifficulty", 8.0);
        difficulty->ApproachRate =
            osureader.get_value("Difficulty", "ApproachRate", 0.0);
        difficulty->SliderMultiplier =
            osureader.get_value("Difficulty", "SliderMultiplier", 0.0);
        difficulty->SliderTickRate =
            osureader.get_value("Difficulty", "SliderTickRate", 0.0);

        // 生成图名
        // map_name = "[o!m] [" + std::to_string(int(CircleSize)) + "k]" +
        // Version;

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

        // bg_path = std::filesystem::path(map_file_path.parent_path().string()
        // +
        //                                 "/" + bg_file_name);
        // bg_rpath =
        //     std::filesystem::relative(bg_path, map_file_path.parent_path());
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

            std::shared_ptr<Note> osu_note;

            // 创建物件
            if (std::stoi(note_paras.at(3)) == 128) {
                auto hold = std::make_unique<OsuHold>();
                // 使用读取出的参数初始化物件
                hold->from_osu_description(note_paras, CircleSize);
                // 设置面条物件的面尾引用
                hold->hold_end_reference = holdend;

                // 更新谱面时长
                if (holdend->timestamp > map_length)
                    map_length = holdend->timestamp;

                // 把长条物件加入缓存
                note_set().add_note(hold);
            } else {
                osu_note = std::make_shared<OsuNote>();
                auto note = std::dynamic_pointer_cast<OsuNote>(osu_note);
                // 使用读取出的参数初始化物件
                note->from_osu_description(note_paras, difficulty->CircleSize);
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
        metadatas[MapMetadataType::MOSU]->map_properties["AudioFilename"] =
            AudioFilename;
        metadatas[MapMetadataType::MOSU]->map_properties["AudioLeadIn"] =
            AudioLeadIn;
        metadatas[MapMetadataType::MOSU]->map_properties["AudioLeadHash"] =
            AudioHash;
        metadatas[MapMetadataType::MOSU]->map_properties["PreviewTime"] =
            std::to_string(PreviewTime);
        metadatas[MapMetadataType::MOSU]->map_properties["Countdown"] =
            std::to_string(Countdown);
        metadatas[MapMetadataType::MOSU]->map_properties["SampleSet"] =
            std::to_string(static_cast<uint32_t>(sample_set));
        metadatas[MapMetadataType::MOSU]->map_properties["StackLeniency"] =
            std::to_string(StackLeniency);
        metadatas[MapMetadataType::MOSU]->map_properties["LetterboxInBreaks"] =
            std::to_string(int(LetterboxInBreaks));
        metadatas[MapMetadataType::MOSU]->map_properties["StoryFireInFront"] =
            std::to_string(int(StoryFireInFront));
        metadatas[MapMetadataType::MOSU]->map_properties["UseSkinSprites"] =
            "0";
        metadatas[MapMetadataType::MOSU]
            ->map_properties["AlwaysShowPlayfield"] =
            std::to_string(int(AlwaysShowPlayfield));
        metadatas[MapMetadataType::MOSU]->map_properties["OverlayPosition"] =
            OverlayPosition;
        metadatas[MapMetadataType::MOSU]->map_properties["SkinPreference"] =
            SkinPreference;
        metadatas[MapMetadataType::MOSU]->map_properties["EpilepsyWarning"] =
            std::to_string(int(EpilepsyWarning));
        metadatas[MapMetadataType::MOSU]->map_properties["CountdownOffset"] =
            std::to_string(CountdownOffset);
        metadatas[MapMetadataType::MOSU]->map_properties["SpecialStyle"] =
            std::to_string(int(SpecialStyle));
        metadatas[MapMetadataType::MOSU]
            ->map_properties["WidescreenStoryboard"] =
            std::to_string(WidescreenStoryboard);
        metadatas[MapMetadataType::MOSU]
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

        metadatas[MapMetadataType::MOSU]->map_properties["Bookmarks"] =
            Bookmarks_str;
        metadatas[MapMetadataType::MOSU]->map_properties["DistanceSpacing"] =
            std::to_string(DistanceSpacing);
        metadatas[MapMetadataType::MOSU]->map_properties["BeatDivisor"] =
            std::to_string(BeatDivisor);
        metadatas[MapMetadataType::MOSU]->map_properties["GridSize"] =
            std::to_string(GridSize);
        metadatas[MapMetadataType::MOSU]->map_properties["TimelineZoom"] =
            std::to_string(TimelineZoom);

        // metadata
        metadatas[MapMetadataType::MOSU]->map_properties["Title"] = Title;
        metadatas[MapMetadataType::MOSU]->map_properties["TitleUnicode"] =
            TitleUnicode;
        metadatas[MapMetadataType::MOSU]->map_properties["Artist"] = Artist;
        metadatas[MapMetadataType::MOSU]->map_properties["ArtistUnicode"] =
            ArtistUnicode;
        metadatas[MapMetadataType::MOSU]->map_properties["Creator"] = Creator;
        metadatas[MapMetadataType::MOSU]->map_properties["Version"] = Version;
        metadatas[MapMetadataType::MOSU]->map_properties["Source"] = Source;

        sstream.clear();
        for (const auto& tag : Tags) {
            sstream << tag << ' ';
        }
        auto Tags_str = sstream.str();
        if (!Tags_str.empty()) {
            Tags_str.pop_back();
        }
        metadatas[MapMetadataType::MOSU]->map_properties["Tags"] = Tags_str;
        metadatas[MapMetadataType::MOSU]->map_properties["BeatmapID"] =
            std::to_string(BeatmapID);
        metadatas[MapMetadataType::MOSU]->map_properties["BeatmapSetID"] =
            std::to_string(BeatmapSetID);

        // difficulty
        metadatas[MapMetadataType::MOSU]->map_properties["HPDrainRate"] =
            std::to_string(HPDrainRate);
        metadatas[MapMetadataType::MOSU]->map_properties["CircleSize"] =
            std::to_string(CircleSize);
        metadatas[MapMetadataType::MOSU]->map_properties["OverallDifficulty"] =
            std::to_string(OverallDifficulty);
        metadatas[MapMetadataType::MOSU]->map_properties["ApproachRate"] =
            std::to_string(ApproachRate);
        metadatas[MapMetadataType::MOSU]->map_properties["SliderMultiplier"] =
            std::to_string(SliderMultiplier);
        metadatas[MapMetadataType::MOSU]->map_properties["SliderTickRate"] =
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
        metadatas[MapMetadataType::MOSU]->map_properties["background"] =
            std::to_string(background_type) + ",0,\"" + bg_file_name + "\"," +
            std::to_string(bgxoffset) + "," + std::to_string(bgyoffset);

        // breaks

    } else {
        XWARN("非.osu格式,读取失败");
    }
}
