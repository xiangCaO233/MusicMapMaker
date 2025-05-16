#include "OsuHold.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "src/mmm/hitobject/Note/Hold.h"

// 构造OsuHoldEnd
OsuHoldEnd::OsuHoldEnd(const std::shared_ptr<OsuHold>& ohold)
    : HoldEnd(ohold), reference(ohold.get()) {
    object_type = HitObjectType::OSUHOLDEND;
}

// 析构OsuHoldEnd
OsuHoldEnd::~OsuHoldEnd() = default;

// 打印用
std::string OsuHoldEnd::toString() {
    return "OsuHoldEnd{timestamp=" + std::to_string(timestamp) +
           ", ref_timestamp=" + std::to_string(reference->timestamp) +
           ", ref_orbit=" + std::to_string(reference->orbit) + "}";
}

OsuHold::OsuHold() : Hold(0, 0, 0) {
    note_type = NoteType::HOLD;
    object_type = HitObjectType::OSUHOLD;
}

OsuHold::OsuHold(uint32_t time, int32_t orbit_pos, uint32_t holdtime)
    : Hold(time, orbit_pos, holdtime) {
    note_type = NoteType::HOLD;
    object_type = HitObjectType::OSUHOLD;
}
// 从父类构造
OsuHold::OsuHold(std::shared_ptr<Hold> src)
    : Hold(src->timestamp, src->orbit, src->hold_time) {
    //-填充属性
    object_type = HitObjectType::OSUHOLD;
}

OsuHold::~OsuHold() {}

// 打印用
std::string OsuHold::toString() {
    std::string sampleStr;
    switch (sample) {
        case NoteSample::NORMAL:
            sampleStr = "NORMAL";
            break;
        case NoteSample::WHISTLE:
            sampleStr = "WHISTLE";
            break;
        case NoteSample::FINISH:
            sampleStr = "FINISH";
            break;
        case NoteSample::CLAP:
            sampleStr = "CLAP";
            break;
    }

    return "OsuHold{timestamp=" + std::to_string(timestamp) +
           ", orbit=" + std::to_string(orbit) +
           ", hold_time=" + std::to_string(hold_time) +
           ", sample=" + sampleStr + ", normalSet=" +
           std::to_string(static_cast<int>(sample_group.normalSet)) +
           ", additionalSet=" +
           std::to_string(static_cast<int>(sample_group.additionalSet)) + "}";
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
    int x = static_cast<int>((double(orbit) + 0.5) * 512 / orbit_count);
    oss << x << ",";

    // y 坐标 (固定192)
    oss << "192,";

    // 开始时间
    oss << timestamp << ",";

    // 物件类型 (HOLD=128)
    oss << "128,";

    // 长键音效 (NoteSample枚举值)
    oss << static_cast<int>(sample) << ",";

    // 结束时间和音效组参数
    int end_time = timestamp + hold_time;
    oss << end_time << ":";

    // 音效组参数
    oss << static_cast<int>(sample_group.normalSet) << ":";
    oss << static_cast<int>(sample_group.additionalSet) << ":";
    oss << sample_group.sampleSetParameter << ":";
    oss << sample_group.volume << ":";

    // 自定义音效文件 (如果有)
    if (!sample_group.sampleFile.empty()) {
        oss << sample_group.sampleFile;
    }

    return oss.str();
}

// 从osu描述加载
void OsuHold::from_osu_description(std::vector<std::string>& description,
                                   int32_t orbit_count) {
    /*
     *长键（仅 osu!mania）
     *长键语法： x,y,开始时间,物件类型,长键音效,结束时间,长键音效组
     *
     *结束时间（整型）： 长键的结束时间，以谱面音频开始为原点，单位是毫秒。
     *x 与长键所在的键位有关。算法为：floor(x * 键位总数 / 512)，并限制在 0 和
     *键位总数 - 1 之间。 *y 不影响长键。默认值为 192，即游戏区域的水平中轴。
     */
    // 位置
    orbit = std::floor(std::stoi(description.at(0)) * orbit_count / 512);
    // 没卵用-om固定192
    // int y = std::stoi(description.at(1));
    // 时间戳
    timestamp = std::stoi(description.at(2));
    note_type = NoteType::HOLD;
    // 音效
    sample = static_cast<NoteSample>(std::stoi(description.at(4)));

    // 长条结束时间
    // 结束时间和音效组参数粘一起了
    // hold_time = std::stoi(description.at(5)) - timestamp;
    std::string token;
    std::istringstream noteiss(description.at(5));
    std::vector<std::string> last_paras;
    while (std::getline(noteiss, token, ':')) {
        last_paras.push_back(token);
    }
    // 最后一组的第一个参数就是结束时间
    hold_time = std::stoi(last_paras.at(0)) - timestamp;

    // 剩下四~五个是音效组参数
    sample_group.normalSet =
        static_cast<SampleSet>(std::stoi(last_paras.at(1)));
    sample_group.additionalSet =
        static_cast<NoteSample>(std::stoi(last_paras.at(2)));
    sample_group.sampleSetParameter = std::stoi(last_paras.at(3));
    sample_group.volume = std::stoi(last_paras.at(4));
    if (last_paras.size() == 6) {
        // 有指定key音文件
        sample_group.sampleFile = last_paras.back();
    }
}
