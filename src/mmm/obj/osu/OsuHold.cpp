#include <math.h>

#include <mmm/obj/osu/OsuHold.hpp>

// 打印用
std::string OsuHold::toString() const {
    return Hold::toString();
    // return "[OsuHold]";
}

// 从osu描述加载
void OsuHold::from_osu_description(const std::vector<std::string>& description,
                                   int32_t orbit_count) {
    set_notetype(NoteType::HOLD);
    /*
     *长键（仅 osu!mania）
     *长键语法： x,y,开始时间,物件类型,长键音效,结束时间,长键音效组
     *
     *结束时间（整型）： 长键的结束时间，以谱面音频开始为原点，单位是毫秒。
     *x 与长键所在的键位有关。算法为：floor(x * 键位总数 / 512)，并限制在 0 和
     *键位总数 - 1 之间。 *y 不影响长键。默认值为 192，即游戏区域的水平中轴。
     */
    for (int i = 0; i < description.size(); ++i) {
        switch (i) {
            case 0: {
                // 位置
                set_trackpos(std::floor(std::stoi(description.at(0)) *
                                        orbit_count / 512));
                break;
            }
            case 1: {
                // 没卵用-om固定192
                // int y = std::stoi(description.at(1));
                break;
            }
            case 2: {
                // 时间戳
                set_timestamp(std::stoi(description.at(2)));
                set_notetype(NoteType::HOLD);
                break;
            }
            case 3: {
                break;
            }
            case 4: {
                // 音效
                set_notesample(
                    static_cast<NoteSample>(std::stoi(description.at(4))));
                break;
            }
            case 5: {
                break;
            }
            default:
                break;
        }
    }

    // 长条结束时间
    // 结束时间和音效组参数粘一起了
    // hold_time = std::stoi(description.at(5)) - timestamp;
    std::string token;
    std::istringstream noteiss(description.at(5));

    // 最后一组的第一个参数就是结束时间
    std::vector<std::string> last_paras;
    while (std::getline(noteiss, token, ':')) {
        last_paras.push_back(token);
    }
    NoteSampleGroup sample_group;
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
                sample_group.sampleSetParameter = std::stoi(last_paras.at(2));
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
    set_duration(std::stoi(last_paras.at(0)) - timestamp());
}

// 转化为osu描述
std::string OsuHold::to_osu_description(int32_t orbit_count) {
    /*
     * 长键格式:
     * x,y,开始时间,物件类型,长键音效,结束时间:音效组:附加音效组:音效参数:音量:[自定义音效文件]
     * 对于长键:
     *   - 物件类型 = 128 (Hold note)
     *   - 结束时间 = 开始时间 + hold_time
     */

    std::ostringstream oss;

    // x 坐标 (根据轨道数计算)
    // 原公式: orbit = floor(x * orbit_count / 512)
    // 反推: x = orbit * 512 / orbit_count
    int x = static_cast<int>((double(trackpos()) + 0.5) * 512 / orbit_count);
    oss << x << ",";

    // y 坐标 (固定192)
    oss << "192,";

    // 开始时间
    oss << timestamp() << ",";

    // 物件类型 (HOLD=128)
    oss << "128,";

    // 长键音效 (NoteSample枚举值)
    oss << static_cast<int>(notesample()) << ",";

    // 结束时间和音效组参数
    int end_time = timestamp() + duration();
    oss << end_time << ":";

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
std::unique_ptr<Note> OsuHold::clone(const MMap* ref) const {
    auto new_note_data = std::make_unique<OsuHold>(ref);
    new_note_data->set_notetype(NoteType::HOLD);
    new_note_data->set_timestamp(timestamp());
    new_note_data->set_trackpos(trackpos());
    new_note_data->set_duration(duration_time);
    new_note_data->set_note_samplegroup(note_samplegroup());
    new_note_data->set_notesample(notesample());
    return new_note_data;
}
