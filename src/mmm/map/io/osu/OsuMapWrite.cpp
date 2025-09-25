#include <QDebug>
#include <fstream>
#include <mmm/info/osu/OsuMapInfo.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/osu/OsuHold.hpp>
#include <mmm/obj/osu/OsuNote.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>
#include <mmm/project/MProject.hpp>
#include <mmm/timing/osu/OsuTiming.hpp>

void MMap::writeOsu(const std::string& desfile) {
    auto p = std::filesystem::path(desfile);

    std::string backslashn;
#ifdef _WIN32
    backslashn = "\r\n";
#else
    backslashn = "\n";
#endif  //_WIN32

    auto meta = std::static_pointer_cast<OsuMapMetadata>(
        metadatas[MapMetadataType::OSU]);
    if (!meta) {
        // 不存在OSU谱面元数据-现场创建一个
        meta = std::make_shared<OsuMapMetadata>(base_metadata(),
                                                project_ref->getPath());
        meta->update_universe_properties();
    }

    auto general = meta->get_chapter<OsuMapChapterGeneral>("General");

    std::ofstream os(desfile);
    // 写出文件
    os << "osu file format v" << meta->file_format_version << backslashn
       << backslashn;
    // 章节	描述	内容类型
    // [General]	谱面的总体信息	键: 值 对
    os << "[General]" << backslashn;
    /*
     ***AudioFilename	String（字符串）	音频文件的相对路径 默认无
     */
    // std::string AudioFilename{""};
    os << "AudioFilename: " << general->AudioFilename << backslashn;

    /*
     ***AudioLeadIn	Integer（整型）	音频文件播放之前预留的空白时间	默认0
     */
    // int32_t AudioLeadIn{0};
    os << "AudioLeadIn: " << general->AudioLeadIn << backslashn;

    /*
     ***AudioHash	String（字符串）	已弃用
     */
    // [[deprecated]] std::string AudioHash{""};
    // os << "AudioHash: " << AudioHash << slashn;

    /*
     ***PreviewTime	Integer（整型）	在选中谱面时的歌曲预览点位置（毫秒）-1
     */
    // int32_t PreviewTime{-1};
    os << "PreviewTime: " << general->PreviewTime << backslashn;

    /*
     ***Countdown	Integer（整型）	在第一个物件之前出现的倒计时速度 (0 =
     *无倒计时, 1 = 正常速度, 2 = 一半速度, 3 = 二倍速度)	默认1
     */
    // int32_t Countdown{1};
    os << "Countdown: " << general->Countdown << backslashn;

    /*
     ***SampleSet	String（字符串）
     *当时间点（红线、绿线）未覆盖设置时的默认音效组（Normal、Soft、Drum）
     *默认Normal
     */
    // SampleSet sample_set{SampleSet::NORMAL};
    std::string sample_name;
    switch (general->sample_set) {
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
    os << "SampleSet: " << sample_name << backslashn;

    /*
     ***StackLeniency	Decimal（精准小数）
     *当物件重叠在同一个位置时，决定物件之间是否出现堆叠效果的阈值（0-1）
     *0.7 Mode Integer（整型）	游戏模式（0 = osu!、1 = osu!taiko、2 =
     *osu!catch、3 = osu!mania）	默认0
     */
    // double StackLeniency{0.0};
    os << "StackLeniency: " << general->StackLeniency << backslashn;

    /*
    ***Mode	Integer（整型）
    *游戏模式（0 = osu!、1 = osu!taiko、2 = osu!catch、3 = osu!mania） 默认0
    */
    // int32_t Mode{0};
    os << "Mode: " << general->Mode << backslashn;

    /*
     ***LetterboxInBreaks
     *Boolean（布尔值）是否开启谱面休息段使用黑边填充设置 默认0
     */
    // bool LetterboxInBreaks{false};
    os << "LetterboxInBreaks: " << general->LetterboxInBreaks << backslashn;

    /*
     ***StoryFireInFront Boolean（布尔值）	已弃用	默认1
     */
    // [[deprecated]] bool StoryFireInFront{true};
    // os << "StoryFireInFront" << (StoryFireInFront ? 1 : 0) << slashn;

    /*
     ***UseSkinSprites Boolean（布尔值）是否允许故事板使用玩家皮肤元素	默认0
     */
    // bool UseSkinSprites{false};
    if (general->UseSkinSprites) {
        os << "UseSkinSprites: " << 1 << backslashn;
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
    if (general->OverlayPosition != "NoChange")
        os << "OverlayPosition: " << general->OverlayPosition << backslashn;

    /*
     ***SkinPreference String（字符串） 推荐在游玩时使用的皮肤名称 默认无
     */
    // std::string SkinPreference{""};
    if (general->SkinPreference != "")
        os << "SkinPreference: " << general->SkinPreference << backslashn;

    /*
     ***EpilepsyWarning Boolean（布尔值） 是否开启谱面闪烁（癫痫）警告	默认0
     */
    // bool EpilepsyWarning{false};
    if (general->EpilepsyWarning) os << "EpilepsyWarning: " << 1 << backslashn;

    /*
     ***CountdownOffset Integer（整型）
     *谱面第一个物件之前的倒计时的偏移值（拍子）默认0
     */
    // int32_t CountdownOffset{0};
    if (general->CountdownOffset != 0)
        os << "CountdownOffset: " << general->CountdownOffset << backslashn;

    /*
     ***SpecialStyle Boolean（布尔值） 是否在 osu!mania 谱面中启用 BMS 风格（N+1
     *键）的键位设置 默认0
     */
    // bool SpecialStyle{false};
    os << "SpecialStyle: " << (general->SpecialStyle ? 1 : 0) << backslashn;

    /*
     ***WidescreenStoryboard	Boolean（布尔值）是否开启故事板的宽屏显示 默认0
     */
    // bool WidescreenStoryboard{false};
    os << "WidescreenStoryboard: " << (general->WidescreenStoryboard ? 1 : 0)
       << backslashn;

    /*
     ***SamplesMatchPlaybackRate
     *Boolean（布尔值）是否允许当变速类型模组开启时，改变音效的播放速率	默认0
     */
    // bool SamplesMatchPlaybackRate{false};
    if (general->SamplesMatchPlaybackRate)
        os << "SamplesMatchPlaybackRate: " << 1 << backslashn;

    os << backslashn;

    // [Editor]	可在谱面编辑器内显示的信息	键: 值 对
    os << "[Editor]" << backslashn;
    auto editor = meta->get_chapter<OsuMapChapterEditor>("Editor");
    /*
     *Bookmarks	逗号分隔的 Integer（整型）数组
     *书签（蓝线）的位置（毫秒）
     */
    // std::vector<int32_t> Bookmarks;
    if (!editor->Bookmarks.empty()) {
        std::string bookmarkstr = "";
        for (const auto& bookmark : editor->Bookmarks) {
            bookmarkstr.append(std::to_string(bookmark));
            bookmarkstr.push_back(',');
        }
        bookmarkstr.pop_back();
        os << "Bookmarks: " << bookmarkstr << backslashn;
    }

    /*
     ***DistanceSpacing	Decimal（精准小数）	间距锁定倍率
     */
    // double DistanceSpacing;
    os << "DistanceSpacing: " << editor->DistanceSpacing << backslashn;

    /*
     ***BeatDivisor	Integer（整型）	节拍细分
     */
    // int32_t BeatDivisor;
    os << "BeatDivisor: " << editor->BeatDivisor << backslashn;

    /*
     ***GridSize	Integer（整型）	网格大小
     */
    // int32_t GridSize;
    os << "GridSize: " << editor->GridSize << backslashn;

    /*
     ***TimelineZoom	Decimal（精准小数）	物件时间轴的缩放倍率
     */
    // double TimelineZoom;
    os << "TimelineZoom: " << editor->TimelineZoom << backslashn;
    os << backslashn;

    // [Metadata]	用于识别谱面的元数据	键:值 对
    os << "[Metadata]" << backslashn;
    auto metadata = meta->get_chapter<OsuMapChapterMetadata>("Metadata");
    /*
     *Title	String（字符串）	歌曲标题的罗马音
     */
    // std::string Title;
    os << "Title:" << metadata->Title << backslashn;

    /*
     ***TitleUnicode	String（字符串）	歌曲标题
     */
    // std::string TitleUnicode;
    os << "TitleUnicode:" << metadata->TitleUnicode << backslashn;

    /*
     ***Artist	String（字符串）	艺术家的罗马音
     */
    // std::string Artist;
    os << "Artist:" << metadata->Artist << backslashn;

    /*
     ***ArtistUnicode	String（字符串）	艺术家
     */
    // std::string ArtistUnicode;
    os << "ArtistUnicode:" << metadata->ArtistUnicode << backslashn;

    /*
     ***Creator	String（字符串）	谱师（谱面创建者）
     */
    // std::string Creator;
    os << "Creator:" << metadata->Creator << backslashn;

    /*
     ***Version	String（字符串）	难度名
     */
    // std::string Version;
    os << "Version:" << metadata->Version << backslashn;

    /*
     ***Source	String（字符串）	歌曲信息与档案的来源
     */
    // std::string Source;
    os << "Source:" << metadata->Source << backslashn;

    /*
     ***Tags	空格分隔的 String（字符串）数组	易于搜索的标签
     */
    // std::vector<std::string> Tags;
    std::string tagstr = "";
    if (!metadata->Tags.empty()) {
        for (const auto& tag : metadata->Tags) {
            tagstr.append(tag);
            tagstr.push_back(' ');
        }
        tagstr.pop_back();
    }
    os << "Tags:" << tagstr << backslashn;

    /*
     ***BeatmapID	Integer（整型）	难度 ID（BID）
     */
    // int32_t BeatmapID;
    os << "BeatmapID:" << metadata->BeatmapID << backslashn;

    /*
     ***BeatmapSetID	Integer（整型）	谱面 ID（SID）
     */
    // int32_t BeatmapSetID;
    os << "BeatmapSetID:" << metadata->BeatmapSetID << backslashn;
    os << backslashn;

    // [Difficulty]	即谱面难度设定	键:值 对
    os << "[Difficulty]" << backslashn;
    auto difficulty = meta->get_chapter<OsuMapChapterDifficulty>("Difficulty");
    /*
     ***HPDrainRate	Decimal（精准小数）	HP 值（0-10）
     */
    // double HPDrainRate{5.0};
    os << "HPDrainRate:" << difficulty->HPDrainRate << backslashn;

    /*
     ***CircleSize	Decimal（精准小数）	CS 值（0-10）
     * osu mania 模式下这个就是key数
     */
    // double CircleSize{4.0};
    os << "CircleSize:" << difficulty->CircleSize << backslashn;

    /*
     ***OverallDifficulty	Decimal（精准小数）	OD 值（0-10）
     * od8~malody的c判
     */
    // double OverallDifficulty{8.0};
    os << "OverallDifficulty:" << difficulty->OverallDifficulty << backslashn;

    /*
     ***ApproachRate	Decimal（精准小数）	AR 值（0-10）
     * 似乎om模式里没卵用
     */
    // double ApproachRate;
    os << "ApproachRate:" << difficulty->ApproachRate << backslashn;

    /*
     ***SliderMultiplier	Decimal（精准小数）	基础滑条速度倍率，乘以
     *100 后可得到该速度下每拍内滑条会经过多少 osu! 像素 似乎om模式里没卵用
     */
    // double SliderMultiplier;
    os << "SliderMultiplier:" << difficulty->SliderMultiplier << backslashn;

    /*
     ***SliderTickRate Decimal（精准小数）	滑条点倍率，每拍中滑条点的数量
     * 似乎om模式里没卵用
     */
    // double SliderTickRate;
    os << "SliderTickRate:" << difficulty->SliderTickRate << backslashn;
    os << backslashn;

    // [Events]	谱面显示设定，故事板事件	逗号分隔的列表
    os << "[Events]" << backslashn;
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
    auto event = meta->get_chapter<OsuMapChapterEvents>("Events");
    std::string bgstr = "";
    bgstr.push_back(std::to_string(event->background_type).at(0));
    bgstr.push_back(',');
    bgstr.push_back(std::to_string(event->video_starttime).at(0));
    bgstr.push_back(',');
    bgstr.append("\"" + event->bg_file_name + "\"");
    bgstr.push_back(',');
    bgstr.push_back(std::to_string(event->bgxoffset).at(0));
    bgstr.push_back(',');
    bgstr.push_back(std::to_string(event->bgyoffset).at(0));

    os << "//Background and Video events" << backslashn;
    os << bgstr << backslashn;
    os << "//Break Periods" << backslashn;
    os << "//Storyboard Layer 0 (Background)" << backslashn;
    os << "//Storyboard Layer 1 (Fail)" << backslashn;
    os << "//Storyboard Layer 2 (Pass)" << backslashn;
    os << "//Storyboard Layer 3 (Foreground)" << backslashn;
    os << "//Storyboard Layer 4 (Overlay)" << backslashn;
    os << "//Storyboard Sound Samples" << backslashn << backslashn;

    //  [TimingPoints]	时间轴设定	逗号分隔的列表
    os << "[TimingPoints]" << backslashn;
    auto& timings = timing_set().get_all_timing_points();
    for (const auto& [time, timing_vec] : timings) {
        for (const auto& timing : timing_vec) {
            auto otiming = dynamic_cast<OsuTiming*>(timing.get());
            if (!otiming) {
                // 并非osutiming,现场创建一个
                auto osutiming = std::make_unique<OsuTiming>(timing.get());
                os << osutiming->to_osu_description() << backslashn;
            } else {
                os << otiming->to_osu_description() << backslashn;
            }
        }
    }

    os << backslashn;
    //  [Colours]	连击、皮肤颜色	键 : 值 对
    //  不写
    //  [HitObjects]	击打物件	逗号分隔的列表
    os << "[HitObjects]" << backslashn;
    // 防止重复写出
    // std::unordered_map<std::shared_ptr<HitObject>, bool> writed_objects;
    auto objhandles = note_set().get_all_notes_ordered();

    for (const auto& objhandle : objhandles) {
        auto obj = note_set().get_note(objhandle);
        // 写出物件
        // 尝试转化OSUNOTE
        auto osu_note = dynamic_cast<const OsuNoteMetadata*>(obj);
        if (osu_note) {
            os << osu_note->to_osu_description(basemeta.track_count)
               << backslashn;
        } else {
            // 都不是-执行转化
            switch (obj->notetype()) {
                case NoteType::NORMAL: {
                    auto osu_note_temp =
                        std::make_unique<OsuNote>(obj->map(), obj);
                    osu_note = osu_note_temp.get();
                    os << osu_note->to_osu_description(basemeta.track_count)
                       << backslashn;
                    break;
                }
                case NoteType::HOLD: {
                    auto osu_note_temp =
                        std::make_unique<OsuHold>(obj->map(), obj);
                    osu_note = osu_note_temp.get();
                    os << osu_note->to_osu_description(basemeta.track_count)
                       << backslashn;
                    // osu_hold = std::make_shared<OsuHold>(
                    //     std::static_pointer_cast<Hold>(note));
                    // os << osu_hold->to_osu_description(orbits) << slashn;
                    break;
                }
                    // 需要区分滑键-放若干个单键替换
                case NoteType::SLIDE: {
                    auto slide_notes =
                        OsuNote::from_slide(static_cast<const Slide*>(obj));
                    for (auto& slide_note : slide_notes) {
                        os << slide_note->to_osu_description(
                                  basemeta.track_count)
                           << backslashn;
                    }
                    break;
                }
                    // 复合键:拆分为若干面条
                // 开头为滑键则在开头多放置一个单键
                // 结尾同理
                case NoteType::COMPOSITE: {
                    auto comp = static_cast<const Composite*>(obj);
                    auto convert_head_or_tail = [comp, &os, &backslashn, this](
                                                    const Note* note,
                                                    bool head) {
                        if (note->type == NoteType::SLIDE) {
                            auto slide = static_cast<const Slide*>(note);
                            auto notes = OsuNote::from_slide(slide);
                            // 第一个是滑键
                            if (head) {
                                if (slide->delta_track() < 0) {
                                    // 掐头
                                    notes.pop_front();
                                } else {
                                    // 去尾
                                    notes.pop_back();
                                }
                            } else {
                                if (slide->delta_track() < 0) {
                                    // 去尾
                                    notes.pop_back();
                                } else {
                                    // 掐头
                                    notes.pop_front();
                                }
                            }
                            for (const auto& note : notes) {
                                os << note->to_osu_description(
                                          basemeta.track_count)
                                   << backslashn;
                            }
                        } else {
                            // 第一个是面条-正常转换
                            auto osu_note_temp =
                                std::make_unique<OsuHold>(comp->map(), note);
                            auto tempnote = osu_note_temp.get();
                            os << tempnote->to_osu_description(
                                      basemeta.track_count)
                               << backslashn;
                        }
                    };
                    if (comp->children().size() < 2) {
                        qDebug() << "非法组合键,跳过转换";
                        break;
                    } else {
                        auto begin = (*comp->children().begin()).get();
                        convert_head_or_tail(begin, true);
                        // 转换中间物件
                        for (auto it =
                                 // 从begin++开始
                             std::next(comp->children().begin());
                             it != comp->children().end(); ++it) {
                            auto& current = *it;
                            if (current == comp->children().back()) {
                                // 是最后一个
                                convert_head_or_tail(current.get(), false);
                            } else {
                                // 转换所有面条
                                if (current->type == NoteType::HOLD) {
                                    auto osu_note_temp =
                                        std::make_unique<OsuHold>(
                                            comp->map(), current.get());
                                    auto tempnote = osu_note_temp.get();
                                    os << tempnote->to_osu_description(
                                              basemeta.track_count)
                                       << backslashn;
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    qDebug() << "已保存为[" << desfile << "]";
}
