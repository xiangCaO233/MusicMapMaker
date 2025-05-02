#include "AudioEnginPlayCallback.h"

AudioEnginPlayCallback::AudioEnginPlayCallback() {}

AudioEnginPlayCallback::~AudioEnginPlayCallback() = default;

void AudioEnginPlayCallback::playpos_call(double playpos) {
  // XINFO("mixer callback pos:" + std::to_string(playpos));
  // TODO(xiang 2025-05-02): 同步效果线程时间
  // XINFO("本轨道缓冲区已清空");
  // XWARN("count:[" + std::to_string(count) + "]");
  current_audio_time = xutil::plannerpcmpos2milliseconds(
      playpos, static_cast<int>(x::Config::samplerate));
  synclock.store(false, std::memory_order_release);
}
