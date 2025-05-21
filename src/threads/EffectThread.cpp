#include "EffectThread.h"

#include <qdir.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "../canvas/map/MapWorkspaceCanvas.h"
#include "../mmm/MapWorkProject.h"
#include "audio/BackgroundAudio.h"
#include "colorful-log.h"
#include "mainwindow.h"
#include "mmm/hitobject/HitObject.h"
#include "mmm/hitobject/Note/Note.h"
#include "mmm/hitobject/Note/rm/Slide.h"
#include "threads/ThreadPool.h"

// 构造EffectThread
EffectThread::EffectThread(std::shared_ptr<MapEditor> e)
    : editor(e), tpool(4) {}

// 析构EffectThread
EffectThread::~EffectThread() { stop(); }

// 启动线程
void EffectThread::start() {
    exit = false;
    thread = std::thread(&EffectThread::effect_thread, this);
    thread.detach();

    XINFO("启动特效线程");
}

// 停止线程
void EffectThread::stop() {
    exit = true;
    threadcv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }
}
// 断开当前回调
void EffectThread::disconnect_current_callback() {
    disconnect(editor->canvas_ref->working_map->audio_pos_callback.get(),
               &AudioEnginPlayCallback::music_play_callback, this,
               &EffectThread::on_music_play_callback);
}

// 更新map
void EffectThread::update_map() {
    connect(editor->canvas_ref->working_map->audio_pos_callback.get(),
            &AudioEnginPlayCallback::music_play_callback, this,
            &EffectThread::on_music_play_callback, Qt::QueuedConnection);
    // 重启线程
    stop();
    start();
}

// 同步音乐音频的时间
void EffectThread::sync_music_time(double time) {
    last_sync_audio_time_ms = time;
    last_sync_real_time = std::chrono::steady_clock::now();  // 记录当前现实时间
    if (is_playing) {
        // ----- 恢复播放时 (paused == false) -----
        // 重置 last_triggered_timestamp 到当前同步时间点之前一点点。
        // 这样 upper_bound 就能找到 >= last_sync_audio_time_ms 的第一个事件。
        // 所以减去 1 确保我们查找的 key 肯定小于或等于实际的同步时间戳。
        // 这假设时间戳的最小间隔大于 1ms，如果不是，需要更小的偏移或不同逻辑。
        last_triggered_timestamp = last_sync_audio_time_ms - 1;

        // 考虑边界情况：如果 last_sync_audio_time_ms 接近 0，减 1
        // 可能变为负数。 如果你的时间戳都是非负的，可以加个判断：
        if (last_sync_audio_time_ms < 1.0) {
            last_triggered_timestamp = -1;
        }

        // XWARN("EffectThread: Resumed. Sync Time: " +
        //       std::to_string(int(last_sync_audio_time_ms)) +
        //       " ms. Reset "
        //       "last_triggered_timestamp to:" +
        //       std::to_string(last_triggered_timestamp));

    } else {
        // ----- 暂停时 (paused == true) -----
        // 不需要修改 last_triggered_timestamp，它记录了暂停前的状态。
        // XWARN("EffectThread: Paused. Sync Time: " +
        //       std::to_string(int(last_sync_audio_time_ms)) +
        //       " ms. Reset "
        //       "last_triggered_timestamp to:" +
        //       std::to_string(last_triggered_timestamp));
    }
    XINFO("同步特效线程[" + std::string(is_playing ? "运行中" : "暂停") +
          "] 同步时间: " + std::to_string(int(last_sync_audio_time_ms)) +
          " ms. 重置"
          "last_triggered_timestamp 为:" +
          std::to_string(last_triggered_timestamp) + "ms");
    // 同步画布
    editor->cstatus.current_time_stamp = time;
    editor->cstatus.current_visual_time_stamp =
        editor->cstatus.current_time_stamp + editor->cstatus.static_time_offset;
}

// 音乐播放回调槽
void EffectThread::on_music_play_callback(double time) {
    XWARN("音乐播放位置回调-----上一回调结束位置[" + std::to_string(time) +
          "]ms");

    sync_music_time(time - (1.0 - editor->cstatus.playspeed) *
                               BackgroundAudio::audio_buffer_offset);
}

// 画布暂停事件--更新参考时间
void EffectThread::on_canvas_pause(bool paused) {
    std::lock_guard<std::mutex> lock(state_mutex);
    is_playing = !paused;
    if (!editor->canvas_ref->working_map) return;
    // 获取精确时间
    // XERROR("Pos Off:[" +
    //        std::to_string(
    //            BackgroundAudio::get_audio_pos(
    //                editor->canvas_ref->working_map->project_reference->devicename,
    //                QDir(editor->canvas_ref->working_map->audio_file_abs_path)
    //                    .canonicalPath()
    //                    .toStdString()) -
    //            editor->current_time_stamp) +
    //        "]");
    sync_music_time(BackgroundAudio::get_audio_pos(
        editor->canvas_ref->working_map->project_reference->devicename,
        QDir(editor->canvas_ref->working_map->audio_file_abs_path)
            .canonicalPath()
            .toStdString()));
}

// 实际运行函数
void EffectThread::effect_thread() {
    double current_estimated_audio_time_ms = 0.0;

    // 休眠逻辑常量
    const std::chrono::milliseconds max_sleep(10);
    const std::chrono::milliseconds min_sleep(1);
    // 提前量现在要考虑播放速度，或者用现实时间的提前量
    // 现实时间提前 5ms
    const std::chrono::microseconds lookahead_real_time(5000);
    while (!exit) {
        // ---读取状态---
        bool currently_playing;
        double sync_audio_time;
        std::chrono::steady_clock::time_point sync_real_time;
        // 当前速度
        if (!editor) continue;

        double current_playspeed = editor->cstatus.playspeed;

        {
            // 锁定状态变量读取区
            std::lock_guard<std::mutex> lock(state_mutex);
            currently_playing = is_playing;
            sync_audio_time = last_sync_audio_time_ms;
            sync_real_time = last_sync_real_time;
        }
        // 解锁
        // --- 计算当前估计的音频时间 ---
        if (currently_playing) {
            auto now_real_time = std::chrono::steady_clock::now();
            // 计算自上次同步以来经过的现实时间 (秒)
            std::chrono::duration<double> elapsed_real_seconds =
                now_real_time - sync_real_time;
            // 推算经过的音频时间 (毫秒) = 现实时间(秒) * 速度 * 1000
            double elapsed_audio_ms =
                elapsed_real_seconds.count() * current_playspeed * 1000.0;
            // 当前估计的音频时间 = 上次同步时间 + 推算的经过时间
            current_estimated_audio_time_ms =
                sync_audio_time + elapsed_audio_ms;

        } else {
            // 如果暂停了，音频时间就停留在上次同步的时间点
            current_estimated_audio_time_ms = sync_audio_time;
        }

        std::shared_ptr<HitObject> next_event = nullptr;
        if (editor->canvas_ref->working_map) {
            auto& hitobjects = editor->canvas_ref->working_map->hitobjects;
            // --- 处理 HitObject (只有在播放时才处理) ---
            if (currently_playing) {
                {
                    // --- 临界区：访问 hitobjects (使用外部传入的锁) ---
                    std::lock_guard<std::mutex> lock(
                        editor->canvas_ref->working_map
                            ->hitobjects_mutex);  // 使用传入的 hitobjects 锁

                    // 查找逻辑不变，但使用 current_estimated_audio_time_ms
                    auto search_key =
                        std::make_shared<Note>(last_triggered_timestamp, 0);
                    search_key->object_type = std::numeric_limits<
                        decltype(HitObject::object_type)>::min();

                    auto iter = hitobjects.upper_bound(search_key);
                    // if (iter != hitobjects.end()) {
                    //   //
                    //   打印找到的第一个事件的时间戳，以及当前的估计时间和基准时间戳
                    //   XWARN("EffectThread Loop: upper_bound found event TS="
                    //   +
                    //         std::to_string(((*iter)->timestamp)) +
                    //         ". "
                    //         "EstimatedTime=" +
                    //         std::to_string(current_estimated_audio_time_ms) +
                    //         ". LastTriggeredTS=" +
                    //         std::to_string(last_triggered_timestamp));
                    // } else {
                    //   // 如果 upper_bound 没找到任何后续事件
                    //   XWARN(
                    //       "EffectThread Loop: upper_bound found NO event. "
                    //       "EstimatedTime=" +
                    //       std::to_string(current_estimated_audio_time_ms) +
                    //       ". LastTriggeredTS=" +
                    //       std::to_string(last_triggered_timestamp));
                    // }
                    const double epsilon = 5;

                    while (iter != hitobjects.end()) {
                        const auto& current_object = *iter;

                        // 使用推算出的时间进行比较
                        if (current_object->timestamp <=
                            current_estimated_audio_time_ms + epsilon) {
                            // --- 触发效果 ---
                            // XWARN("触发事件");
                            // XWARN("触发事件: EventTS=" +
                            //       std::to_string(current_object->timestamp) +
                            //       ", EstimatedAudioTime=" +
                            //       std::to_string(current_estimated_audio_time_ms)
                            //       +
                            //       ",obj:\n" + iter->get()->toString());
                            if (current_object->is_note) {
                                auto note = std::static_pointer_cast<Note>(
                                    current_object);
                                // 物件的横向偏移
                                auto x =
                                    editor->ebuffer.edit_area_start_pos_x +
                                    editor->ebuffer.orbit_width * note->orbit +
                                    editor->ebuffer.orbit_width / 2.0;
                                // 轨道宽度
                                auto ow = editor->ebuffer.orbit_width;
                                // 画布引用
                                auto canvas = editor->canvas_ref;
                                if (!canvas->played_effects_objects.contains(
                                        note)) {
                                    // 播放物件添加到的位置
                                    auto obj_it = canvas->played_effects_objects
                                                      .emplace(note)
                                                      .first;

                                    // TODO(xiang 2025-05-14): 性能优化--严重
                                    // 使用线程池添加任务
                                    tpool.enqueue_void([=]() {
                                        auto play_x = x;
                                        // 特效类型
                                        EffectType t;
                                        // 音效类型
                                        SoundEffectType soundt;
                                        // 特效帧数
                                        double playtime;
                                        if (note) {
                                            switch (note->note_type) {
                                                case NoteType::SLIDE: {
                                                    if (note->compinfo !=
                                                            ComplexInfo::NONE &&
                                                        note->compinfo !=
                                                            ComplexInfo::END)
                                                        return;

                                                    t = EffectType::SLIDEARROW;
                                                    play_x +=
                                                        (std::
                                                             static_pointer_cast<
                                                                 Slide>(note))
                                                            ->slide_parameter *
                                                        ow;
                                                    playtime =
                                                        canvas->skin
                                                            .normal_hit_effect_duration /
                                                        canvas->editor->cstatus
                                                            .playspeed;
                                                    if (note->compinfo ==
                                                        ComplexInfo::NONE) {
                                                        soundt =
                                                            SoundEffectType::
                                                                SLIDE;
                                                    } else {
                                                        soundt =
                                                            SoundEffectType::
                                                                COMMON_HIT;
                                                    }
                                                    break;
                                                }
                                                default: {
                                                    t = EffectType::NORMAL;
                                                    playtime =
                                                        canvas->skin
                                                            .normal_hit_effect_duration /
                                                        canvas->editor->cstatus
                                                            .playspeed;
                                                    soundt = SoundEffectType::
                                                        COMMON_HIT;
                                                    break;
                                                }
                                            }
                                            if (note->note_type ==
                                                NoteType::HOLD) {
                                                auto hold =
                                                    std::static_pointer_cast<
                                                        Hold>(note);
                                                playtime =
                                                    hold->hold_time /
                                                    canvas->editor->cstatus
                                                        .playspeed;
                                            }

                                            // 播放特效
                                            canvas->play_effect(
                                                play_x,
                                                canvas->editor->ebuffer
                                                    .judgeline_visual_position,
                                                playtime, t);

                                            // 延迟对音
                                            std::this_thread::sleep_for(
                                                std::chrono::milliseconds(int(
                                                    canvas->des_update_time *
                                                    1.75 /
                                                    canvas->editor->cstatus
                                                        .playspeed)));

                                            // 播放音效
                                            BackgroundAudio::
                                                play_audio_with_new_orbit(
                                                    canvas->working_map
                                                        ->project_reference
                                                        ->devicename,
                                                    canvas->skin
                                                        .get_sound_effect(
                                                            soundt),
                                                    0);
                                        }
                                    });
                                }
                            }

                            // 更新最后触发的时间戳
                            last_triggered_timestamp =
                                current_object->timestamp;
                            ++iter;
                        } else {
                            // 找到下一个未到期的事件
                            next_event = current_object;
                            break;
                        }
                    }
                }  // --- 临界区结束 ---
            } else {
                // 暂停状态下，可以查找下一个事件，但不触发
                std::lock_guard<std::mutex> lock(
                    editor->canvas_ref->working_map->hitobjects_mutex);
                auto search_key =
                    std::make_shared<Note>(last_triggered_timestamp, 0);
                search_key->object_type = std::numeric_limits<
                    decltype(HitObject::object_type)>::min();
                auto iter = hitobjects.upper_bound(search_key);
                if (iter != hitobjects.end()) {
                    next_event = *iter;
                }
            }
        }

        // --- 休眠逻辑 ---
        std::chrono::milliseconds sleep_duration = max_sleep;

        if (currently_playing && next_event) {
            // 仅在播放且有下一事件
            // 距离下一个事件的 *音频时间* (毫秒)
            double audio_time_to_next_ms =
                next_event->timestamp - current_estimated_audio_time_ms;

            if (audio_time_to_next_ms > 0) {
                // 将音频时间差转换为 *现实时间* 差 (毫秒)
                double real_time_to_next_ms =
                    audio_time_to_next_ms / current_playspeed;

                // 计算现实时间的休眠时长 (微秒)
                long long sleep_us =
                    static_cast<long long>(real_time_to_next_ms * 1000.0);

                // 减去提前量 (现实时间)
                sleep_us -= lookahead_real_time.count();

                if (sleep_us <= min_sleep.count() * 1000) {
                    sleep_duration = min_sleep;
                } else {
                    // 转换为毫秒进行比较和设置
                    sleep_duration =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::microseconds(sleep_us));

                    if (sleep_duration > max_sleep) {
                        sleep_duration = max_sleep;
                    }
                    if (sleep_duration < min_sleep) {
                        sleep_duration = min_sleep;
                    }
                }
            } else {
                // 事件已错过或非常近，最小休眠
                sleep_duration = min_sleep;
            }
        } else if (!currently_playing) {
            // 暂停时可以睡久一点，或者根据需要调整（例如，如果暂停时仍需快速响应开始信号）
            sleep_duration = max_sleep;
        }
        // else (正在播放但没有下一个事件，或速度为0) -> 使用默认的 max_sleep
        // 执行休眠
        std::unique_lock<std::mutex> lock(threadmtx);
        // 等待 sleep_duration 或 exit 变为 true
        threadcv.wait_for(lock, sleep_duration, [&] { return exit.load(); });
    }
}
