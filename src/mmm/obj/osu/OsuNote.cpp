#include <math.h>

#include <mmm/obj/osu/OsuNote.hpp>

// 打印用
std::string OsuNote::toString() {}

// 从osu描述加载
void OsuNote::from_osu_description(const std::vector<std::string>& description,
                                   int32_t orbit_count) {
    using enum NoteMetadataType;
    auto metait = metadata().find(OSU);
    if (metait == metadata().end()) {
        // 注册元数据
        metait = metadata().try_emplace(OSU).first;
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
    set_trackpos(std::floor(std::stoi(description.at(0)) * orbit_count / 512));

    // 没卵用-om固定192
    // int y = std::stoi(description.at(1));

    // 时间戳
    set_timestamp(std::stoi(description.at(2)));

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
std::string OsuNote::to_osu_description(int32_t orbit_count) {}
