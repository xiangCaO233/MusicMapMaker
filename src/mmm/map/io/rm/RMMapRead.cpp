#include <log/colorful-log.h>

#include <QDebug>
#include <cstdint>
#include <fstream>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>
#include <util/mutil.hpp>

// 二进制读取器
class BinaryReader {
   public:
    // 读取指定指针位置的数据
    template <typename T>
    T read_value(const char* data, bool is_little_endian = true) {
        T value;
        std::memcpy(&value, data, sizeof(T));

        if (!is_little_endian) {
            char* ptr = reinterpret_cast<char*>(&value);
            std::reverse(ptr, ptr + sizeof(T));
        }

        return value;
    }
};

void MMap::readImd() {
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
    // 打开文件，以二进制模式读取
    // 切换为绝对路径
    if (basemeta.map_path.is_relative()) {
        basemeta.map_path = std::filesystem::absolute(basemeta.map_path);
    }
    auto fname = basemeta.map_path.filename();
    XINFO("路径:" + basemeta.map_path.string());
    if (basemeta.map_path.extension() == ".imd") {
        auto fnamestr = fname.string();

        auto first_pos = fnamestr.find('_');
        auto second_pos = fnamestr.find('_', first_pos + 1);

        try {
            basemeta.track_count = std::stoi(fnamestr.substr(first_pos + 1, 1));
        } catch (std::exception& e) {
            XWARN("读取文件名key数失败-" + std::string(e.what()));
        }
        auto last_pos = fnamestr.rfind(".");

        // 截取第二个_到最后一个.之间的字符串作为版本
        basemeta.version =
            second_pos < last_pos
                ? fnamestr.substr(second_pos + 1, last_pos - second_pos - 1)
                : "unknown";

        // 截取0~第一个_之间的字符串作为文件前缀-标题
        auto file_presuffix = fnamestr.substr(0, first_pos);
        basemeta.title_unicode = file_presuffix;
        basemeta.title = mutil::sanitizeFilename_ascii(basemeta.title_unicode);

        bool has_audio{true};
        // 前缀+.mp3作为音频文件名
        basemeta.main_audio_path =
            basemeta.map_path.parent_path() / (file_presuffix + ".mp3");
        // 也可为wav,ogg
        if (!std::filesystem::exists(basemeta.main_audio_path)) {
            basemeta.main_audio_path =
                basemeta.map_path.parent_path() / (file_presuffix + ".wav");
            if (!std::filesystem::exists(basemeta.main_audio_path)) {
                basemeta.main_audio_path =
                    basemeta.map_path.parent_path() / (file_presuffix + ".ogg");
                if (!std::filesystem::exists(basemeta.main_audio_path)) {
                    has_audio = false;
                }
            }
        }
        if (!has_audio) {
            basemeta.main_audio_path.clear();
            XWARN("未找到imd对应音频文件");
        }

        // 检查前缀+.png 或.jpg .jpeg有哪个用哪个作为bg
        bool has_bg{true};
        basemeta.main_cover_path =
            basemeta.map_path.parent_path() / (file_presuffix + ".png");
        if (!std::filesystem::exists(basemeta.main_cover_path)) {
            basemeta.main_cover_path =
                basemeta.map_path.parent_path() / (file_presuffix + ".jpg");
            if (!std::filesystem::exists(basemeta.main_cover_path)) {
                basemeta.main_cover_path = basemeta.map_path.parent_path() /
                                           (file_presuffix + ".jpeg");
                if (!std::filesystem::exists(basemeta.main_cover_path)) {
                    has_bg = false;
                }
            }
        }
        if (!has_bg) {
            basemeta.main_cover_path.clear();
            XWARN("未找到imd对应背景图片");
        }

        std::ifstream file(basemeta.map_path, std::ios::binary);

        if (!file) {
            XWARN("无法打开文件" + basemeta.map_path.generic_string());
            return;
        }

        // 获取文件大小
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // 读取整个文件到vector中
        // 读取的全部数据
        std::vector<char> buffer_data(fileSize);
        file.read(buffer_data.data(), fileSize);

        // 读取器
        BinaryReader reader;

        auto data_pos = buffer_data.data();

        // 0~4字节:int32 谱面时长
        // 谱面时长
        basemeta.map_length = reader.read_value<int32_t>(data_pos);
        data_pos += 4;
        XINFO("谱面时长:[" + std::to_string(basemeta.map_length) + "]");

        // 5~8字节:int32 图时间点数
        auto timing_point_amount =
            reader.read_value<int32_t>(buffer_data.data() + 4);
        data_pos += 4;
        XINFO("读取到imd文件时间点数:[" + std::to_string(timing_point_amount) +
              "]");

        std::vector<std::unique_ptr<Timing>> temp_timings;
        // 接下来每12字节按4字节int32+8字节float64(double)组合为一个时间点
        // 共${图时间点数}组timing数据

        Timing* timing{nullptr};
        // rm的timing不能变速,只能变bpm写谱--(附:ivm没写)

        for (int i = 0; i < timing_point_amount; i++) {
            auto timing_timestamp = reader.read_value<int32_t>(data_pos);
            data_pos += 4;
            auto timing_bpm = reader.read_value<double>(data_pos);
            data_pos += 8;
            auto read_timing = std::make_unique<Timing>();
            read_timing->type = TimingType::RMTIMING;
            read_timing->timestamp = timing_timestamp;
            read_timing->bpm = timing_bpm;
            read_timing->beat_length = 60000. / timing_bpm;

            // 防止ivm生成的一万个重复timing
            if (!timing || read_timing->bpm != timing->bpm) {
                timing = read_timing.get();
                // 加入缓存timing列表
                temp_timings.emplace_back(std::move(read_timing));
                XINFO(
                    "读取到timing:[time:" + std::to_string(timing->timestamp) +
                    ",bpm:" + std::to_string(timing->bpm) + "]");
            }
        }

        // 然后一个03 03未知意义的int16
        // 跳过2字节
        data_pos += 2;

        // 接下来一个int32:表格行数
        auto table_rows = reader.read_value<int32_t>(data_pos);
        data_pos += 4;
        XINFO("读取到表格行数:[" + std::to_string(table_rows) + "]");

        // 后面全是物件的数据
        // 11字节为一组
        // 00   00   00 00 00 00  00    00 00 00 00
        // 类型 没用    时间戳    轨道     参数

        // 缓存父类指针
        std::unique_ptr<Note> temp_note;
        // 缓存组合键指针
        std::unique_ptr<Composite> temp_complex_note{nullptr};
        // 组合键是否构建完成
        bool comp_done{true};

        int obj_count = 0;

        // 读取全部物件
        while ((buffer_data.data() + buffer_data.size() - data_pos) > 0) {
            // 类型1字节
            auto note_type_info = reader.read_value<int8_t>(data_pos);
            auto note_complex_info = note_type_info & 0xf0;
            auto note_type = note_type_info & 0x0f;
            // 移动2字节
            data_pos += 2;

            // 时间戳4字节
            auto note_timestamp = reader.read_value<int32_t>(data_pos);
            data_pos += 4;

            // 无符号单字节-轨道位置1字节
            auto note_orbit = reader.read_value<uint8_t>(data_pos);
            data_pos += 1;

            // 物件参数4字节
            auto note_parameter = reader.read_value<int32_t>(data_pos);
            data_pos += 4;

            // 更新轨道数
            if (note_orbit + 1 > basemeta.track_count)
                basemeta.track_count = note_orbit + 1;

            // 更新谱面长度
            if (int64_t(note_timestamp) > basemeta.map_length)
                basemeta.map_length = note_timestamp;

            // 初始化物件
            switch (note_type) {
                case 0: {
                    // 单键
                    temp_note = std::make_unique<Note>(this);
                    break;
                }
                case 1: {
                    // 滑键
                    temp_note = std::make_unique<Slide>(this);
                    auto slide = static_cast<Slide*>(temp_note.get());
                    temp_note->set_notetype(NoteType::SLIDE);
                    slide->set_delta_track(note_parameter);
                    break;
                }
                case 2: {
                    // 长条
                    temp_note = std::make_unique<Hold>(this);
                    temp_note->set_notetype(NoteType::HOLD);
                    auto hold = static_cast<Hold*>(temp_note.get());
                    hold->set_duration(note_parameter);
                    // 需要更新谱面长度
                    if (int64_t(hold->timestamp() + hold->duration()) >
                        basemeta.map_length)
                        basemeta.map_length =
                            hold->timestamp() + hold->duration();
                    break;
                }
            }

            // 组合键处理
            switch (note_complex_info) {
                case 0x60: {
                    // 组合键头(开始键)
                    temp_complex_note = std::make_unique<Composite>(this);
                    temp_complex_note->set_notetype(NoteType::COMPOSITE);
                    temp_complex_note->set_timestamp(note_timestamp);
                    temp_complex_note->set_trackpos(note_orbit);
                    comp_done = false;
                    break;
                }
                case 0x20: {
                    // 组合键中间
                    // 非法出现组合键信息-跳过
                    if (!temp_complex_note) continue;
                    // 只是一味添加
                    // temp_complex_note->add_child(std::move(temp_note));
                    break;
                }
                case 0xa0: {
                    // 组合键尾(结束键)
                    // 设置父物件
                    // 非法出现组合键信息-跳过
                    if (!temp_complex_note) continue;
                    comp_done = true;
                    break;
                }
            }

            // 更新本物件
            temp_note->set_timestamp(note_timestamp);
            temp_note->set_trackpos(note_orbit);
            if (temp_complex_note) {
                // 添加当前物件到缓存组合键
                auto tempnoteptr = temp_note->clone(temp_note->map_ref);
                if (!temp_complex_note->add_child(std::move(temp_note))) {
                    XWARN("添加组合键子键失败");
                    XWARN("物件信息:" + tempnoteptr->toString());
                }
                if (comp_done) {
                    // 把组合物件加入集合(组合在此之后失效)
                    auto handle =
                        note_set().add_note(std::move(temp_complex_note));
                    noteUUIDManager.register_new_note(handle);
                }
            } else {
                // 把物件加入集合(物件在此之后失效)
                auto handle = note_set().add_note(std::move(temp_note));
                noteUUIDManager.register_new_note(handle);
            }

            ++obj_count;
        }

        // orbits = max_orbits;
        XINFO("读取到物件:" + std::to_string(obj_count));
        // qDebug() << "table_rows:" << std::to_string(table_rows);

        // 生成图名
        basemeta.name = "[rm] " + file_presuffix + " [" +
                        std::to_string(basemeta.track_count) + "k] " +
                        basemeta.version;
        // 把timing添加到timing集合
        for (auto& timing : temp_timings) {
            timing_set().add_timing_point(std::move(timing));
        }

        update_preferenceBPM();

        // 最后生成全部拍
        analyzeBeatInfo();

        // std::map<uint32_t, Beat> sorted_beats(beatInfo.begin(),
        // beatInfo.end()); for (const auto& pair : sorted_beats) {
        //     const Beat& b = pair.second;
        //     qDebug() << "Beat at " << b.beat_start << "ms "
        //              << "(length: " << b.beat_length << "ms): "
        //              << "Best division found -> 1/" << b.divisors;
        // }

        // debugmap
        // XINFO("-------全部物件-------");
        // auto note_handles = note_set().get_all_notes_ordered();
        // for (const auto& handle : note_handles) {
        //     XINFO(note_set().get_note(handle)->toString());
        // }
    } else {
        XWARN("非.imd格式,读取失败");
    }
}
