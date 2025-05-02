#ifndef M_EFFECTTHREAD_H
#define M_EFFECTTHREAD_H

#include <qobject.h>
#include <qtmetamacros.h>

#include <memory>
#include <mutex>
#include <thread>

#include "editor/MapEditor.h"
class EffectThread : public QObject {
  Q_OBJECT

  // 退出线程
  std::atomic<bool> exit{false};

  std::thread thread;

  // 是否正在播放
  std::atomic<bool> is_playing{false};

  // 最后一次触发的时间
  double last_triggered_timestamp{0};

  // 上一次同步的现实时间
  std::chrono::steady_clock::time_point last_sync_real_time;

  // 上一次同步的音频时间
  double last_sync_audio_time_ms = 0.0;

  // 状态变量锁
  std::mutex state_mutex;

  // 编辑器
  std::shared_ptr<MapEditor> editor;

 public slots:
  // 画布暂停事件
  void on_canvas_pause(bool paused);

  // 音乐播放回调槽
  void on_music_play_callback(double time);

 public:
  // 构造EffectThread
  EffectThread(std::shared_ptr<MapEditor> e);

  // 析构EffectThread
  virtual ~EffectThread();

  // 同步音乐音频的时间
  void sync_music_time(double time);

  // 实际运行函数
  void effect_thread();

  // 更新map
  void update_map();
};

#endif  // M_EFFECTTHREAD_H
