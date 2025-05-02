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
#include "mmm/hitobject/HitObject.h"
#include "mmm/hitobject/Note/Note.h"

// 构造EffectThread
EffectThread::EffectThread(std::shared_ptr<MapEditor> e) : editor(e) {
  thread = std::thread(&EffectThread::effect_thread, this);
  thread.detach();
}

// 析构EffectThread
EffectThread::~EffectThread() { exit = true; }

// 画布暂停事件
void EffectThread::on_canvas_pause(bool paused) {
  std::lock_guard<std::mutex> lock(state_mutex);
  is_playing = !paused;
  // 获取精确时间
  last_sync_audio_time_ms = BackgroundAudio::get_audio_pos(
      editor->canvas_ref->working_map->project_reference->devicename,
      QDir(editor->canvas_ref->working_map->audio_file_abs_path)
          .canonicalPath()
          .toStdString());
  last_sync_real_time = std::chrono::steady_clock::now();  // 记录当前现实时间
  if (is_playing) {
    // ----- 恢复播放时 (paused == false) -----
    // 重置 last_triggered_timestamp 到当前同步时间点之前一点点。
    // 这样 upper_bound 就能找到 >= last_sync_audio_time_ms 的第一个事件。
    // 将 double 时间转换为 long long 可能会截断小数部分，
    // 所以减去 1 确保我们查找的 key 肯定小于或等于实际的同步时间戳。
    // 这假设时间戳的最小间隔大于 1ms，如果不是，需要更小的偏移或不同逻辑。
    last_triggered_timestamp = last_sync_audio_time_ms - 1;

    // 考虑边界情况：如果 last_sync_audio_time_ms 接近 0，减 1 可能变为负数。
    // 如果你的时间戳都是非负的，可以加个判断：
    if (last_sync_audio_time_ms < 1.0) {
      last_triggered_timestamp = -1;
    }

    // 打印日志，方便调试
    XWARN("EffectThread: Resumed. Sync Time: " +
          std::to_string(int(last_sync_audio_time_ms)) +
          " ms. Reset "
          "last_triggered_timestamp to:" +
          std::to_string(last_triggered_timestamp));

  } else {
    // ----- 暂停时 (paused == true) -----
    // 不需要修改 last_triggered_timestamp，它记录了暂停前的状态。
    XWARN("EffectThread: Paused. Sync Time: " +
          std::to_string(int(last_sync_audio_time_ms)) +
          " ms. Reset "
          "last_triggered_timestamp to:" +
          std::to_string(last_triggered_timestamp));
  }
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

    double current_playspeed = editor->playspeed;

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
      current_estimated_audio_time_ms = sync_audio_time + elapsed_audio_ms;

    } else {
      // 如果暂停了，音频时间就停留在上次同步的时间点
      current_estimated_audio_time_ms = sync_audio_time;
    }

    std::shared_ptr<HitObject> next_event = nullptr;
    if (editor->canvas_ref->working_map) {
      auto& hitobjects = editor->canvas_ref->working_map->hitobjects;
      // --- 处理 HitObject (只有在播放时才处理) ---
      if (currently_playing) {
        // --- 临界区：访问 hitobjects (使用外部传入的锁) ---
        {
          std::lock_guard<std::mutex> lock(
              editor->canvas_ref->working_map
                  ->hitobjects_mutex);  // 使用传入的 hitobjects 锁

          // 查找逻辑不变，但使用 current_estimated_audio_time_ms
          auto search_key = std::make_shared<Note>(last_triggered_timestamp, 0);
          search_key->object_type =
              std::numeric_limits<decltype(HitObject::object_type)>::min();

          auto iter = hitobjects.upper_bound(search_key);
          // if (iter != hitobjects.end()) {
          //   // 打印找到的第一个事件的时间戳，以及当前的估计时间和基准时间戳
          //   XWARN("EffectThread Loop: upper_bound found event TS=" +
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
          const double epsilon = 0.001;

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
              //       std::to_string(current_estimated_audio_time_ms) +
              //       ",obj:\n" + iter->get()->toString());
              if (current_object->is_note) {
                auto note = std::static_pointer_cast<Note>(current_object);
                // TODO: 实际的音频播放调用
                switch (note->note_type) {
                  case NoteType::NOTE:
                  case NoteType::HOLD: {
                    BackgroundAudio::play_audio_with_new_orbit(
                        editor->canvas_ref->working_map->project_reference
                            ->devicename,
                        editor->canvas_ref->skin.get_sound_effect(
                            SoundEffectType::COMMON_HIT),
                        0);
                    break;
                  }
                  case NoteType::SLIDE: {
                    BackgroundAudio::play_audio_with_new_orbit(
                        editor->canvas_ref->working_map->project_reference
                            ->devicename,
                        editor->canvas_ref->skin.get_sound_effect(
                            SoundEffectType::SLIDE),
                        0);
                    break;
                  }
                }
              }

              last_triggered_timestamp =
                  current_object->timestamp;  // 更新最后触发的时间戳
              ++iter;
            } else {
              next_event = current_object;  // 找到下一个未到期的事件
              break;
            }
          }
        }  // --- 临界区结束 ---
      } else {
        // 暂停状态下，可以查找下一个事件，但不触发
        std::lock_guard<std::mutex> lock(
            editor->canvas_ref->working_map->hitobjects_mutex);
        auto search_key = std::make_shared<Note>(last_triggered_timestamp, 0);
        search_key->object_type =
            std::numeric_limits<decltype(HitObject::object_type)>::min();
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
        double real_time_to_next_ms = audio_time_to_next_ms / current_playspeed;

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
    if (!exit) {
      std::this_thread::sleep_for(sleep_duration);
    }
  }
}
