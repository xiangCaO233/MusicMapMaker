#include "OsuTiming.h"

#include <cstdlib>
#include <string>

#include "src/mmm/timing/Timing.h"

OsuTiming::OsuTiming() : Timing() { type = TimingType::OSUTIMING; }

OsuTiming::~OsuTiming() = default;

// 转换为osu的字符串
std::string OsuTiming::to_osu_description() { return ""; }

// 从osu的字符串读取
void OsuTiming::from_osu_description(std::vector<std::string>& description) {
    /*
     * 时间语法：时间,拍长,节拍,音效组,音效参数,音量,是否为非继承时间点（红线）,效果
     *时间（整型）：
     *时间轴区间的开始时间，以谱面音频开始为原点，单位是毫秒。
     *这个时间轴区间的结束时间即为下一个时间点的开始时间（如果这是最后一个时间点，则无结束时间）。
     *
     *拍长（精准小数）： 这个参数有两种含义：
     *对于非继承时间点（红线），这个参数即一拍的长度，单位是毫秒。
     *对于继承时间点（绿线），这个参数为负值，去掉符号后被 100
     *整除，即为这根绿线控制的滑条速度。例如，-50
     *代表这一时间段的滑条速度均为基础滑条速度的 2 倍。
     *
     *节拍（整型）： 一小节中的拍子数量。继承时间点（绿线）的这个值无效果。
     *
     *音效组（整型）： 物件使用的默认音效组（0 = 谱面默认设置（SampleSet），1 =
     *normal，2 = soft，3 = drum）。
     *
     *音效参数（整型）： 物件使用的自定义音效参数。
     *0 表示使用 osu! 默认的音效。
     *
     *音量（整型）： 击打物件的音量（0 - 100）。
     *
     *是否为非继承时间点（红线）（布尔值）： 字面意思。
     *效果（整型）： 一位影响时间点特殊效果的参数。参见：效果部分。
     */
    timestamp = std::stof(description.at(0));

    // 上一个基准bpm
    static double last_base_bpm;

    // 先判断是否继承时间点
    is_inherit_timing = std::stoi(description.at(6)) == 0;
    if (is_inherit_timing) {
        // bpm只存储倍速--并非bpm
        bpm = 100.0 / std::abs(std::stod(description.at(1)));
        is_base_timing = false;
        basebpm = last_base_bpm;
    } else {
        // 真实bpm
        bpm = 1.0 / std::stod(description.at(1)) * 1000.0 * 60.0;
        last_base_bpm = bpm;
        basebpm = last_base_bpm;
    }
    // beats
    beat = std::stoi(description.at(2));
    // 音效组
    sample_set = static_cast<SampleSet>(std::stoi(description.at(3)));
    // 音效参数
    sample_parameter = std::stoi(description.at(4));
    // 音量
    volume = std::stoi(description.at(5));
    // 效果
    effect = (int8_t)std::stoi(description.at(6));
}
