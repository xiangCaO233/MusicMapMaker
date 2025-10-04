#include <QDebug>
#include <fstream>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/rm/Slide.hpp>

enum class ComplexInfo {
    NONE,
    HEAD,
    BODY,
    END,
};

// 写出一个物件
void write_note(std::ofstream& os, const Note* note,
                ComplexInfo compinfo = ComplexInfo::NONE) {
    // 写出物件
    //
    //
    // 类型
    uint8_t comp_type = 0x00;
    // 获取组合信息
    switch (compinfo) {
        case ComplexInfo::HEAD: {
            comp_type = 0x60;
            break;
        }
        case ComplexInfo::BODY: {
            comp_type = 0x20;
            break;
        }
        case ComplexInfo::END: {
            comp_type = 0xa0;
            break;
        }
        default:
            break;
    }

    uint8_t note_type = static_cast<uint8_t>(note->notetype());
    uint8_t type_value = comp_type | note_type;
    os.write(reinterpret_cast<const char*>(&type_value), sizeof(type_value));

    // 没卵用的空字节
    uint8_t void_byte = 0;
    os.write(reinterpret_cast<const char*>(&void_byte), sizeof(void_byte));

    // 时间戳：uint32_t -> int32_t，检查溢出
    uint32_t uint_time = note->timestamp();
    if (uint_time >
        static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
        // 值太大
        uint_time = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
    }
    int32_t timestamp = static_cast<int32_t>(uint_time);
    os.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));

    // 轨道：uint32_t -> uint8_t，轨道最大为255个
    uint32_t uint_track = note->trackpos();
    uint8_t imdorbit = static_cast<uint8_t>(
        std::min(uint_track,
                 static_cast<uint32_t>(std::numeric_limits<uint8_t>::max())));
    os.write(reinterpret_cast<const char*>(&imdorbit), sizeof(imdorbit));

    // 参数：根据类型，转int32_t
    int32_t param = 0;
    switch (note->notetype()) {
        case NoteType::SLIDE: {
            int64_t delta = static_cast<const Slide*>(note)->delta_track();
            // int64_t -> int32_t，检查范围
            if (delta > std::numeric_limits<int32_t>::max()) {
                delta = std::numeric_limits<int32_t>::max();
            } else if (delta < std::numeric_limits<int32_t>::min()) {
                delta = std::numeric_limits<int32_t>::min();
            }
            param = static_cast<int32_t>(delta);
            break;
        }
        case NoteType::HOLD: {
            uint32_t uint_dur = static_cast<const Hold*>(note)->duration();
            // uint32_t -> int32_t，正数溢出限制最大
            if (uint_dur >
                static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
                uint_dur =
                    static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
            }
            param = static_cast<int32_t>(uint_dur);
            break;
        }
        default:
            break;
    }
    os.write(reinterpret_cast<const char*>(&param), sizeof(param));
}

void MMap::writeImd(const std::string& desfile) {
    auto p = std::filesystem::path(desfile);

    // 统计表格行数
    auto table_rows = 0;
    std::unordered_map<const Note*, bool> readed_object;
    auto allnote_handles = note_set().get_all_notes_ordered();
    for (const auto& handle : allnote_handles) {
        // 筛除面尾滑尾和包含组合键引用的物件和重复物件
        // 优先写出完整组合键
        auto note = static_cast<const Note*>(note_set().get_note(handle));
        if (!readed_object.contains(note)) {
            if (note->type == NoteType::COMPOSITE) {
                // 统计组合键
                auto comp = static_cast<const Composite*>(note);
                for (const auto& child : comp->children()) {
                    ++table_rows;
                }
            } else {
                ++table_rows;
            }
            readed_object.insert({note, true});
        }
    }

    // 写出到指定文件
    std::ofstream os(desfile, std::ios::binary);
    /*
     * 0~4字节:int32 谱面时长
     * 5~8字节:int32 图时间点数
     *
     * 接下来每12字节按4字节int32+8字节float64(double)组合为一个时间点
     * 共${图时间点数}组timing数据
     *
     * 然后一个03 03未知意义的int16
     *
     * 接下来一个int32:表格行数
     *
     * 后面全是物件的数据
     * 11字节为一组
     * 00   00   00 00 00 00  00    00 00 00 00
     * 类型 没用    时间戳    轨道     参数
     * ---类型
     * 高位为0时不处于组合键中
     * 高位为6时处于组合键的第一个子键
     * 高位为2时处于组合键的中间部分子键
     * 高位为A时处于组合键的最后一个子键
     *
     * 低位为0时为单键类型
     * 低位为1时为滑键类型
     * 低位为2时为长键类型
     *
     * ---参数
     * 类型为长键时代表持续时间
     * 类型为滑键时为滑动参数
     *
     * 滑动参数为-1代表向左滑动1轨道
     * 滑动参数为-2代表向左滑动2轨道
     * 滑动参数为3代表向右滑动3轨道
     */
    // 0~4字节:int32 谱面时长
    uint32_t uint_length = basemeta.map_length;
    if (uint_length >
        static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
        // 哎呀，值太大啦~ 抛异常或者夹到最大值，随你啦
        throw std::runtime_error("长度太大了，小笨蛋，谱面要爆炸哦~");
    }
    int32_t length = static_cast<int32_t>(uint_length);
    os.write(reinterpret_cast<const char*>(&length), sizeof(length));

    auto& timing_points = timings.get_all_timing_points();

    // 5~8字节:int32 图时间点数
    int32_t timing_count{0};
    for (const auto& [time, timing_vec] : timing_points) {
        for (const auto& timing : timing_vec) {
            if (timing->is_base_timing) {
                ++timing_count;
            }
        }
    }

    // double process_time = 0;
    // double bpm = timings.begin()->get()->basebpm;
    // double beat_length = 60 * 1000.0 / bpm;

    // // 兼容模式-塞满垃圾timing
    // while (int32_t(process_time) < map_length) {
    //     auto process_time_i = int32_t(process_time);
    //     process_time += beat_length;
    //     ++timing_count;
    // }
    // ++timing_count;
    os.write(reinterpret_cast<const char*>(&timing_count),
             sizeof(timing_count));

    // double bpm{0};
    // 接下来每12字节按4字节int32+8字节float64(double)组合为一个时间点
    // 共${图时间点数}组timing数据
    for (const auto& [time, timing_vec] : timing_points) {
        for (const auto& timing : timing_vec) {
            if (timing->is_base_timing) {
                int32_t timing_time = timing->timestamp;
                double timing_bpm = timing->bpm;
                // bpm = timing_bpm;
                os.write(reinterpret_cast<const char*>(&timing_time),
                         sizeof(timing_time));
                os.write(reinterpret_cast<const char*>(&timing_bpm),
                         sizeof(timing_bpm));
            }
        }
    }

    // auto process_time = 0;
    // auto beat_length = 60000.0 / bpm;

    // // 兼容模式-塞满垃圾timing
    // while (int32_t(process_time) < basemeta.map_length) {
    //     auto process_time_i = int32_t(process_time);
    //     os.write(reinterpret_cast<const char*>(&process_time_i),
    //              sizeof(process_time_i));
    //     os.write(reinterpret_cast<const char*>(&bpm), sizeof(bpm));
    //     process_time += beat_length;
    // }

    // auto process_time_i = int32_t(process_time);
    // os.write(reinterpret_cast<const char*>(&process_time_i),
    //          sizeof(process_time_i));
    // os.write(reinterpret_cast<const char*>(&bpm), sizeof(bpm));

    // 然后一个03 03未知意义的int16
    int16_t unknown_flag = 0x0303;
    os.write(reinterpret_cast<const char*>(&unknown_flag),
             sizeof(unknown_flag));

    // 接下来一个int32:表格行数
    os.write(reinterpret_cast<const char*>(&table_rows), sizeof(table_rows));
    // 后面全是物件的数据
    // 11字节为一组
    // 00   00   00 00 00 00  00    00 00 00 00
    // 类型 没用    时间戳    轨道     参数
    // ---类型
    // 高位为0时不处于组合键中
    // 高位为6时处于组合键的第一个子键
    // 高位为2时处于组合键的中间部分子键
    // 高位为A时处于组合键的最后一个子键
    //
    // 低位为0时为单键类型
    // 低位为1时为滑键类型
    // 低位为2时为长键类型
    //
    // ---参数
    // 类型为长键时代表持续时间
    // 类型为滑键时为滑动参数
    //
    // 滑动参数为-1代表向左滑动1轨道
    // 滑动参数为-2代表向左滑动2轨道
    // 滑动参数为3代表向右滑动3轨道
    //
    //
    auto write_note_count{0};
    // 防止重复写出同一物件
    std::unordered_map<const Note*, bool> writed_object;
    for (const auto& handle : allnote_handles) {
        auto note = note_set().get_note(handle);
        // 优先写出完整组合键
        // 不写出处于组合键内的物件
        if (!writed_object.contains(note))
            writed_object.insert({note, true});
        else
            continue;
        if (note->notetype() == NoteType::COMPOSITE) {
            auto comp = static_cast<const Composite*>(note);
            Note* prechild{nullptr};
            // 直接写出所有子键
            for (auto& child_note : comp->children()) {
                writed_object.insert({child_note.get(), true});
                // 修正当前组合物件子键的时间戳
                if (prechild) {
                    switch (prechild->notetype()) {
                        case NoteType::HOLD: {
                            auto pre_hold = static_cast<Hold*>(prechild);
                            auto current_slide =
                                static_cast<Slide*>(child_note.get());
                            current_slide->time =
                                pre_hold->timestamp() + pre_hold->duration();
                            break;
                        }
                        case NoteType::SLIDE: {
                            auto pre_slide = static_cast<Slide*>(prechild);
                            auto current_hold =
                                static_cast<Hold*>(child_note.get());
                            current_hold->time = pre_slide->timestamp();
                            break;
                        }
                        default:
                            break;
                    }
                }
                auto info = ComplexInfo::BODY;
                if (child_note == comp->children().front()) {
                    info = ComplexInfo::HEAD;
                }
                if (child_note == comp->children().back()) {
                    info = ComplexInfo::END;
                }
                ++write_note_count;
                write_note(os, child_note.get(), info);
                prechild = child_note.get();
            }

        } else {
            write_note(os, note);
            ++write_note_count;
        }
    }
    qDebug() << "imd 写出物件数量:" << std::to_string(write_note_count);
    qDebug() << "已保存为[" << desfile << "]";
}
