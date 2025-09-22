#include <math.h>

#include <format>
#include <memory>
#include <mmm/info/osu/OsuNoteInfo.hpp>
#include <mmm/obj/osu/OsuNote.hpp>
#include <mmm/obj/rm/Slide.hpp>
#include <utility>

OsuNote::OsuNote(const MMap* map, const Note* note) : Note(map) {
    set_notetype(note->type);
    // 位置
    set_trackpos(note->trackpos());

    // 时间戳
    set_timestamp(note->timestamp());
}

// 打印用
std::string OsuNote::toString() const {
    auto parent = Note::toString();
    std::string sampleStr;
    switch (notesample()) {
        using enum NoteSample;
        case NORMAL:
            sampleStr = "NORMAL";
            break;
        case WHISTLE:
            sampleStr = "WHISTLE";
            break;
        case FINISH:
            sampleStr = "FINISH";
            break;
        case CLAP:
            sampleStr = "CLAP";
            break;
    }
    auto samplestr = std::format(
        "Sample:{{timestamp={}, orbit={}, sample={}, normalSet={}, "
        "additionalSet={}}}",
        timestamp(), trackpos(), sampleStr,
        static_cast<int>(note_samplegroup().normalSet),
        static_cast<int>(note_samplegroup().additionalSet));
    return parent + "\n" + sampleStr;
}

// 从滑键转换
std::list<std::unique_ptr<OsuNote>> OsuNote::from_slide(const Slide* slide) {
    // 在滑动轨迹上生成note
    if (!slide) return {};
    std::list<std::unique_ptr<OsuNote>> res;
    // 从哪个轨道
    auto from = slide->delta_track() < 0 ? slide->track + slide->delta_track()
                                         : slide->track;
    // 到哪个轨道
    auto to = slide->delta_track() < 0 ? slide->track
                                       : slide->track + slide->delta_track();
    for (auto i{from}; i <= to; ++i) {
        // 构造osunote
        auto generated_note = std::make_unique<OsuNote>(slide->map());
        generated_note->set_timestamp(slide->timestamp());
        generated_note->set_trackpos(i);
        // 添加到结果集
        res.push_back(std::move(generated_note));
    }
    return res;
}

// 从osu描述加载
void OsuNote::from_osu_description(const std::vector<std::string>& description,
                                   int32_t orbit_count) {
    using enum NoteMetadataType;
    auto metait = metadata().find(OSU);
    if (metait == metadata().end()) {
        // 注册元数据
        metait = metadata().try_emplace(OSU).first;
        metait->second = std::make_shared<OsuNoteMetadata>();
    }
    const auto& meta = metait->second;
    set_notetype(NoteType::NORMAL);

    /*
     *长键（仅 osu!mania）
     *长键语法： x,y,开始时间,物件类型,长键音效,结束时间,长键音效组
     *
     *结束时间（整型）： 长键的结束时间，以谱面音频开始为原点，单位是毫秒。
     *x 与长键所在的键位有关。算法为：floor(x * 键位总数 / 512)，并限制在 0 和
     *键位总数 - 1 之间。 *y 不影响长键。默认值为 192，即游戏区域的水平中轴。
     */

    // 位置
    set_trackpos(uint32_t(
        std::floor(std::stod(description.at(0)) * double(orbit_count) / 512.)));

    // 没卵用-om固定192
    // int y = std::stoi(description.at(1));

    // 时间戳
    set_timestamp(std::stoi(description.at(2)));

    // 物件类型
    set_notetype(NoteType::NORMAL);

    // 音效
    set_notesample(static_cast<NoteSample>(std::stoi(description.at(4))));

    // XINFO("load note:" + s);

    if (description.size() >= 6) {
        NoteSampleGroup sample_group;
        // 单键:剩下的就是音效组参数
        std::string token;
        std::istringstream noteiss(description.at(5));
        std::vector<std::string> last_paras;
        while (std::getline(noteiss, token, ':')) {
            last_paras.push_back(token);
        }
        for (int i = 0; i < last_paras.size(); ++i) {
            switch (i) {
                case 0: {
                    sample_group.normalSet =
                        static_cast<SampleSet>(std::stoi(last_paras.at(0)));
                    break;
                }
                case 1: {
                    sample_group.additionalSet =
                        static_cast<NoteSample>(std::stoi(last_paras.at(1)));
                    break;
                }
                case 2: {
                    sample_group.sampleSetParameter =
                        std::stoi(last_paras.at(2));
                    break;
                }
                case 3: {
                    sample_group.volume = std::stoi(last_paras.at(3));
                    break;
                }
                case 4: {
                    // 有指定key音文件
                    sample_group.sampleFile = last_paras.at(4);
                    break;
                }
                default:
                    break;
            }
        }
        set_note_samplegroup(sample_group);
    }

    // 添加统一属性表
    meta->note_properties[OSU]["sample"] =
        std::to_string(static_cast<int>(notesample()));
    meta->note_properties[OSU]["samplegroup-normalset"] =
        std::to_string(static_cast<int>(note_samplegroup().normalSet));
    meta->note_properties[OSU]["samplegroup-additionalset"] =
        std::to_string(static_cast<int>(note_samplegroup().additionalSet));
    meta->note_properties[OSU]["samplegroup-samplesetparameter"] =
        std::to_string(static_cast<int>(note_samplegroup().sampleSetParameter));
    meta->note_properties[OSU]["samplegroup-volume"] =
        std::to_string(static_cast<int>(note_samplegroup().volume));
    meta->note_properties[OSU]["samplegroup-samplefile"] =
        note_samplegroup().sampleFile;
}

// 转化为osu描述
std::string OsuNote::to_osu_description(int32_t orbit_count) const {
    /*
     * 格式:
     * x,y,开始时间,物件类型,长键音效,结束时间:音效组:附加音效组:音效参数:音量[:自定义音效文件]
     * 对于单键:
     *   - 结束时间 = 开始时间
     *   - 音效组参数格式为:
     * normalSet:additionalSet:sampleSetParameter:volume:[sampleFile]
     */

    std::ostringstream oss;

    // x 坐标 (根据轨道数计算)
    // 原公式: orbit = floor(x * orbit_count / 512)
    // 反推: x = orbit * 512 / orbit_count
    auto x = static_cast<int>((double(trackpos()) + 0.5) * 512 / orbit_count);
    oss << x << ",";

    // y 坐标 (固定192)
    oss << "192,";

    // 开始时间
    oss << timestamp() << ",";

    // 物件类型 (NOTE=1)
    oss << "1,";

    // 长键音效 (NoteSample枚举值)
    oss << static_cast<int>(notesample()) << ",";

    // 结束时间 (单键等于开始时间)
    oss << timestamp() << ":";

    // 音效组参数
    oss << static_cast<int>(note_samplegroup().normalSet) << ":";
    oss << static_cast<int>(note_samplegroup().additionalSet) << ":";
    oss << note_samplegroup().sampleSetParameter << ":";
    oss << note_samplegroup().volume << ":";

    // 自定义音效文件 (如果有)
    if (!note_samplegroup().sampleFile.empty()) {
        oss << note_samplegroup().sampleFile;
    }

    return oss.str();
}

// 克隆物件
std::unique_ptr<Note> OsuNote::clone(const MMap* ref) const {
    auto newnote = std::make_unique<OsuNote>(ref);
    newnote->set_timestamp(time);
    newnote->set_trackpos(track);
    newnote->set_note_samplegroup(note_samplegroup());
    newnote->set_notesample(notesample());
    return newnote;
}
