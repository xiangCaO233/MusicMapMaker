#include "AudioEnginPlayCallback.h"

#include <string>

#include "colorful-log.h"

AudioEnginPlayCallback::AudioEnginPlayCallback() {}

AudioEnginPlayCallback::~AudioEnginPlayCallback() = default;

void AudioEnginPlayCallback::playpos_call(double playpos) {
  // XINFO("mixer callback pos:" + std::to_string(playpos));
  // TODO(xiang 2025-05-02): 同步效果线程时间
  // XINFO("本轨道缓冲区已清空");
  synclock.store(false, std::memory_order_release);
  static int32_t count = 0;
  ++count;
  XWARN("count:[" + std::to_string(count) + "]");
  if (count % 1000 == 0) {
    emit music_play_callback(xutil::plannerpcmpos2milliseconds(
        playpos, static_cast<int>(x::Config::samplerate)));
    count = 0;
  }
}
