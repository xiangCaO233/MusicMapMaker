#ifndef M_AUDIOENGIN_PLAYCALLBACK_H
#define M_AUDIOENGIN_PLAYCALLBACK_H

#include <qobject.h>
#include <qtmetamacros.h>

#include "../../../log/colorful-log.h"
#include "AudioManager.h"
#include "util/utils.h"

class AudioEnginPlayCallback : public QObject, public PlayposCallBack {
  Q_OBJECT
  double current_audio_time{0};
 signals:
  void music_play_callback(double time);

 public:
  std::atomic<bool> synclock{true};
  AudioEnginPlayCallback();
  ~AudioEnginPlayCallback();
  static int32_t count;

  void playpos_call(double playpos);

  template <typename func>
  void waitfor_clear_buffer(func&& f) {
    synclock.store(true, std::memory_order_relaxed);
    XINFO("等待缓冲区清空");
    // 等到回调执行
    while (synclock.load(std::memory_order_acquire)) {
      // 让出CPU时间片--等待播放线程清空缓冲区并修改同步锁为true
      std::this_thread::yield();
    }

    // 执行任意传入lambda
    f();
    ++count;
    if (count % 2000 == 0) {
      emit music_play_callback(current_audio_time);
      count = 0;
    }
  }
};

#endif  // M_AUDIOENGIN_PLAYCALLBACK_H
