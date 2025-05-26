#include "MMap.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../../ui/mainwindow.h"
#include "../../util/mutil.h"
#include "../MapWorkProject.h"
#include "../hitobject/HitObject.h"
#include "../hitobject/Note/HoldEnd.h"
#include "../hitobject/Note/Note.h"
#include "../hitobject/Note/rm/ComplexNote.h"
#include "../hitobject/Note/rm/Slide.h"
#include "../timing/Timing.h"
#include "colorful-log.h"
#include "mmm/Metadata.h"
#include "osu/OsuMap.h"
#include "rm/RMMap.h"

// 线程池
ThreadPool MMap::map_pool = ThreadPool(4);

MMap::MMap() {
    // 初始化播放回调对象
    audio_pos_callback = std::make_shared<AudioEnginPlayCallback>();
}

MMap::~MMap() = default;

void write_note_json(json& notes_json, std::shared_ptr<HitObject> o) {
    auto note = std::dynamic_pointer_cast<Note>(o);

    if (note) {
        json note_json;
        note_json["time"] = note->timestamp;
        note_json["orbit"] = note->orbit;
        note_json["complex-info"] = static_cast<int>(note->compinfo);
        std::string type_name;
        switch (note->note_type) {
            case NoteType::NOTE: {
                type_name = "note";
                break;
            }
            case NoteType::HOLD: {
                auto hold = std::static_pointer_cast<Hold>(note);
                note_json["duration"] = hold->hold_time;
                type_name = "hold";
                break;
            }
            case NoteType::SLIDE: {
                auto slide = std::static_pointer_cast<Slide>(note);
                note_json["orbit-alt"] = slide->slide_parameter;
                type_name = "slide";
                break;
            }
            default: {
                type_name = "";
                break;
            }
        }
        note_json["type"] = type_name;

        // 元数据
        auto& metas_json = note_json["metas"];
        for (const auto& [type, metadata] : note->metadatas) {
            std::string type_name;
            switch (type) {
                case NoteMetadataType::NOSU: {
                    type_name = "osu";
                    break;
                }
                case NoteMetadataType::NMALODY: {
                    type_name = "malody";
                    break;
                }
            }
            auto& meta_json = metas_json[type_name];
            for (const auto& [key, value] : metadata->note_properties) {
                meta_json[key] = value;
            }
        }

        notes_json.push_back(note_json);

        // XINFO("writed_note:" + note_json.dump());
    }
}
// 从文件读取谱面
void MMap::load_from_file(const char* path) {
    auto s = std::string(path);
    if (s.ends_with(".mmm")) {
        try {
            map_file_path = std::filesystem::path(path);
            // 打开文件流
            std::ifstream input_file(path);
            // 解析JSON数据
            json mapdata_json = json::parse(input_file);
            // 读取json
            title = mapdata_json["title"];
            title_unicode = mapdata_json["title-unicode"];
            artist = mapdata_json["artist"];
            artist_unicode = mapdata_json["artist-unicode"];
            version = mapdata_json["version"];
            author = mapdata_json["author"];
            preference_bpm = mapdata_json["preference-bpm"];
            map_length = mapdata_json["maplength"];
            orbits = mapdata_json["orbits"];
            audio_file_rpath =
                std::filesystem::path(mapdata_json["music"].get<std::string>());
            audio_file_abs_path =
                map_file_path.parent_path() / audio_file_rpath;

            bg_rpath =
                std::filesystem::path(mapdata_json["bg"].get<std::string>());
            bg_path = map_file_path.parent_path() / bg_rpath;

            map_name = "[mmm] " + artist_unicode + " - " + title_unicode +
                       " [" + std::to_string(orbits) + "k] " + "[" + version +
                       "]";

            // 缓存父类指针
            std::shared_ptr<Note> temp_note;
            // 缓存组合键指针
            std::shared_ptr<ComplexNote> temp_complex_note;

            // 先读取物件
            auto notes_json = mapdata_json["notes"];
            for (const auto& note_json : notes_json) {
                // std::cout << note_json.dump() << "\n";
                auto type = note_json.value<std::string>("type", "note");
                if (type == "note") {
                    temp_note = std::make_shared<Note>(
                        note_json.value<int32_t>("time", 0),
                        note_json.value<int32_t>("orbit", 0));
                } else {
                    if (type == "hold") {
                        temp_note = std::make_shared<Hold>(
                            note_json.value<int32_t>("time", 0),
                            note_json.value<int32_t>("orbit", 0),
                            note_json.value<int32_t>("duration", 0));
                        auto hold = std::static_pointer_cast<Hold>(temp_note);
                        auto hold_end = std::make_shared<HoldEnd>(hold);
                        hold->hold_end_reference = hold_end;
                        temp_hold_list.insert(hold);
                        hitobjects.insert(hold_end);
                    } else if (type == "slide") {
                        temp_note = std::make_shared<Slide>(
                            note_json.value<int32_t>("time", 0),
                            note_json.value<int32_t>("orbit", 0),
                            note_json.value<int32_t>("orbit-alt", 1));
                        auto slide = std::static_pointer_cast<Slide>(temp_note);
                        auto slide_end = std::make_shared<SlideEnd>(slide);
                        slide->slide_end_reference = slide_end;
                        hitobjects.insert(slide_end);
                    }
                }

                // 处理组合键
                temp_note->compinfo = static_cast<ComplexInfo>(
                    note_json.value<uint8_t>("complex-info", 0));
                std::shared_ptr<Note> prechild = nullptr;

                switch (temp_note->compinfo) {
                    case ComplexInfo::HEAD: {
                        if (prechild) {
                            // 存在非法组合键开始
                            // 清空之前的缓存引用
                            prechild->parent_reference->child_notes.clear();
                            prechild->parent_reference = nullptr;
                            hitobjects.erase(prechild);
                        }
                        temp_complex_note = std::make_shared<ComplexNote>(
                            temp_note->timestamp, temp_note->orbit);
                        // 设置父物件
                        temp_note->parent_reference = temp_complex_note.get();
                        temp_complex_note->child_notes.insert(temp_note);
                        prechild = temp_note;
                        break;
                    }
                    case ComplexInfo::BODY: {
                        // 设置父物件
                        if (!temp_complex_note) continue;

                        temp_note->parent_reference = temp_complex_note.get();
                        temp_complex_note->child_notes.insert(temp_note);
                        prechild = temp_note;
                        break;
                    }
                    case ComplexInfo::END: {
                        if (!temp_complex_note) continue;
                        temp_complex_note->child_notes.insert(temp_note);
                        temp_note->parent_reference = temp_complex_note.get();
                        hitobjects.insert(temp_complex_note);
                        prechild = nullptr;
                        temp_complex_note.reset();
                        break;
                    }
                    default:
                        break;
                }

                hitobjects.insert(temp_note);

                if (hitobjects.find(temp_note) == hitobjects.end()) {
                    // 不重复添加物件
                }

                // 物件元数据
                auto& metas_json = note_json["metas"];
                for (const auto& [type, metajson] : metas_json.items()) {
                    // 注册物件的元数据并填入
                    if (type == "osu") {
                        auto& osumeta =
                            temp_note->metadatas[NoteMetadataType::NOSU];
                        osumeta = std::make_shared<NoteMetadata>();
                        for (const auto& [key, value] : metajson.items()) {
                            osumeta->note_properties[key] =
                                value.get<std::string>();
                        }
                    } else if (type == "malody") {
                        auto& malodymeta =
                            temp_note->metadatas[NoteMetadataType::NMALODY];
                        for (const auto& [key, value] : metajson.items()) {
                            malodymeta->note_properties[key] =
                                value.get<std::string>();
                        }
                    }
                }
            }

            // 读取时间点数据
            auto& timings_json = mapdata_json["timings"];
            for (const auto& timing_json : timings_json) {
                auto timing = std::make_shared<Timing>();
                timing->timestamp = timing_json["time"];
                timing->type = TimingType::GENERAL;
                timing->is_base_timing = timing_json["isbase"];
                timing->basebpm = timing_json["basebpm"];
                timing->bpm = timing_json["bpm"];
                insert_timing(timing);
            }
            input_file.close();
        } catch (std::exception e) {
            std::cerr << e.what() << "\n";
        }
    }
}
// 初始化备份
void MMap::init_backups() {
    // 读取备份目录
    auto subdir = mutil::sanitizeFilename(title_unicode + "_" + version);
    auto bkup_file_path = project_reference->ppath /
                          MainWindow::settings.backup_relative_path / subdir;
    if (!std::filesystem::exists(bkup_file_path)) {
        try {
            std::filesystem::create_directories(bkup_file_path);
            XINFO("已创建备份谱面目录[" + bkup_file_path.generic_string() +
                  "]");
        } catch (std::exception e) {
            XERROR("无法创建备份谱面目录[" + bkup_file_path.generic_string() +
                   "]")
        }
    } else {
        // 读取目录内文件名到备份队列
        for (const auto& entry :
             std::filesystem::directory_iterator(bkup_file_path)) {
            auto file_abspath = std::filesystem::weakly_canonical(
                std::filesystem::absolute(entry.path()));
            if (std::filesystem::is_regular_file(file_abspath) &&
                file_abspath.extension() == ".bak") {
                map_backup_paths_queue.push_back(file_abspath.generic_string());
            }
        }
    }
}

// 写出到文件
void MMap::write_to_file(const char* path) {
    // 根据文件后缀决定如何转换
    auto p = std::filesystem::path(path);
    if (mutil::endsWithExtension(p, ".mmm") ||
        mutil::endsWithExtension(p, ".bak")) {
        // 写出为mmm-json
        //
        // 更新json
        //
        // 谱面数据json
        json mapdata_json;

        // 基本共通数据
        mapdata_json["title"] = title;
        mapdata_json["title-unicode"] = title_unicode;
        mapdata_json["artist"] = artist;
        mapdata_json["artist-unicode"] = artist_unicode;
        mapdata_json["version"] = version;
        mapdata_json["author"] = author;
        mapdata_json["preference-bpm"] = preference_bpm;
        mapdata_json["maplength"] = map_length;
        mapdata_json["orbits"] = orbits;
        mapdata_json["music"] = audio_file_rpath.generic_string();
        mapdata_json["bg"] = bg_rpath.generic_string();

        // 时间点数据
        auto& timings_json = mapdata_json["timings"];

        int index = 0;
        for (const auto& timing : timings) {
            auto& timing_json = timings_json[index];
            timing_json["time"] = timing->timestamp;
            timing_json["isbase"] = timing->is_base_timing;
            timing_json["basebpm"] = timing->basebpm;
            timing_json["bpm"] = timing->bpm;
            // TODO(xiang 2025-05-19): timing的元数据实现和保存
            ++index;
        }

        write_note_count = 0;

        // 物件数据
        auto notes_json = json::array();

        // 防止重复写出同一物件
        std::unordered_map<std::shared_ptr<HitObject>, bool> writed_object;
        for (const auto& hitobject : hitobjects) {
            // 筛除面尾滑尾和包含组合键引用的物件和重复物件
            // 优先写出完整组合键
            if (!hitobject->is_note) continue;
            auto note = std::static_pointer_cast<Note>(hitobject);
            // 不写出处于组合键内的物件
            if (!note || note->parent_reference) continue;
            if (writed_object.find(note) == writed_object.end())
                writed_object.insert({note, true});
            else
                continue;
            if (note->note_type == NoteType::COMPLEX) {
                auto comp = std::static_pointer_cast<ComplexNote>(note);
                // 直接写出所有子键
                for (auto& child_note : comp->child_notes) {
                    // if (child_note->timestamp == 25571) {
                    //     std::cout << child_note->timestamp << "\n";
                    // }
                    write_note_json(notes_json, child_note);
                    ++write_note_count;
                    writed_object.insert({child_note, true});
                }
            } else {
                write_note_json(notes_json, note);
                ++write_note_count;
            }
        }
        mapdata_json["notes"] = notes_json;
        XINFO("mmm write_note_count:" + std::to_string(write_note_count));

        //
        // 写出到文件
        // 打开文件输出流-覆盖模式
        std::ofstream out(path);
        out << mapdata_json.dump(4);
        out.close();
    } else if (mutil::endsWithExtension(p, ".imd")) {
        // 转化为rmmap并写出
        auto rmmap =
            std::make_shared<RMMap>(std::shared_ptr<MMap>(this, [](MMap*) {}));
        rmmap->write_to_file(path);
    } else if (mutil::endsWithExtension(p, ".osu")) {
        // 转化为osumap并写出
        auto omap =
            std::make_shared<OsuMap>(std::shared_ptr<MMap>(this, [](MMap*) {}));
        omap->write_to_file(path);
    }
}

// 写出文件是否合法
bool MMap::is_write_file_legal(const char* file, std::string& res) {
    return true;
}

// 是否包含物件
bool MMap::contains_obj(std::shared_ptr<HitObject> o) { return false; }

// 注册元数据
void MMap::register_metadata(MapMetadataType type) {
    switch (type) {
        case MapMetadataType::MIMD: {
            metadatas[type] = RMMap::default_metadata();
            break;
        }
        case MapMetadataType::MOSU: {
            metadatas[type] = OsuMap::default_metadata();
            break;
        }
        default:
            break;
    }
}

// 插入物件
void MMap::insert_hitobject(std::shared_ptr<HitObject> hitobject) {
    // 普通物件
    auto note = std::dynamic_pointer_cast<Note>(hitobject);
    if (note) {
        switch (note->note_type) {
            case NoteType::NOTE: {
                hitobjects.insert(note);
                break;
            }
            case NoteType::SLIDE: {
                // 滑键
                // 放入主区,并在主区添加面尾物件
                auto insert_slide = std::static_pointer_cast<Slide>(note);
                hitobjects.insert(insert_slide);
                hitobjects.insert(insert_slide->slide_end_reference);
                break;
            }
            case NoteType::HOLD: {
                // 面条
                auto insert_hold = std::static_pointer_cast<Hold>(note);
                // 放入主区,缓冲区,并在主区添加面尾物件
                hitobjects.insert(insert_hold);
                temp_hold_list.insert(insert_hold);
                hitobjects.insert(insert_hold->hold_end_reference);
                break;
            }
            case NoteType::COMPLEX: {
                // 组合键
                auto insert_complex =
                    std::static_pointer_cast<ComplexNote>(note);
                for (const auto& child_note : insert_complex->child_notes) {
                    // 递归插入所有子物件
                    insert_hitobject(child_note);
                }
                // 然后添加自身
                hitobjects.insert(insert_complex);

                break;
            }
        }
    } else {
        // 非note的其他物件
        hitobjects.insert(hitobject);
    }
}

// 移除物件
void MMap::remove_hitobject(std::shared_ptr<HitObject> hitobject) {
    auto note = std::dynamic_pointer_cast<Note>(hitobject);
    if (note) {
        // 添加相同物件的迭代器到数组
        auto insert_same_objs =
            [&](std::vector<decltype(hitobjects.begin())>& to_erase,
                std::shared_ptr<HitObject> obj) {
                auto it = hitobjects.lower_bound(obj);
                // 前移到上一时间戳
                while (it != hitobjects.begin() &&
                       obj->timestamp - it->get()->timestamp < 10) {
                    --it;
                }

                // 添加即将删除迭代器
                while (it != hitobjects.end() &&
                       (it->get()->timestamp - obj->timestamp) < 5) {
                    if (it->get()->equals(obj)) {
                        to_erase.push_back(it);
                    }
                    ++it;
                }
            };
        // 添加相同物件的迭代器到数组
        auto insert_same_tempobjs =
            [&](std::vector<decltype(temp_hold_list.begin())>& to_erase,
                std::shared_ptr<Hold> obj) {
                auto it = temp_hold_list.lower_bound(obj);
                // 前移到上一时间戳
                while (it != temp_hold_list.begin() &&
                       obj->timestamp - it->get()->timestamp < 10) {
                    --it;
                }

                // 添加即将删除迭代器
                while (it != temp_hold_list.end() &&
                       (it->get()->timestamp - obj->timestamp) < 5) {
                    if (it->get()->equals(obj)) {
                        to_erase.push_back(it);
                    }
                    ++it;
                }
            };
        // 从主集合移除
        auto remove_from_main =
            [&](std::vector<decltype(hitobjects.begin())>& to_erase) {
                // 从后往前删除，避免迭代器失效
                for (auto rit = to_erase.rbegin(); rit != to_erase.rend();
                     ++rit) {
                    XWARN(QString("移除map源物件:t[%1]")
                              .arg((*rit)->get()->timestamp)
                              .toStdString());
                    hitobjects.erase(*rit);
                }
            };
        // 从缓存面条集合移除
        auto remove_from_bufferholds =
            [&](std::vector<decltype(temp_hold_list.begin())>& to_erase) {
                // 从后往前删除，避免迭代器失效
                for (auto rit = to_erase.rbegin(); rit != to_erase.rend();
                     ++rit) {
                    XWARN(QString("移除map源物件:t[%1]")
                              .arg((*rit)->get()->timestamp)
                              .toStdString());
                    temp_hold_list.erase(*rit);
                }
            };

        switch (note->note_type) {
            case NoteType::NOTE: {
                // 删除单键-可能的重叠物件一起删
                auto insert_slide = std::static_pointer_cast<Slide>(note);
                // 存储要删除的迭代器（可能有多个匹配项）
                std::vector<decltype(hitobjects.begin())> to_erase;
                insert_same_objs(to_erase, note);
                remove_from_main(to_erase);

                break;
            }
            case NoteType::SLIDE: {
                // 滑键
                // 从主区移除,并在主区移除对应面尾物件
                auto insert_slide = std::static_pointer_cast<Slide>(note);

                // 存储要删除的迭代器（可能有多个匹配项）
                std::vector<decltype(hitobjects.begin())> to_erase;

                // 添加滑头和滑尾
                insert_same_objs(to_erase, insert_slide);
                // 从主集合移除物件
                remove_from_main(to_erase);

                to_erase.clear();
                insert_same_objs(to_erase, insert_slide->slide_end_reference);
                // 从主集合移除物件
                remove_from_main(to_erase);
                break;
            }
            case NoteType::HOLD: {
                // 面条
                auto remove_hold = std::static_pointer_cast<Hold>(note);
                // 从主区和缓冲区移除,并在主区移除面尾物件
                // 存储要删除的迭代器（可能有多个匹配项）
                std::vector<decltype(hitobjects.begin())> to_erase;

                // 添加面头和面尾
                insert_same_objs(to_erase, remove_hold);
                // 从主集合移除物件
                remove_from_main(to_erase);

                // 防止迭代器失效
                to_erase.clear();
                insert_same_objs(to_erase, remove_hold->hold_end_reference);
                // 从主集合移除物件
                remove_from_main(to_erase);

                // 面条缓存
                std::vector<decltype(temp_hold_list.begin())> to_erase_temp;
                insert_same_tempobjs(to_erase_temp, remove_hold);
                remove_from_bufferholds(to_erase_temp);

                break;
            }
            case NoteType::COMPLEX: {
                // 组合键
                auto remove_complex =
                    std::static_pointer_cast<ComplexNote>(note);
                for (const auto& child_note : remove_complex->child_notes) {
                    // 递归移除所有子物件
                    remove_hitobject(child_note);
                }
                // 然后移除自身
                // 存储要删除的迭代器（可能有多个匹配项）
                std::vector<decltype(hitobjects.begin())> to_erase;
                insert_same_objs(to_erase, remove_complex);
                remove_from_main(to_erase);
                break;
            }
        }
    } else {
        hitobjects.erase(hitobject);
    }
}

// 执行操作
void MMap::execute_edit_operation(ObjEditOperation& operation) {
    std::lock_guard<std::mutex> lock(hitobjects_mutex);

    bool remove_parent{false};
    // 移除src的物件
    if (!operation.src_objects.empty())
        for (const auto& srcobj : operation.src_objects) {
            remove_hitobject(srcobj);
        }

    // 插入des的物件
    if (!operation.des_objects.empty()) {
        for (const auto& obj : operation.des_objects) {
            // 添加处若有完全相同的物件-取消此次添加
            bool add{true};
            auto it = hitobjects.lower_bound(obj);
            // 前移到上一时间戳
            if (it != hitobjects.begin() && it == hitobjects.end()) --it;
            while (it != hitobjects.begin() &&
                   obj->timestamp - it->get()->timestamp < 10)
                --it;
            while (it != hitobjects.end() &&
                   (it->get()->timestamp - obj->timestamp) < 5) {
                if (obj->equals(*it)) {
                    add = false;
                    break;
                }
                ++it;
            }
            if (add) {
                insert_hitobject(obj);
                XWARN(QString("添加map目标物件:t[%1]")
                          .arg(obj->timestamp)
                          .toStdString());
            } else {
                XWARN(QString("t[%1]已有物件,跳过添加")
                          .arg(obj->timestamp)
                          .toStdString());
            }
        }
    }
}

void MMap::execute_edit_operation(TimingEditOperation& operation) {}

// 生成此拍的分拍策略
void MMap::generate_divisor_policy(const std::shared_ptr<Beat>& beat) {
    // 计算这拍之内的可能分拍策略
    // 获取此拍范围内的物件
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> notes;
    query_object_in_range(notes, beat->start_timestamp, beat->end_timestamp);

    if (notes.empty()) {
        // 没有物件,默认1分
        beat->divisors = 1;
    } else {
        std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> objs;
        for (const auto& note : notes) {
            // 设置拍引用
            note->beatinfo = beat;
            // set自动排序
            objs.insert(note);
        }

        // 允许5ms误差
        beat->divisors = mutil::calculateDivisionStrategy(objs, beat, 5);
    }
}

// 擦除指定范围内的拍
void MMap::erase_beats(double start, double end) {
    if (beats.empty()) return;

    // 找到此时间区间内的第一拍位置
    auto beat_lower_bound = std::make_shared<Beat>(200, start, start);
    // 找到第一个大于或等于 lower_bound 的元素
    auto beat_startit = beats.lower_bound(beat_lower_bound);

    // 找到此时间区间内的最后一拍位置
    auto beat_upper_bound = std::make_shared<Beat>(200, end, end);
    // 找到第一个严格大于 upper_bound 的元素
    auto beat_endit = beats.upper_bound(beat_upper_bound);
    // 移除此区间全部的beat
    // 确保 beat_endit 不是 begin()，否则 -- 会越界
    if (beat_endit != beats.begin()) {
        beat_endit--;  // 现在指向最后一个 ≤ end 的 Beat
    } else {
        // 所有 Beat 都 > end，区间内无 Beat 可删
        return;
    }
    auto start_legal = beat_startit != beats.end();
    if (start_legal) {
        auto start_less_than_end =
            *beat_startit != *beat_endit && **beat_startit <= **beat_endit;
        // 确保 beat_startit ≤ beat_endit（否则区间无效）
        if (start_less_than_end) {
            // 删除 [beat_startit, beat_endit] 区间内的所有 Beat
            beats.erase(beat_startit, ++beat_endit);  // erase 是 [first, last)
        }
    }
}

// 有序的添加timing-会分析并更新拍
void MMap::insert_timing(const std::shared_ptr<Timing>& timing) {
    // 加入timing列表
    auto insertit = timings.insert(timing);
    ++insertit;

    auto temp_timing_list_it = temp_timing_map.find(timing->timestamp);
    if (temp_timing_list_it == temp_timing_map.end()) {
        // 添加映射
        temp_timing_list_it =
            temp_timing_map.try_emplace(timing->timestamp).first;
    }
    temp_timing_list_it->second.emplace_back(timing);

    // 更新此timing时间开始到下一基准timing之前的拍/之后没有就一直到maplegnth
    std::shared_ptr<Timing> next_base_timing = nullptr;
    while (insertit != timings.end()) {
        if ((*insertit)->is_base_timing) {
            next_base_timing = *insertit;
            break;
        }
        ++insertit;
    }

    // 区分变速timing和基准timing
    if (timing->is_base_timing) {
        // 是基准timing--音乐bpm变化
        // 界定处理时间范围
        auto start = timing->timestamp;
        uint32_t end;

        // 区分有无
        if (next_base_timing) {
            end = next_base_timing->timestamp;
        } else {
            end = map_length;
        }

        // 擦除范围内的拍
        erase_beats(start, end);

        // 当前处理到的时间
        auto current_process_time = double(start);
        // 每一拍的时间
        double beattime = 60.0 / timing->basebpm * 1000.0;

        // 获取此时间范围内的timing,可能有变速timing
        std::vector<std::shared_ptr<Timing>> src_timings;
        query_timing_in_range(src_timings, start, end);

        if (!src_timings.empty()) {
            // 范围内有其他timing
            auto it = src_timings.begin();
            // 这里都是变速timing了
            while (current_process_time < end) {
                // 每一拍
                auto beat =
                    std::make_shared<Beat>(timing->bpm, current_process_time,
                                           current_process_time + beattime);
                if (beat->start_timestamp >= (*it)->timestamp) {
                    if (it + 1 != src_timings.end()) {
                        // 区间内当前变速timing后面还有变速timing
                        if (beat->start_timestamp < (*(it + 1))->timestamp) {
                            // 此拍在此变速timing后在下一变速timing前-使用此timing的时间线缩放
                            beat->timeline_zoom = (*it)->bpm;
                        } else {
                            // 到了下一个变速timing后
                            // 更新一下当前使用的变速timing重来
                            it++;
                            continue;
                        }
                    } else {
                        // 之后就这一个变速timing了--使用此timing的时间线缩放
                        beat->timeline_zoom = (*it)->bpm;
                    }
                } else {
                    // 还没到变速timing位置,不使用时间线缩放--直接将此timing作为结果
                }

                // 计算这拍之内的可能分拍策略
                generate_divisor_policy(beat);
                if (beat->divisors != 1) {
                    // 锁定分拍策略
                    beat->divisors_customed = true;
                }
                // 插入beat结果
                beats.insert(beat);
                current_process_time += beattime;
            }
        } else {
            while (current_process_time < end) {
                // 每一拍
                auto beat = std::make_shared<Beat>(
                    timing->basebpm, current_process_time,
                    current_process_time + beattime);
                // 计算这拍之内的可能分拍策略
                generate_divisor_policy(beat);
                if (beat->divisors != 1) {
                    // 锁定分拍策略
                    beat->divisors_customed = true;
                }
                // 插入beat结果
                beats.insert(beat);
                current_process_time += beattime;
            }
        }
    } else {
        // 变速timing-音乐bpm不变/视觉变速(仅时间线缩放变化)
        // 获取此timing的下一timing(无论是基准还是变速)
        std::shared_ptr<Timing> next_timing = nullptr;
        if (insertit != timings.end()) {
            // 有下一个timing
            next_timing = *insertit;
        } else {
            // 没有,不管
        }
        const double start = timing->timestamp;
        double end;

        // 检查是否有
        if (next_timing) {
            // 区间为timing到下一timing之间的拍
            end = next_timing->timestamp;
        } else {
            // 没有,区间到结尾
            end = map_length;
        }

        // 生成新拍
        // 修改范围内的拍的时间线缩放
        // 当前处理到的时间
        auto beatdummy = std::make_shared<Beat>(double(start));
        auto current_process_beatit = beats.lower_bound(beatdummy);

        // 每一拍的实际时间
        double beattime = 60.0 / timing->basebpm * 1000.0;

        while ((*current_process_beatit)->end_timestamp < end) {
            // 修改时间线缩放
            (*current_process_beatit)->timeline_zoom = timing->bpm;
            ++current_process_beatit;
        }
    }
}

// 移除timing-会分析并更新拍
void MMap::remove_timing(std::shared_ptr<Timing> timing) {}

// 查询指定位置附近的timing列表
// 返回3个timing-基准timing,前一个基准timing,前一个变速timing和后一个变速timing
// 包含传入时间点时返回4个或5个(放在[3]和[4]),即此时间点同时存在基准和变速timing
void MMap::query_around_timing(
    std::vector<std::shared_ptr<Timing>>& result_timings, int32_t time) {
    if (timings.empty()) return;
    // 查找到第一个大于等于此时间的timing迭代器
    auto it = std::upper_bound(
        timings.begin(), timings.end(), time,
        [](int time, const auto& timing) { return timing->timestamp >= time; });

    // 确定使用的timing
    if (it == timings.end()) {
        // 没找到比当前时间靠后的timing
        // 使用最后一个timing
        it = timings.end();
        --it;
    } else {
        // 找到了比当前时间靠后的timing
        if (it == timings.begin()) {
            // 找到的是第一个
            // 就使用这个timing
        } else {
            // 使用前一个
            --it;
        }
    }
    std::shared_ptr<Timing> res = *it;

    // 查找重复时间点的timing表
    auto timing_list_it = temp_timing_map.find(res->timestamp);

    // 添加到传入引用
    for (const auto& timing : timing_list_it->second) {
        result_timings.emplace_back(timing);
    }
}

// 查询区间窗口内的timing
void MMap::query_timing_in_range(
    std::vector<std::shared_ptr<Timing>>& result_timings, int32_t start,
    int32_t end) {
    result_timings.clear();
    auto lower_bound = std::make_shared<Timing>();
    lower_bound->timestamp = start;

    // 找到第一个大于或等于 lower_bound 的元素
    auto it = timings.lower_bound(lower_bound);

    // 遍历直到超出 upper_bound
    while (it != timings.end() && (*it)->timestamp < end) {
        result_timings.push_back(*it);
        ++it;
    }
}

// 查询区间窗口内的timings
void MMap::query_timing_in_range(
    std::vector<std::vector<std::shared_ptr<Timing>>*>& result_timingss,
    int32_t start, int32_t end) {
    result_timingss.clear();

    // 找到第一个大于或等于 lower_bound 的元素
    auto it = temp_timing_map.lower_bound(start);

    // 遍历直到超出 upper_bound
    while (it != temp_timing_map.end() && it->first < end) {
        result_timingss.push_back(&it->second);
        ++it;
    }
}

// 查询指定时间之前的拍
std::list<std::shared_ptr<Beat>> MMap::query_beat_before_time(int32_t time) {
    if (beats.empty()) return {};

    auto beatdummy = std::make_shared<Beat>(200, time, time);
    // 找到第一个大于或等于 beatdummy 的元素
    auto it = beats.lower_bound(beatdummy);
    if (it == beats.end()) {
        // 找不到比当前时间还靠后的拍
        // 看上一拍是否存在(500ms内)
        --it;
        if (std::abs(it->get()->end_timestamp - time) < 500) {
            return {*it};
        } else {
            return {};
        }
    } else {
        if (it->get()->start_timestamp == time) {
            // 鼠标位置已经有一拍
            std::list<std::shared_ptr<Beat>> list = {*it};
            ++it;
            // 检查下一拍
            if (it == beats.end()) {
                // 不存在下一拍(最后一拍了)
            } else {
                // 添加下一拍
                list.emplace_back(*it);
            }

            return list;
        } else {
            // 找到鼠标位置后的下一拍
            if (it == beats.begin()) {
                // 下拍就已经是第一拍
                // 500ms内返回此拍-否则不返回
                if (std::abs(it->get()->start_timestamp - time) < 500) {
                    return {*it};
                } else {
                    return {};
                }
            } else {
                // 返回此拍和上一拍
                std::list<std::shared_ptr<Beat>> list = {*it};
                --it;
                list.emplace_back(*it);
                return list;
            }
        }
    }
}

// 查询时间区间窗口内的拍
void MMap::query_beat_in_range(std::vector<std::shared_ptr<Beat>>& result_beats,
                               int32_t start, const int32_t end) {
    result_beats.clear();
    auto lower_bound = std::make_shared<Beat>(200, start, start);

    // 找到第一个大于或等于 lower_bound 的元素
    auto it = beats.lower_bound(lower_bound);

    // 遍历直到超出 upper_bound
    while (it != beats.end() && (*it)->start_timestamp < end) {
        result_beats.push_back(*it);
        ++it;
    }
}

struct SharedPtrCompare {
    bool operator()(const std::shared_ptr<HitObject>& a,
                    const std::shared_ptr<HitObject>& b) const {
        // 基类有虚函数 lessThan
        return a->lessThan(b.get());
    }
};

// 查询区间窗口内有的物件
void MMap::query_object_in_range(
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>&
        result_objects,
    int32_t start, const int32_t end, bool with_longhold) {
    /*
     *lower_bound(key)	返回第一个 ≥ key 的元素的迭代器。
     *upper_bound(key)	返回第一个 > key 的元素的迭代器。
     */
    if (with_longhold) {
        // 带超级长条
        // 面尾时间>窗口起始时间且面头<结束时间就加入列表
        for (const auto& hold : temp_hold_list) {
            if (hold->timestamp < end &&
                hold->timestamp + hold->hold_time > start) {
                result_objects.insert(hold);
            }
        }
    }

    // 窗口内的物件
    auto startit = hitobjects.lower_bound(std::make_shared<Note>(start, 0));
    int count = 0;
    int deviation = 0;
    while (startit != hitobjects.end() && startit != hitobjects.begin() &&
           deviation < 5) {
        --startit;
        ++count;
        deviation = std::abs((*startit)->timestamp - start);
        if (deviation < 5) {
            result_objects.insert(*startit);
        }
    }

    // 返回
    while (count > 0) {
        ++startit;
        --count;
    }

    ComplexNote* temp_comp = nullptr;

    for (; startit != hitobjects.end() && (*startit)->timestamp < end;
         ++startit) {
        // 添加组合键
        auto note = std::dynamic_pointer_cast<Note>(*startit);
        if (note && note->parent_reference &&
            temp_comp != note->parent_reference) {
            temp_comp = note->parent_reference;
            // 将所有子物件加入渲染队列
            for (const auto& subnote : temp_comp->child_notes) {
                result_objects.insert(subnote);
            }
        }
        // 添加窗口内物件
        result_objects.insert(*startit);
    }
}
