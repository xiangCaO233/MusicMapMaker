#include "OsuMap.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "../../hitobject/Note/osu/OsuHold.h"
#include "../../hitobject/Note/osu/OsuNote.h"
#include "../../timing/osu/OsuTiming.h"
#include "../MMap.h"
#include "colorful-log.h"
#include "src/mmm/hitobject/HitObject.h"
#include "src/mmm/hitobject/Note/Note.h"

OsuFileReader::OsuFileReader() {};

// 格式化行
void OsuFileReader::parse_line(const std::string& line) {
  if (line[0] == '[' && line.back() == ']') {
    current_chapter = line.substr(1, line.size() - 2);
  } else {
    //[Events]	谱面显示设定，故事板事件	逗号分隔的列表
    //[TimingPoints]	时间轴设定	逗号分隔的列表
    //[HitObjects]	击打物件	逗号分隔的列表

    if (current_chapter == "Events") {
      auto start_5_string = line.substr(0, 5);
      auto start_1_char = line.at(0);
      // XINFO(start_5_string);
      // 并非一定五个参数
      if (start_5_string == "Video") {
        map_properties[current_chapter]["background video"] = line;
      } else if (start_5_string == "Break") {
        map_properties[current_chapter]
                      ["Break" + std::to_string(current_breaks_index++)] = line;
      } else {
        // general bg
        map_properties[current_chapter]["background"] = line;
      }
      // int commas = std::count(line.begin(), line.end(), ',');
      // byd被wiki骗了，不能用逗号数区分事件类型
      // switch (commas) {
      //   case 4: {
      //     map_properties[current_chapter]["background"] = line;
      //     break;
      //   }
      //   case 2: {
      //     map_properties[current_chapter]
      //                   [std::to_string(current_breaks_index++)] = line;
      //     break;
      //   }
      //   default:
      //     map_properties[current_chapter]["unknown"] = line;
      // }
    } else if (current_chapter == "TimingPoints") {
      map_properties[current_chapter][std::to_string(current_timing_index++)] =
          line;
    } else if (current_chapter == "HitObjects") {
      map_properties[current_chapter]
                    [std::to_string(current_hitobject_index++)] = line;
    } else {
      size_t eq_pos = line.find(':');
      if (eq_pos != std::string::npos) {
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        map_properties[current_chapter][key] = value;
      }
    }
  }
}

OsuMap::OsuMap() : MMap() { maptype = MapType::OSUMAP; }

OsuMap::~OsuMap() = default;

// 从文件读取谱面
void OsuMap::load_from_file(const char* path) {
  std::filesystem::path p(path);
  auto fname = p.filename();
  // XINFO("文件名:" + fname.string());
  if (p.extension() == ".osu") {
    // XINFO("load_osu:" + p.extension().string());
    std::ifstream ifs(p);
    if (!ifs.is_open()) {
      XERROR("打开文件[" + p.string() + "]失败");
      return;
    }
    OsuFileReader osureader;
    std::string read_buffer;
    std::getline(ifs, read_buffer);
    auto cpos = read_buffer.find("format");
    if (cpos != std::string::npos) {
      // 取出osu版本
      auto vnum = read_buffer.substr(cpos + 8, read_buffer.size() - 1);
      file_format_version = std::stoi(vnum);
    } else {
      // 清除可能的 EOF 标志
      ifs.clear();
      // 回到文件开头
      ifs.seekg(0, std::ios::beg);
    }
    while (std::getline(ifs, read_buffer)) {
      if (!read_buffer.empty() && read_buffer.back() == '\r') {
        read_buffer.pop_back();
      }
      // XINFO(read_buffer);
      if (  // 未读到内容
          read_buffer.empty() ||
          // 直接结束
          read_buffer[0] == ';' ||
          // 注释
          (read_buffer[0] == '/' && read_buffer[1] == '/'))
        continue;
      osureader.parse_line(read_buffer);
    }
    // XINFO("parse result:");
    // for (const auto& [chapter, properties_map] : osureader.map_properties) {
    //   XINFO("-----------chapter:" + chapter + "---------------");
    //   for (const auto& [key, value] : properties_map) {
    //     XINFO("key:" + key + "-->value:" + value);
    //   }
    // }

    // 初始化map

    // general
    AudioFilename =
        osureader.get_value("General", "AudioFilename", std::string(""));
    AudioLeadIn = osureader.get_value("General", "AudioLeadIn", 0);
    AudioHash = osureader.get_value("General", "AudioHash", std::string(""));
    PreviewTime = osureader.get_value("General", "PreviewTime", -1);
    Countdown = osureader.get_value("General", "Countdown", 1);
    sample_set =
        static_cast<SampleSet>(osureader.get_value("General", "SampleSet", 1));
    StackLeniency = osureader.get_value("General", "StackLeniency", 0.0);
    LetterboxInBreaks =
        osureader.get_value("General", "LetterboxInBreaks", false);
    StoryFireInFront = osureader.get_value("General", "StoryFireInFront", true);
    UseSkinSprites = osureader.get_value("General", "UseSkinSprites", false);
    AlwaysShowPlayfield =
        osureader.get_value("General", "AlwaysShowPlayfield", false);
    OverlayPosition = osureader.get_value("General", "OverlayPosition",
                                          std::string("NoChange"));
    SkinPreference =
        osureader.get_value("General", "SkinPreference", std::string(""));
    EpilepsyWarning = osureader.get_value("General", "EpilepsyWarning", false);
    CountdownOffset = osureader.get_value("General", "CountdownOffset", 0);
    SpecialStyle = osureader.get_value("General", "SpecialStyle", false);
    WidescreenStoryboard =
        osureader.get_value("General", "WidescreenStoryboard", false);
    SamplesMatchPlaybackRate =
        osureader.get_value("General", "SamplesMatchPlaybackRate", false);

    // editor
    // Bookmarks	逗号分隔的 Integer（整型）数组
    // 书签（蓝线）的位置（毫秒）
    auto marks = osureader.get_value("Editor", "Bookmarks", std::string(""));
    std::istringstream iss(marks);
    std::string token;
    while (std::getline(iss, token, ',')) {
      // 移除首尾空格
      token.erase(token.begin(),
                  std::find_if(token.begin(), token.end(),
                               [](int ch) { return !std::isspace(ch); }));
      token.erase(std::find_if(token.rbegin(), token.rend(),
                               [](int ch) { return !std::isspace(ch); })
                      .base(),
                  token.end());
      Bookmarks.push_back(std::stoi(token));
    }
    DistanceSpacing = osureader.get_value("Editor", "DistanceSpacing", 0.0);
    BeatDivisor = osureader.get_value("Editor", "BeatDivisor", 0);
    GridSize = osureader.get_value("Editor", "GridSize", 0);
    TimelineZoom = osureader.get_value("Editor", "TimelineZoom", 0.0);

    // metadata
    Title = osureader.get_value("Metadata", "Title", std::string(""));
    TitleUnicode =
        osureader.get_value("Metadata", "TitleUnicode", std::string(""));
    Artist = osureader.get_value("Metadata", "Artist", std::string(""));
    ArtistUnicode =
        osureader.get_value("Metadata", "ArtistUnicode", std::string(""));
    Creator = osureader.get_value("Metadata", "Creator", std::string("mmm"));
    Version = osureader.get_value("Metadata", "Version", std::string("[mmm]"));
    Source = osureader.get_value("Metadata", "Source", std::string(""));
    // ***Tags	空格分隔的 String（字符串）数组	易于搜索的标签
    auto tags = osureader.get_value("Editor", "Tags", std::string(""));
    std::istringstream tiss(tags);
    while (std::getline(tiss, token, ' ')) {
      Tags.push_back(token);
    }
    BeatmapID = osureader.get_value("Metadata", "BeatmapID", -1);
    BeatmapSetID = osureader.get_value("Metadata", "BeatmapSetID", -1);

    // difficulty
    HPDrainRate = osureader.get_value("Difficulty", "HPDrainRate", 5.0);
    CircleSize = osureader.get_value("Difficulty", "CircleSize", 4.0);
    OverallDifficulty =
        osureader.get_value("Difficulty", "OverallDifficulty", 8.0);
    ApproachRate = osureader.get_value("Difficulty", "ApproachRate", 0.0);
    SliderMultiplier =
        osureader.get_value("Difficulty", "SliderMultiplier", 0.0);
    SliderTickRate = osureader.get_value("Difficulty", "SliderTickRate", 0.0);

    // colour--- 不写

    // event
    // bg
    auto background_des = osureader.get_value(
        "Events", "background", std::string("0,0,\"bg.png\",0,0"));
    std::istringstream biss(background_des);
    std::vector<std::string> background_paras;
    while (std::getline(biss, token, ',')) {
      background_paras.emplace_back(token);
    }
    if (background_paras.at(0) == "0") {
      // 是图片
      background_type = 0;
    } else {
      // 是视频
      background_type = 1;
    }
    bg_file_name = background_paras.at(2);
    if (background_paras.size() >= 5) {
      bgxoffset = std::stoi(background_paras.at(3));
      bgyoffset = std::stoi(background_paras.at(4));
    } else {
      bgxoffset = 0;
      bgyoffset = 0;
    }

    // breaks
    for (int i = 0; i < osureader.current_breaks_index; i++) {
      auto breaks_des = osureader.get_value("Events", std::to_string(i),
                                            std::string("2,0,0"));
      std::istringstream breaksiss(breaks_des);
      std::vector<std::string> breaks_paras;
      while (std::getline(breaksiss, token, ',')) {
        breaks_paras.emplace_back(token);
      }
      // 添加一个休息段
      breaks.emplace_back(std::stoi(breaks_paras.at(1)),
                          std::stoi(breaks_paras.at(2)));
    }

    // 创建timing
    for (int i = 0; i < osureader.current_timing_index; i++) {
      // 按顺序读取timing点
      auto timing_point_des =
          osureader.get_value("TimingPoints", std::to_string(i),
                              std::string("10000,333.33,4,0,0,100,1,1"));
      std::istringstream timingiss(timing_point_des);
      std::vector<std::string> timing_point_paras;
      while (std::getline(timingiss, token, ',')) {
        timing_point_paras.emplace_back(token);
      }
      // 创建timing
      auto osu_timing = std::make_shared<OsuTiming>();
      // 使用读取出的参数初始化timing
      osu_timing->from_osu_description(timing_point_paras);
      // 加入timing列表
      timings.emplace_back(osu_timing);
      auto temp_timing_list_it = temp_timing_map.find(osu_timing->timestamp);
      if (temp_timing_list_it == temp_timing_map.end()) {
        // 添加映射
        temp_timing_list_it =
            temp_timing_map.try_emplace(osu_timing->timestamp).first;
      }
      temp_timing_list_it->second.emplace_back(osu_timing);
    }

    // 创建物件
    for (int i = 0; i < osureader.current_hitobject_index; i++) {
      // 按顺序读取物件
      auto note_des =
          osureader.get_value("HitObjects", std::to_string(i),
                              std::string("469,192,1846,1,0,0:0:0:0:"));
      std::istringstream noteiss(note_des);
      std::vector<std::string> note_paras;
      while (std::getline(noteiss, token, ',')) {
        note_paras.emplace_back(token);
      }
      std::shared_ptr<Note> osu_note;
      // 创建物件
      if (std::stoi(note_paras.at(3)) == 128) {
        osu_note = std::make_shared<OsuHold>();
        auto hold = std::dynamic_pointer_cast<OsuHold>(osu_note);
        // 使用读取出的参数初始化物件
        hold->from_osu_description(note_paras, CircleSize);
        // 创建一个面尾物件
        auto holdend = std::make_shared<OsuHoldEnd>(hold);
        holdend->is_hold_end = true;
        hitobjects.emplace_back(holdend);

        // 把长条物件加入缓存
        temp_hold_list.emplace_back(hold);
      } else {
        osu_note = std::make_shared<OsuNote>();
        auto note = std::dynamic_pointer_cast<OsuNote>(osu_note);
        // 使用读取出的参数初始化物件
        note->from_osu_description(note_paras, CircleSize);
      }
      // 加入物件列表
      hitobjects.emplace_back(osu_note);
    }

    std::sort(hitobjects.begin(), hitobjects.end(),
              [](std::shared_ptr<HitObject>& ho1,
                 std::shared_ptr<HitObject>& ho2) { return *ho1 < *ho2; });
  } else {
    XWARN("非.osu格式,读取失败");
  }
}

// 查询指定位置附近的同时间点timing列表(优先在此之前,没有之前找之后)
void OsuMap::query_around_timing(
    std::vector<std::shared_ptr<Timing>>& result_timings, int32_t time) {
  if (timings.empty()) return;
  // 查找到第一个大于等于此时间的timing迭代器
  auto it = std::upper_bound(
      timings.begin(), timings.end(), time,
      [](int time, const auto& timing) { return timing->timestamp >= time; });

  // 确定使用的timing
  std::shared_ptr<Timing> res;
  if (it == result_timings.end()) {
    // 没找到比当前时间靠后的timing
    // 使用最后一个timing
    res = result_timings.at(timings.size() - 1);

  } else {
    // 找到了比当前时间靠后的timing
    if (*it == timings.front()) {
      // 找到的是第一个
      // 就使用这个timing
      res = *it;
    } else {
      // 使用前一个
      res = *(it - 1);
    }
  }

  // 查找重复时间点的timing表
  auto timing_list_it = temp_timing_map.find(res->timestamp);

  // 添加到传入引用
  for (const auto& timing : timing_list_it->second) {
    result_timings.emplace_back(timing);
  }
}

// 查询区间内有的物件
void OsuMap::query_object_in_range(
    std::vector<std::shared_ptr<HitObject>>& result_objects, int32_t start,
    int32_t end) {
  int count = 0;
  // 查找到第一个大于等于起始时间的物件迭代器
  auto it_start = std::upper_bound(
      hitobjects.begin(), hitobjects.end(), start,
      [](int32_t time, const auto& obj) { return obj->timestamp >= time; });

  // 查找到第一个小于等于结束时间的物件迭代器
  auto it_end = std::lower_bound(
      hitobjects.begin(), hitobjects.end(), end,
      [](const auto& obj, int time) { return obj->timestamp <= time; });

  // 先将面尾时间大于起始时间的物件也加入列表
  for (const auto& hold : temp_hold_list) {
    if (hold->timestamp <= end &&
        hold->hold_end_reference->timestamp >= start) {
      result_objects.emplace_back(hold);
      count++;
    }
  }

  // 将区间内物件加入结果
  for (; it_start < it_end; it_start++) {
    // 查重
    auto note = std::static_pointer_cast<OsuNote>((*it_start));
    bool added{false};
    if (note->type == NoteType::HOLD) {
      for (const auto& obj : result_objects)
        if (obj.get() == note.get()) added = true;
    }
    if (!added) {
      result_objects.emplace_back(note);
      count++;
    }
  }
}
