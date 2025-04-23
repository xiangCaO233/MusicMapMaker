#include "RMMap.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "../../hitobject/Note/HoldEnd.h"
#include "../../hitobject/Note/rm/ComplexNote.h"
#include "../../hitobject/Note/rm/Slide.h"
#include "../MMap.h"
#include "colorful-log.h"

// 构造ImdReader
BinaryReader::BinaryReader() {}

// 析构ImdReader
BinaryReader::~BinaryReader() = default;

RMMap::RMMap() : MMap() { maptype = MapType::RMMAP; }

RMMap::~RMMap() = default;

// 从文件读取谱面
void RMMap::load_from_file(const char* path) {
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
  map_file_path = std::filesystem::path(path);
  if (map_file_path.is_relative()) {
    map_file_path = std::filesystem::absolute(map_file_path);
  }
  auto fname = map_file_path.filename();
  auto fnamestr = fname.string();

  auto first_pos = fnamestr.find('_');
  auto second_pos = fnamestr.find('_', first_pos + 1);
  auto last_pos = fnamestr.rfind(".");

  // 截取第二个_到最后一个.之间的字符串作为版本
  version = second_pos < last_pos
                ? fnamestr.substr(second_pos + 1, last_pos - second_pos - 1)
                : "unknown";

  // 截取0~第一个_之间的字符串作为文件前缀
  auto file_presuffix = fnamestr.substr(0, first_pos);

  // 前缀+.mp3作为音频文件名
  audio_file_abs_path = map_file_path.parent_path() / (file_presuffix + ".mp3");

  // 检查前缀+.png 或.jpg .jpeg有哪个用哪个作为bg
  bool has_bg{true};
  bg_path = map_file_path.parent_path() / (file_presuffix + ".png");
  if (!std::filesystem::exists(bg_path)) {
    bg_path = map_file_path.parent_path() / (file_presuffix + ".jpg");
    if (!std::filesystem::exists(bg_path)) {
      bg_path = map_file_path.parent_path() / (file_presuffix + ".jpeg");
      if (!std::filesystem::exists(bg_path)) {
        has_bg = false;
      }
    }
  }
  if (!has_bg) {
    bg_path.clear();
  }

  XINFO("路径:" + map_file_path.string());
  if (map_file_path.extension() != ".imd") {
    XERROR("不是imd文件,读个锤子");
    return;
  }
  std::ifstream file(map_file_path, std::ios::binary);

  if (!file) {
    XERROR("无法打开文件");
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
  map_length = reader.read_value<int32_t>(data_pos);
  data_pos += 4;
  XINFO("谱面时长:[" + std::to_string(map_length) + "]");

  // 5~8字节:int32 图时间点数
  auto timing_point_amount = reader.read_value<int32_t>(buffer_data.data() + 4);
  data_pos += 4;
  XINFO("图时间点数:[" + std::to_string(timing_point_amount) + "]");

  std::vector<std::shared_ptr<Timing>> temp_timings;
  // 接下来每12字节按4字节int32+8字节float64(double)组合为一个时间点
  // 共${图时间点数}组timing数据
  auto timing = std::make_shared<Timing>();
  // rm的timing不能变速,只能变bpm写谱--(附:ivm没写)
  timing->type = TimingType::RMTIMING;

  for (int i = 0; i < timing_point_amount; i++) {
    auto timing_timestamp = reader.read_value<int32_t>(data_pos);
    data_pos += 4;
    auto timing_bpm = reader.read_value<double>(data_pos);
    data_pos += 8;
    auto read_timing = std::make_shared<Timing>();
    read_timing->type = TimingType::RMTIMING;
    read_timing->timestamp = timing_timestamp;
    read_timing->basebpm = timing_bpm;
    read_timing->bpm = timing_bpm;

    // 防止ivm生成的一万个重复timing
    if (read_timing->basebpm != timing->basebpm) {
      timing = read_timing;
      // 加入缓存timing列表
      temp_timings.emplace_back(timing);
      XINFO("读取到timing:[time:" + std::to_string(timing->timestamp) +
            ",bpm:" + std::to_string(timing->basebpm) + "]");
    }
  }

  // 然后一个03 03未知意义的int16
  // 跳过2字节
  data_pos += 2;

  // 接下来一个int32:表格行数
  table_rows = reader.read_value<int32_t>(data_pos);
  data_pos += 4;
  XINFO("表格行数:[" + std::to_string(table_rows) + "]");

  // 后面全是物件的数据
  // 11字节为一组
  // 00   00   00 00 00 00  00    00 00 00 00
  // 类型 没用    时间戳    轨道     参数

  // 缓存父类指针
  std::shared_ptr<Note> temp_note;
  // 缓存组合键指针
  std::shared_ptr<ComplexNote> temp_complex_note;

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
    if (note_orbit + 1 > max_orbits) max_orbits = note_orbit + 1;

    // 更新谱面长度
    if (note_timestamp > map_length) map_length = note_timestamp;

    // 初始化物件
    switch (note_type) {
      case 0: {
        // 单键
        temp_note = std::make_shared<Note>(note_timestamp, note_orbit);
        break;
      }
      case 1: {
        // 滑键
        temp_note =
            std::make_shared<Slide>(note_timestamp, note_orbit, note_parameter);

        // 构造一个滑键尾物件
        auto slide_end = std::make_shared<SlideEnd>(
            std::static_pointer_cast<Slide>(temp_note));
        hitobjects.insert(slide_end);

        break;
      }
      case 2: {
        // 长条
        temp_note =
            std::make_shared<Hold>(note_timestamp, note_orbit, note_parameter);
        auto hold = std::static_pointer_cast<Hold>(temp_note);
        // 构造一个面条尾物件
        auto hold_end = std::make_shared<HoldEnd>(hold);
        hitobjects.insert(hold_end);
        // 面尾也需要更新谱面长度
        if (hold_end->timestamp > map_length) map_length = hold_end->timestamp;
        // 把长条物件加入缓存
        temp_hold_list.insert(hold);
        break;
      }
    }

    // 添加到物件列表
    hitobjects.insert(temp_note);

    // 组合键处理
    switch (note_complex_info) {
      case 0x60: {
        // 组合键头(开始键)
        temp_complex_note =
            std::make_shared<ComplexNote>(note_timestamp, note_orbit);
        // 设置父物件
        temp_note->parent_reference = temp_complex_note.get();
        temp_note->compinfo = ComplexInfo::HEAD;
        // 添加本头
        temp_complex_note->child_notes.emplace_back(temp_note);
        break;
      }
      case 0x20: {
        // 组合键中间
        // 设置父物件
        temp_note->parent_reference = temp_complex_note.get();
        temp_note->compinfo = ComplexInfo::BODY;
        // 只是一味添加
        temp_complex_note->child_notes.emplace_back(temp_note);
        break;
      }
      case 0xa0: {
        // 组合键尾(结束键)
        // 设置父物件
        temp_note->parent_reference = temp_complex_note.get();
        temp_note->compinfo = ComplexInfo::END;
        // 先添加,再把组合键添加进去
        temp_complex_note->child_notes.emplace_back(temp_note);
        hitobjects.insert(temp_complex_note);
        temp_complex_note.reset();
        break;
      }
    }

    // XINFO("物件组合类型:[" + std::to_string(note_complex_info) +
    // "]物件类型:[" +
    //       std::to_string(note_type) + "],时间戳:[" +
    //       std::to_string(note_timestamp) + "],轨道:[" +
    //       std::to_string(note_orbit) + "],参数:[" +
    //       std::to_string(note_parameter) + "]");
  }

  // 生成图名
  map_name = "[rm] " + file_presuffix + " [" + std::to_string(max_orbits) +
             "k] " + version;

  // 添加timing--自动生成拍
  for (const auto& temp_timing : temp_timings) {
    insert_timing(temp_timing);
  }
}
