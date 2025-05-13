#ifndef M_OSUMAP_H
#define M_OSUMAP_H

#include <cstdint>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../SampleSet.h"
#include "../MMap.h"
/*
 *.osu (文件格式)
 *.osu 是一种简单易懂的，用于存储 osu! 谱面难度信息的文件格式。
 *
 *结构
 *文件第一行描述了谱面文件存储格式的版本。目前，osu file format v14 是最新版。
 *
 *之后的内容分为多个章节，并由方括号包裹的章节标题所分隔。
 *
 *章节	描述	内容类型
 *[General]	谱面的总体信息	键: 值 对
 *[Editor]	可在谱面编辑器内显示的信息	键: 值 对
 *[Metadata]	用于识别谱面的元数据	键:值 对
 *[Difficulty]	即谱面难度设定	键:值 对
 *[Events]	谱面显示设定，故事板事件	逗号分隔的列表
 *[TimingPoints]	时间轴设定	逗号分隔的列表
 *[Colours]	连击、皮肤颜色	键 : 值 对
 *[HitObjects]	击打物件	逗号分隔的列表
 */

class OsuHold;
class OsuTiming;

class OsuMap : public MMap {
   public:
    // 构造OsuMap
    OsuMap();
    // 析构OsuMap
    ~OsuMap() override;

    // 文件第一行描述了谱面文件存储格式的版本。
    // 目前，osu file format v14是最新版。
    uint32_t file_format_version{14};

    // Chapter General
    /*
     *常规
     *项目	数据类型	描述	默认值
     */

    /*
     ***AudioFilename	String（字符串）	音频文件的相对路径 默认无
     */
    std::string AudioFilename{""};

    /*
     ***AudioLeadIn	Integer（整型）	音频文件播放之前预留的空白时间	默认0
     */
    int32_t AudioLeadIn{0};

    /*
     ***AudioHash	String（字符串）	已弃用
     */
    [[deprecated]] std::string AudioHash{""};

    /*
     ***PreviewTime	Integer（整型）	在选中谱面时的歌曲预览点位置（毫秒）-1
     */
    int32_t PreviewTime{-1};

    /*
     ***Countdown	Integer（整型）	在第一个物件之前出现的倒计时速度 (0 =
     *无倒计时, 1 = 正常速度, 2 = 一半速度, 3 = 二倍速度)	默认1
     */
    int32_t Countdown{1};

    /*
     ***SampleSet	String（字符串）
     *当时间点（红线、绿线）未覆盖设置时的默认音效组（Normal、Soft、Drum）
     *默认Normal
     */
    SampleSet sample_set{SampleSet::NORMAL};

    /*
     ***StackLeniency	Decimal（精准小数）
     *当物件重叠在同一个位置时，决定物件之间是否出现堆叠效果的阈值（0-1）
     *0.7 Mode Integer（整型）	游戏模式（0 = osu!、1 = osu!taiko、2 =
     *osu!catch、3 = osu!mania）	默认0
     */
    double StackLeniency{0.0};

    /*
     ***LetterboxInBreaks
     *Boolean（布尔值）是否开启谱面休息段使用黑边填充设置 默认0
     */
    bool LetterboxInBreaks{false};

    /*
     ***StoryFireInFront Boolean（布尔值）	已弃用	默认1
     */
    [[deprecated]] bool StoryFireInFront{true};

    /*
     ***UseSkinSprites Boolean（布尔值）是否允许故事板使用玩家皮肤元素	默认0
     */
    bool UseSkinSprites{false};

    /*
     ***AlwaysShowPlayfield Boolean（布尔值） 已弃用	默认0
     */
    [[deprecated]] bool AlwaysShowPlayfield{false};

    /*
     ***OverlayPosition String（字符串）
     *设置物件皮肤覆盖层与数字层之间的关系（NoChange = 使用玩家皮肤设定， Below
     *= 覆盖层绘制于数字之下，Above = 覆盖层绘制于数字之上）	默认NoChange
     */
    std::string OverlayPosition{"NoChange"};

    /*
     ***SkinPreference String（字符串） 推荐在游玩时使用的皮肤名称 默认无
     */
    std::string SkinPreference{""};

    /*
     ***EpilepsyWarning Boolean（布尔值） 是否开启谱面闪烁（癫痫）警告	默认0
     */
    bool EpilepsyWarning{false};

    /*
     ***CountdownOffset Integer（整型）
     *谱面第一个物件之前的倒计时的偏移值（拍子）默认0
     */
    int32_t CountdownOffset{0};

    /*
     ***SpecialStyle Boolean（布尔值） 是否在 osu!mania 谱面中启用 BMS 风格（N+1
     *键）的键位设置 默认0
     */
    bool SpecialStyle{false};

    /*
     ***WidescreenStoryboard	Boolean（布尔值）是否开启故事板的宽屏显示 默认0
     */
    bool WidescreenStoryboard{false};

    /*
     ***SamplesMatchPlaybackRate
     *Boolean（布尔值）是否允许当变速类型模组开启时，改变音效的播放速率	默认0
     */
    bool SamplesMatchPlaybackRate{false};

    // Chapter Editor
    /*
     *这些设置只在谱面编辑器内有效，不影响谱面的实际游玩。
     *
     *项目	数据类型	描述
     ***Bookmarks	逗号分隔的 Integer（整型）数组
     *书签（蓝线）的位置（毫秒）
     */
    std::vector<int32_t> Bookmarks;

    /*
     ***DistanceSpacing	Decimal（精准小数）	间距锁定倍率
     */
    double DistanceSpacing;

    /*
     ***BeatDivisor	Integer（整型）	节拍细分
     */
    int32_t BeatDivisor;

    /*
     ***GridSize	Integer（整型）	网格大小
     */
    int32_t GridSize;

    /*
     ***TimelineZoom	Decimal（精准小数）	物件时间轴的缩放倍率
     */
    double TimelineZoom;

    // Chapter Metadata
    /*
     *元数据
     *项目	数据类型	描述
     *
     ***Title	String（字符串）	歌曲标题的罗马音
     */
    std::string Title;

    /*
     ***TitleUnicode	String（字符串）	歌曲标题
     */
    std::string TitleUnicode;

    /*
     ***Artist	String（字符串）	艺术家的罗马音
     */
    std::string Artist;

    /*
     ***ArtistUnicode	String（字符串）	艺术家
     */
    std::string ArtistUnicode;

    /*
     ***Creator	String（字符串）	谱师（谱面创建者）
     */
    std::string Creator;

    /*
     ***Version	String（字符串）	难度名
     */
    std::string Version;

    /*
     ***Source	String（字符串）	歌曲信息与档案的来源
     */
    std::string Source;

    /*
     ***Tags	空格分隔的 String（字符串）数组	易于搜索的标签
     */
    std::vector<std::string> Tags;

    /*
     ***BeatmapID	Integer（整型）	难度 ID（BID）
     */
    int32_t BeatmapID;

    /*
     ***BeatmapSetID	Integer（整型）	谱面 ID（SID）
     */
    int32_t BeatmapSetID;

    // Chapter Difficulty
    /*
     *项目	数据类型	描述
     ***HPDrainRate	Decimal（精准小数）	HP 值（0-10）
     */
    double HPDrainRate{5.0};

    /*
     ***CircleSize	Decimal（精准小数）	CS 值（0-10）
     * osu mania 模式下这个就是key数
     */
    double CircleSize{4.0};

    /*
     ***OverallDifficulty	Decimal（精准小数）	OD 值（0-10）
     * od8~malody的c判
     */
    double OverallDifficulty{8.0};

    /*
     ***ApproachRate	Decimal（精准小数）	AR 值（0-10）
     * 似乎om模式里没卵用
     */
    double ApproachRate;

    /*
     ***SliderMultiplier	Decimal（精准小数）	基础滑条速度倍率，乘以
     *100 后可得到该速度下每拍内滑条会经过多少 osu! 像素 似乎om模式里没卵用
     */
    double SliderMultiplier;

    /*
     ***SliderTickRate Decimal（精准小数）	滑条点倍率，每拍中滑条点的数量
     * 似乎om模式里没卵用
     */
    double SliderTickRate;

    // Chapter Events
    // *[Events]	谱面显示设定，故事板事件	逗号分隔的列表
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
    int32_t background_type{0};
    // 背景文件目录
    std::string bg_file_name;
    // 背景的位置x偏移
    int32_t bgxoffset;
    // 背景的位置y偏移
    int32_t bgyoffset;

    /*
     *休息段
     *休息段语法：Break,开始时间,结束时间
     *
     *Break 可用 2 代替。
     *
     *结束时间（整型）： 休息段的结束时间。以谱面音频开始为原点，单位是毫秒。
     */
    struct Break {
        int32_t start;
        int32_t end;
    };
    std::vector<Break> breaks;

    /*--------------不实现
     *故事板
     *获取更多故事板语法，参见：编写故事板代码https://osu.ppy.sh/wiki/zh/Storyboard/Scripting。
     *
     *故事板代码可编写并存储至独立的 .osb
     *扩展名文件内，且这个文件包含的故事板代码，将能在谱面内所有难度运行并显示。
     *
     *你可以为谱面内的不同难度制作它们专属的故事板，也可以与以上提到的公用故事板结合使用。
     */

    // Chapter Colour
    /*
     *颜色
     *本节的所有设置均为颜色设置。这里的颜色为用逗号分隔的 RGB 数组（0 -
     *255），分别代表此颜色中的红、绿、蓝分量。
     *
     *项目	描述
     ***Combo#， # 处是整型数据	自定义的 Combo 颜色
     * 这里存rgba
     */
    std::vector<int32_t> ComboColor;

    /*
     ***SliderTrackOverride	自定义的滑条轨道颜色
     */
    std::vector<int32_t> SliderTrackOverride;

    /*
     ***SliderBorder	滑条边界颜色
     */
    std::vector<int32_t> SliderBorder;

    // 从文件读取谱面
    void load_from_file(const char* path) override;
};

class OsuFileReader {
   public:
    // 构造OsuFileReader
    OsuFileReader();
    // 析构OsuFileReader
    ~OsuFileReader() = default;

    // 格式化过的属性
    std::unordered_map<std::string,
                       std::unordered_map<std::string, std::string>>
        map_properties;

    // 当前索引
    uint32_t current_timing_index{0};
    uint32_t current_hitobject_index{0};
    uint32_t current_breaks_index{0};

    // 当前章节
    std::string current_chapter;

    // 格式化行
    void parse_line(const std::string& line_buffer);

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

#endif  // M_OSUMAP_H
