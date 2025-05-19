#include "OsuNote.h"

#include <cmath>
#include <memory>
#include <sstream>
#include <string>

#include "../Note.h"
#include "mmm/Metadata.h"

OsuNote::OsuNote() : Note(0, 0) {
    object_type = HitObjectType::OSUNOTE;
    note_type = NoteType::NOTE;
    // 初始化osu元数据
    metadatas.try_emplace(NoteMetadataType::NOSU).first->second =
        std::make_shared<NoteMetadata>();
}
// 从父类转化
OsuNote::OsuNote(std::shared_ptr<Note> src) : Note(src->timestamp, src->orbit) {
    // -填充属性
    auto metait = metadatas.find(NoteMetadataType::NOSU);
    if (metait == metadatas.end()) {
        // 注册元数据
        metait = metadatas.try_emplace(NoteMetadataType::NOSU).first;
    }
    auto& meta = metait->second;
    if (!meta) {
        meta = default_metadata();
    }

    // 从元数据读取成员
    sample = static_cast<NoteSample>(
        std::stoi(meta->get_value<std::string>("sample", "0")));
    sample_group.normalSet = static_cast<SampleSet>(
        std::stoi(meta->get_value<std::string>("samplegroup-normalset", "0")));
    sample_group.additionalSet = static_cast<NoteSample>(std::stoi(
        meta->get_value<std::string>("samplegroup-additionalset", "0")));
    sample_group.sampleSetParameter = std::stoi(
        meta->get_value<std::string>("samplegroup-samplesetparameter", "0"));
    sample_group.volume =
        std::stoi(meta->get_value<std::string>("samplegroup-volume", "0"));
    sample_group.sampleFile =
        meta->get_value<std::string>("samplegroup-samplefile", "");
}

// osu物件默认的元数据
std::shared_ptr<NoteMetadata> OsuNote::default_metadata() {
    auto meta = std::make_shared<NoteMetadata>();
    meta->note_properties["sample"] =
        std::to_string(static_cast<int>(NoteSample::NORMAL));
    NoteSampleGroup defgroup;
    meta->note_properties["samplegroup-normalset"] =
        std::to_string(static_cast<int>(defgroup.normalSet));
    meta->note_properties["samplegroup-additionalset"] =
        std::to_string(static_cast<int>(defgroup.additionalSet));
    meta->note_properties["samplegroup-samplesetparameter"] =
        std::to_string(static_cast<int>(defgroup.sampleSetParameter));
    meta->note_properties["samplegroup-volume"] =
        std::to_string(static_cast<int>(defgroup.volume));
    meta->note_properties["samplegroup-samplefile"] = defgroup.sampleFile;
    return meta;
}

// 从滑键转化
std::vector<OsuNote> OsuNote::from_slide(std::shared_ptr<Slide> slide) {
    // TODO(xiang 2025-05-19): 将滑键转换为多个OsuNote
    return {};
}

OsuNote::OsuNote(uint32_t time, int32_t orbit_pos) : Note(time, orbit_pos) {
    object_type = HitObjectType::OSUNOTE;
    note_type = NoteType::NOTE;
}

OsuNote::~OsuNote() = default;

// 打印用
std::string OsuNote::toString() {
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

    return "OsuNote{timestamp=" + std::to_string(timestamp) +
           ", orbit=" + std::to_string(orbit) + ", sample=" + sampleStr +
           ", normalSet=" +
           std::to_string(static_cast<int>(sample_group.normalSet)) +
           ", additionalSet=" +
           std::to_string(static_cast<int>(sample_group.additionalSet)) + "}";
}

// 转化为osu描述
std::string OsuNote::to_osu_description(int32_t orbit_count) {
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
    int x = static_cast<int>((double(orbit) + 0.5) * 512 / orbit_count);
    oss << x << ",";

    // y 坐标 (固定192)
    oss << "192,";

    // 开始时间
    oss << timestamp << ",";

    // 物件类型 (NOTE=1)
    oss << "1,";

    // 长键音效 (NoteSample枚举值)
    oss << static_cast<int>(sample) << ",";

    // 结束时间 (单键等于开始时间)
    oss << timestamp << ":";

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
void OsuNote::from_osu_description(std::vector<std::string>& description,
                                   int32_t orbit_count) {
    auto metait = metadatas.find(NoteMetadataType::NOSU);
    if (metait == metadatas.end()) {
        // 注册元数据
        metait = metadatas.try_emplace(NoteMetadataType::NOSU).first;
    }
    auto& meta = metait->second;

    // std::string s("");
    // for (const auto& var : description) {
    //  // s.append(var);
    //  XINFO(var);
    //}
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

    note_type = NoteType::NOTE;

    // 音效
    sample = static_cast<NoteSample>(std::stoi(description.at(4)));

    // XINFO("load note:" + s);

    if (description.size() >= 6) {
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
            }
        }
    }
    meta->note_properties["sample"] = std::to_string(static_cast<int>(sample));
    meta->note_properties["samplegroup-normalset"] =
        std::to_string(static_cast<int>(sample_group.normalSet));
    meta->note_properties["samplegroup-additionalset"] =
        std::to_string(static_cast<int>(sample_group.additionalSet));
    meta->note_properties["samplegroup-samplesetparameter"] =
        std::to_string(static_cast<int>(sample_group.sampleSetParameter));
    meta->note_properties["samplegroup-volume"] =
        std::to_string(static_cast<int>(sample_group.volume));
    meta->note_properties["samplegroup-samplefile"] = sample_group.sampleFile;
}
