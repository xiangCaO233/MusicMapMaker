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
#include "mmm/hitobject/Note/rm/Slide.h"
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

OsuMap::OsuMap() {
    maptype = MapType::OSUMAP;
    // 注册元数据
    register_metadata(MapMetadataType::MOSU);
}
// 通过父类构造
OsuMap::OsuMap(std::shared_ptr<MMap> srcmap) {
    // 从父类复制所有基础数据
    map_name = srcmap->map_name;
    title = srcmap->title;
    title_unicode = srcmap->title_unicode;
    artist = srcmap->artist;
    artist_unicode = srcmap->artist_unicode;
    author = srcmap->author;
    version = srcmap->version;

    map_file_path = srcmap->map_file_path;
    audio_file_abs_path = srcmap->audio_file_abs_path;
    bg_path = srcmap->bg_path;

    project_reference = srcmap->project_reference;
    maptype = srcmap->maptype;
    preference_bpm = srcmap->preference_bpm;
    map_length = srcmap->map_length;
    orbits = srcmap->orbits;

    metadatas = srcmap->metadatas;

    {
        std::lock_guard<std::mutex> lock(srcmap->hitobjects_mutex);
        hitobjects = srcmap->hitobjects;
        temp_hold_list = srcmap->temp_hold_list;
    }

    timings = srcmap->timings;
    temp_timing_map = srcmap->temp_timing_map;
    beats = srcmap->beats;
    audio_pos_callback = srcmap->audio_pos_callback;

    // 查找元数据表-不存在则创建
    auto meta_it = metadatas.find(MapMetadataType::MOSU);
    if (meta_it == metadatas.end()) {
        metadatas[MapMetadataType::MOSU] = default_metadata();
    }

    // General 章节
    AudioFilename =
        metadatas[MapMetadataType::MOSU]->map_properties["AudioFilename"];
    AudioLeadIn = std::stoi(
        metadatas[MapMetadataType::MOSU]->map_properties["AudioLeadIn"]);
    PreviewTime = std::stoi(
        metadatas[MapMetadataType::MOSU]->map_properties["PreviewTime"]);
    Countdown = std::stoi(
        metadatas[MapMetadataType::MOSU]->map_properties["Countdown"]);
    sample_set = static_cast<SampleSet>(std::stoi(
        metadatas[MapMetadataType::MOSU]->map_properties["SampleSet"]));
    StackLeniency = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["StackLeniency"]);
    LetterboxInBreaks =
        std::stoi(metadatas[MapMetadataType::MOSU]
                      ->map_properties["LetterboxInBreaks"]) != 0;
    UseSkinSprites = std::stoi(metadatas[MapMetadataType::MOSU]
                                   ->map_properties["UseSkinSprites"]) != 0;
    OverlayPosition =
        metadatas[MapMetadataType::MOSU]->map_properties["OverlayPosition"];
    EpilepsyWarning = std::stoi(metadatas[MapMetadataType::MOSU]
                                    ->map_properties["EpilepsyWarning"]) != 0;
    CountdownOffset = std::stoi(
        metadatas[MapMetadataType::MOSU]->map_properties["CountdownOffset"]);
    SpecialStyle =
        std::stoi(
            metadatas[MapMetadataType::MOSU]->map_properties["SpecialStyle"]) !=
        0;
    WidescreenStoryboard =
        std::stoi(metadatas[MapMetadataType::MOSU]
                      ->map_properties["WidescreenStoryboard"]) != 0;
    SamplesMatchPlaybackRate =
        std::stoi(metadatas[MapMetadataType::MOSU]
                      ->map_properties["SamplesMatchPlaybackRate"]) != 0;

    // Editor 章节
    DistanceSpacing = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["DistanceSpacing"]);
    BeatDivisor = std::stoi(
        metadatas[MapMetadataType::MOSU]->map_properties["BeatDivisor"]);
    GridSize =
        std::stoi(metadatas[MapMetadataType::MOSU]->map_properties["GridSize"]);
    TimelineZoom = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["TimelineZoom"]);

    // Metadata 章节
    Title = metadatas[MapMetadataType::MOSU]->map_properties["Title"];
    TitleUnicode =
        metadatas[MapMetadataType::MOSU]->map_properties["TitleUnicode"];
    Artist = metadatas[MapMetadataType::MOSU]->map_properties["Artist"];
    ArtistUnicode =
        metadatas[MapMetadataType::MOSU]->map_properties["ArtistUnicode"];
    Creator = metadatas[MapMetadataType::MOSU]->map_properties["Creator"];
    Version = metadatas[MapMetadataType::MOSU]->map_properties["Version"];
    Source = metadatas[MapMetadataType::MOSU]->map_properties["Source"];
    BeatmapID = std::stoi(
        metadatas[MapMetadataType::MOSU]->map_properties["BeatmapID"]);
    BeatmapSetID = std::stoi(
        metadatas[MapMetadataType::MOSU]->map_properties["BeatmapSetID"]);

    // Difficulty 章节
    HPDrainRate = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["HPDrainRate"]);
    CircleSize = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["CircleSize"]);
    OverallDifficulty = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["OverallDifficulty"]);
    ApproachRate = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["ApproachRate"]);
    SliderMultiplier = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["SliderMultiplier"]);
    SliderTickRate = std::stod(
        metadatas[MapMetadataType::MOSU]->map_properties["SliderTickRate"]);

    // 如果父类有相关数据，覆盖默认值
    if (!srcmap->title.empty()) Title = srcmap->title;
    if (!srcmap->title_unicode.empty()) TitleUnicode = srcmap->title_unicode;
    if (!srcmap->artist.empty()) Artist = srcmap->artist;
    if (!srcmap->artist_unicode.empty()) ArtistUnicode = srcmap->artist_unicode;
    if (!srcmap->author.empty()) Creator = srcmap->author;
    if (!srcmap->version.empty()) Version = srcmap->version;

    // 处理背景和音频路径
    if (!srcmap->bg_path.empty()) {
        bg_file_name = srcmap->bg_path.filename().string();
        // 默认背景位置 (0,0)
        bgxoffset = 0;
        bgyoffset = 0;
    }

    if (!srcmap->audio_file_abs_path.empty()) {
        AudioFilename = srcmap->audio_file_abs_path.filename().string();
    }

    // 设置文件格式版本
    file_format_version = 14;
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

// 写出到文件
void OsuMap::write_to_file(const char* path) {
    auto p = std::filesystem::path(path);
    if (mutil::endsWithExtension(p, ".mmm")) {
        MMap::write_to_file(path);
        return;
    }

    std::string res;
    std::string slashn;
#ifdef _WIN32
    slashn = "\r\n";
#else
    slashn = "\n";
#endif  //_WIN32

    if (!is_write_file_legal(path, res)) {
        XWARN("写出路径不合法,已修正到项目路径");
    }
    auto& meta = metadatas[MapMetadataType::MOSU]->map_properties;
    std::ofstream os(res);
    // 写出文件
    os << "osu file format v" << file_format_version << slashn << slashn;
    // 章节	描述	内容类型
    // [General]	谱面的总体信息	键: 值 对
    os << "[General]" << slashn;
    /*
     ***AudioFilename	String（字符串）	音频文件的相对路径 默认无
     */
    // std::string AudioFilename{""};
    os << "AudioFilename: " << AudioFilename << slashn;

    /*
     ***AudioLeadIn	Integer（整型）	音频文件播放之前预留的空白时间	默认0
     */
    // int32_t AudioLeadIn{0};
    os << "AudioLeadIn: " << AudioLeadIn << slashn;

    /*
     ***AudioHash	String（字符串）	已弃用
     */
    // [[deprecated]] std::string AudioHash{""};
    // os << "AudioHash: " << AudioHash << slashn;

    /*
     ***PreviewTime	Integer（整型）	在选中谱面时的歌曲预览点位置（毫秒）-1
     */
    // int32_t PreviewTime{-1};
    os << "PreviewTime: " << PreviewTime << slashn;

    /*
     ***Countdown	Integer（整型）	在第一个物件之前出现的倒计时速度 (0 =
     *无倒计时, 1 = 正常速度, 2 = 一半速度, 3 = 二倍速度)	默认1
     */
    // int32_t Countdown{1};
    os << "Countdown: " << Countdown << slashn;

    /*
     ***SampleSet	String（字符串）
     *当时间点（红线、绿线）未覆盖设置时的默认音效组（Normal、Soft、Drum）
     *默认Normal
     */
    // SampleSet sample_set{SampleSet::NORMAL};
    std::string sample_name;
    switch (sample_set) {
        case SampleSet::DRUM: {
            sample_name = "Drum";
            break;
        }
        case SampleSet::SOFT: {
            sample_name = "Soft";
            break;
        }
        case SampleSet::NORMAL: {
            sample_name = "Normal";
            break;
        }
        case SampleSet::NONE: {
            sample_name = "None";
            break;
        }
    }
    os << "SampleSet: " << sample_name << slashn;

    /*
     ***StackLeniency	Decimal（精准小数）
     *当物件重叠在同一个位置时，决定物件之间是否出现堆叠效果的阈值（0-1）
     *0.7 Mode Integer（整型）	游戏模式（0 = osu!、1 = osu!taiko、2 =
     *osu!catch、3 = osu!mania）	默认0
     */
    // double StackLeniency{0.0};
    os << "StackLeniency: " << StackLeniency << slashn;

    /*
    ***Mode	Integer（整型）
    *游戏模式（0 = osu!、1 = osu!taiko、2 = osu!catch、3 = osu!mania） 默认0
    */
    // int32_t Mode{0};
    os << "Mode: " << Mode << slashn;

    /*
     ***LetterboxInBreaks
     *Boolean（布尔值）是否开启谱面休息段使用黑边填充设置 默认0
     */
    // bool LetterboxInBreaks{false};
    os << "LetterboxInBreaks: " << LetterboxInBreaks << slashn;

    /*
     ***StoryFireInFront Boolean（布尔值）	已弃用	默认1
     */
    // [[deprecated]] bool StoryFireInFront{true};
    // os << "StoryFireInFront" << (StoryFireInFront ? 1 : 0) << slashn;

    /*
     ***UseSkinSprites Boolean（布尔值）是否允许故事板使用玩家皮肤元素	默认0
     */
    // bool UseSkinSprites{false};
    if (UseSkinSprites) {
        os << "UseSkinSprites: " << 1 << slashn;
    }

    /*
     ***AlwaysShowPlayfield Boolean（布尔值） 已弃用	默认0
     */
    // [[deprecated]] bool AlwaysShowPlayfield{false};
    // os << "AlwaysShowPlayfield" << (AlwaysShowPlayfield? 1 : 0) << slashn;

    /*
     ***OverlayPosition String（字符串）
     *设置物件皮肤覆盖层与数字层之间的关系（NoChange = 使用玩家皮肤设定， Below
     *= 覆盖层绘制于数字之下，Above = 覆盖层绘制于数字之上）	默认NoChange
     */
    // std::string OverlayPosition{"NoChange"};
    if (OverlayPosition != "NoChange")
        os << "OverlayPosition: " << OverlayPosition << slashn;

    /*
     ***SkinPreference String（字符串） 推荐在游玩时使用的皮肤名称 默认无
     */
    // std::string SkinPreference{""};
    if (SkinPreference != "")
        os << "SkinPreference: " << SkinPreference << slashn;

    /*
     ***EpilepsyWarning Boolean（布尔值） 是否开启谱面闪烁（癫痫）警告	默认0
     */
    // bool EpilepsyWarning{false};
    if (EpilepsyWarning) os << "EpilepsyWarning: " << 1 << slashn;

    /*
     ***CountdownOffset Integer（整型）
     *谱面第一个物件之前的倒计时的偏移值（拍子）默认0
     */
    // int32_t CountdownOffset{0};
    if (CountdownOffset != 0)
        os << "CountdownOffset: " << CountdownOffset << slashn;

    /*
     ***SpecialStyle Boolean（布尔值） 是否在 osu!mania 谱面中启用 BMS 风格（N+1
     *键）的键位设置 默认0
     */
    // bool SpecialStyle{false};
    os << "SpecialStyle: " << (SpecialStyle ? 1 : 0) << slashn;

    /*
     ***WidescreenStoryboard	Boolean（布尔值）是否开启故事板的宽屏显示 默认0
     */
    // bool WidescreenStoryboard{false};
    os << "WidescreenStoryboard: " << (WidescreenStoryboard ? 1 : 0) << slashn;

    /*
     ***SamplesMatchPlaybackRate
     *Boolean（布尔值）是否允许当变速类型模组开启时，改变音效的播放速率	默认0
     */
    // bool SamplesMatchPlaybackRate{false};
    if (SamplesMatchPlaybackRate)
        os << "SamplesMatchPlaybackRate: " << 1 << slashn;

    os << slashn;

    // [Editor]	可在谱面编辑器内显示的信息	键: 值 对
    os << "[Editor]" << slashn;
    /*
     *Bookmarks	逗号分隔的 Integer（整型）数组
     *书签（蓝线）的位置（毫秒）
     */
    // std::vector<int32_t> Bookmarks;
    if (!Bookmarks.empty()) {
        std::string bookmarkstr = "";
        for (const auto& bookmark : Bookmarks) {
            bookmarkstr.append(std::to_string(bookmark));
            bookmarkstr.push_back(',');
        }
        bookmarkstr.pop_back();
        os << "Bookmarks: " << bookmarkstr << slashn;
    }

    /*
     ***DistanceSpacing	Decimal（精准小数）	间距锁定倍率
     */
    // double DistanceSpacing;
    os << "DistanceSpacing: " << DistanceSpacing << slashn;

    /*
     ***BeatDivisor	Integer（整型）	节拍细分
     */
    // int32_t BeatDivisor;
    os << "BeatDivisor: " << BeatDivisor << slashn;

    /*
     ***GridSize	Integer（整型）	网格大小
     */
    // int32_t GridSize;
    os << "GridSize: " << GridSize << slashn;

    /*
     ***TimelineZoom	Decimal（精准小数）	物件时间轴的缩放倍率
     */
    // double TimelineZoom;
    os << "TimelineZoom: " << TimelineZoom << slashn;
    os << slashn;

    // [Metadata]	用于识别谱面的元数据	键:值 对
    os << "[Metadata]" << slashn;
    /*
     *Title	String（字符串）	歌曲标题的罗马音
     */
    // std::string Title;
    os << "Title:" << Title << slashn;

    /*
     ***TitleUnicode	String（字符串）	歌曲标题
     */
    // std::string TitleUnicode;
    os << "TitleUnicode:" << TitleUnicode << slashn;

    /*
     ***Artist	String（字符串）	艺术家的罗马音
     */
    // std::string Artist;
    os << "Artist:" << Artist << slashn;

    /*
     ***ArtistUnicode	String（字符串）	艺术家
     */
    // std::string ArtistUnicode;
    os << "ArtistUnicode:" << ArtistUnicode << slashn;

    /*
     ***Creator	String（字符串）	谱师（谱面创建者）
     */
    // std::string Creator;
    os << "Creator:" << Creator << slashn;

    /*
     ***Version	String（字符串）	难度名
     */
    // std::string Version;
    os << "Version:" << Version << slashn;

    /*
     ***Source	String（字符串）	歌曲信息与档案的来源
     */
    // std::string Source;
    os << "Source:" << Source << slashn;

    /*
     ***Tags	空格分隔的 String（字符串）数组	易于搜索的标签
     */
    // std::vector<std::string> Tags;
    std::string tagstr = "";
    if (!Tags.empty()) {
        for (const auto& tag : Tags) {
            tagstr.append(tag);
            tagstr.push_back(' ');
        }
        tagstr.pop_back();
    }
    os << "Tags:" << tagstr << slashn;

    /*
     ***BeatmapID	Integer（整型）	难度 ID（BID）
     */
    // int32_t BeatmapID;
    os << "BeatmapID:" << BeatmapID << slashn;

    /*
     ***BeatmapSetID	Integer（整型）	谱面 ID（SID）
     */
    // int32_t BeatmapSetID;
    os << "BeatmapSetID:" << BeatmapSetID << slashn;
    os << slashn;

    // [Difficulty]	即谱面难度设定	键:值 对
    os << "[Difficulty]" << slashn;
    /*
     ***HPDrainRate	Decimal（精准小数）	HP 值（0-10）
     */
    // double HPDrainRate{5.0};
    os << "HPDrainRate:" << HPDrainRate << slashn;

    /*
     ***CircleSize	Decimal（精准小数）	CS 值（0-10）
     * osu mania 模式下这个就是key数
     */
    // double CircleSize{4.0};
    os << "CircleSize:" << CircleSize << slashn;

    /*
     ***OverallDifficulty	Decimal（精准小数）	OD 值（0-10）
     * od8~malody的c判
     */
    // double OverallDifficulty{8.0};
    os << "OverallDifficulty:" << OverallDifficulty << slashn;

    /*
     ***ApproachRate	Decimal（精准小数）	AR 值（0-10）
     * 似乎om模式里没卵用
     */
    // double ApproachRate;
    os << "ApproachRate:" << ApproachRate << slashn;

    /*
     ***SliderMultiplier	Decimal（精准小数）	基础滑条速度倍率，乘以
     *100 后可得到该速度下每拍内滑条会经过多少 osu! 像素 似乎om模式里没卵用
     */
    // double SliderMultiplier;
    os << "SliderMultiplier:" << SliderMultiplier << slashn;

    /*
     ***SliderTickRate Decimal（精准小数）	滑条点倍率，每拍中滑条点的数量
     * 似乎om模式里没卵用
     */
    // double SliderTickRate;
    os << "SliderTickRate:" << SliderTickRate << slashn;
    os << slashn;

    // [Events]	谱面显示设定，故事板事件	逗号分隔的列表
    os << "[Events]" << slashn;
    /*
     *背景
     *背景语法：0,0,文件名,x 轴位置,y 轴位置
     *
     *文件名（字符串）：
     *背景图片在谱面文件夹内的文件名或者相对路径。若文件路径周围包含英文双引号，则也可被识别。
     *x 轴位置（整型） 和 y 轴位置（整型）：
     *以屏幕中心为原点的背景图片位置偏移值，单位是 osu! 像素。例如，50,100
     *表示这张背景图片在游玩时，需要移动至屏幕中心向右移动 50 osu!
     *像素，向下移动 100 osu! 像素显示。如果偏移值为 0,0，也可以忽略不写。 视频
     *视频语法：Video,开始时间,文件名,x 轴位置,y 轴位置
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
    std::string bgstr = "";
    bgstr.push_back(std::to_string(background_type).at(0));
    bgstr.push_back(',');
    bgstr.push_back(std::to_string(video_starttime).at(0));
    bgstr.push_back(',');
    bgstr.append("\"" + bg_path.filename().generic_string() + "\"");
    bgstr.push_back(',');
    bgstr.push_back(std::to_string(bgxoffset).at(0));
    bgstr.push_back(',');
    bgstr.push_back(std::to_string(bgyoffset).at(0));

    os << "//Background and Video events" << slashn;
    os << bgstr << slashn;
    os << "//Break Periods" << slashn;
    os << "//Storyboard Layer 0 (Background)" << slashn;
    os << "//Storyboard Layer 1 (Fail)" << slashn;
    os << "//Storyboard Layer 2 (Pass)" << slashn;
    os << "//Storyboard Layer 3 (Foreground)" << slashn;
    os << "//Storyboard Layer 4 (Overlay)" << slashn;
    os << "//Storyboard Sound Samples" << slashn << slashn;

    //  [TimingPoints]	时间轴设定	逗号分隔的列表
    os << "[TimingPoints]" << slashn;
    for (const auto& timing : timings) {
        auto otiming = std::dynamic_pointer_cast<OsuTiming>(timing);
        if (otiming) {
        } else {
            // 从基本物件转化
        }
        os << otiming->to_osu_description() << slashn;
    }

    os << slashn;
    //  [Colours]	连击、皮肤颜色	键 : 值 对
    //  不写
    //  [HitObjects]	击打物件	逗号分隔的列表
    os << "[HitObjects]" << slashn;
    // 防止重复写出
    std::unordered_map<std::shared_ptr<HitObject>, bool> writed_objects;

    // 渲染物件
    // 计算图形
    for (const auto& obj : hitobjects) {
        if (!obj->is_note || obj->object_type == HitObjectType::RMCOMPLEX)
            continue;
        auto note = std::dynamic_pointer_cast<Note>(obj);
        if (!note) continue;
        if (writed_objects.find(note) == writed_objects.end())
            writed_objects.insert({note, true});
        else
            continue;
        // 写出物件
        // 尝试转化OSUNOTE
        auto osu_note = std::dynamic_pointer_cast<OsuNote>(note);
        if (osu_note) {
            os << osu_note->to_osu_description(orbits) << slashn;
        } else {
            // 尝试转化OSUHOLD
            auto osu_hold = std::dynamic_pointer_cast<OsuHold>(note);
            if (osu_hold) {
                os << osu_hold->to_osu_description(orbits) << slashn;
            } else {
                // 都不是-转化
                switch (note->note_type) {
                    case NoteType::NOTE: {
                        osu_note = std::make_shared<OsuNote>(
                            std::static_pointer_cast<Note>(note));
                        os << osu_note->to_osu_description(orbits) << slashn;
                        break;
                    }
                    case NoteType::HOLD: {
                        osu_hold = std::make_shared<OsuHold>(
                            std::static_pointer_cast<Hold>(note));
                        os << osu_hold->to_osu_description(orbits) << slashn;
                        break;
                    }
                        // 需要区分滑键-放若干个单键替换
                    case NoteType::SLIDE: {
                        auto slide_notes = OsuNote::from_slide(
                            std::static_pointer_cast<Slide>(note));
                        for (auto& slide_note : slide_notes) {
                            os << slide_note.to_osu_description(orbits)
                               << slashn;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
    XINFO("已保存为[" + res + "]");
}

// 写出文件是否合法
bool OsuMap::is_write_file_legal(const char* file, std::string& res) {
    // 应该的文件名
    auto legal_file_name =
        mutil::sanitizeFilename(artist + " - " + title + "(" + author + ")" +
                                "[" + version + "]" + ".osu");
    auto opath = std::filesystem::path(file);
    auto opathstr = opath.generic_string();
    if (!mutil::endsWithExtension(opath, ".osu")) {
        res = (opath.parent_path() / legal_file_name).generic_string();
        return false;
    }
    res = opathstr;
    return true;
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

        auto sample_set_str =
            osureader.get_value("General", "SampleSet", std::string("None"));
        while (sample_set_str.starts_with(' ')) {
            sample_set_str.erase(0, 1);
        }

        if (sample_set_str == "None") {
            sample_set = SampleSet::NONE;
        } else if (sample_set_str == "Soft") {
            sample_set = SampleSet::SOFT;
        } else if (sample_set_str == "Normal") {
            sample_set = SampleSet::NORMAL;
        } else if (sample_set_str == "Drum") {
            sample_set = SampleSet::DRUM;
        } else {
            sample_set = SampleSet::NONE;
        }

        StackLeniency = osureader.get_value("General", "StackLeniency", 0.0);
        Mode = osureader.get_value("General", "Mode", 0);
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
        title = Title;
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
        video_starttime = std::stoi(background_paras.at(1));
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
        bg_rpath =
            std::filesystem::relative(bg_path, map_file_path.parent_path());
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
