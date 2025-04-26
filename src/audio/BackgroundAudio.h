#ifndef M_BACKGROUNDAUDIO_H
#define M_BACKGROUNDAUDIO_H

#include <memory>
#include <string>
#include <unordered_map>

#include "../log/colorful-log.h"
#include "AudioManager.h"

class BackgroundAudio {
  // 音频管理器
  static std::shared_ptr<XAudioManager> audiomanager;

  // 轨道音量
  static float orbit_volume;

  // 轨道速度
  static float orbit_speed;

  // 全局音量
  static float global_volume;

  // 全局速度
  static float global_speed;

 public:
  inline static void init() {
    BackgroundAudio::audiomanager = XAudioManager::newmanager();
    // XLogger::setlevel(spdlog::level::warn);
    // audiomanager->disableLoggin();
  }

  inline static std::unordered_map<int, std::shared_ptr<XOutputDevice>>*
  devices() {
    return BackgroundAudio::audiomanager->get_outdevices();
  }

  inline static void add_playpos_callback(
      const std::string& device_full_name, const std::string& audio_full_path,
      std::shared_ptr<PlayposCallBack> callback) {
    if (device_full_name == "unknown output device") return;
    auto device_idit =
        audiomanager->get_outdevice_indicies()->find(device_full_name);
    if (device_idit == audiomanager->get_outdevice_indicies()->end()) return;
    auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
    if (audio_idit == audiomanager->get_handles()->end()) return;
    auto& device = audiomanager->get_outdevices()->at(device_idit->second);
    auto orbit_it =
        device->player->mixer->audio_orbits.find(audio_idit->second);
    if (orbit_it == device->player->mixer->audio_orbits.end()) return;
    orbit_it->second->add_playpos_callback(callback);
  }

  // 初始化设备
  inline static void init_device(const std::string& device_full_name) {
    if (device_full_name == "unknown output device") return;
    auto device_idit =
        audiomanager->get_outdevice_indicies()->find(device_full_name);
    if (device_idit != audiomanager->get_outdevice_indicies()->end()) {
      auto& device = audiomanager->get_outdevices()->at(device_idit->second);
      if (!device->player) {
        device->creat_player();
      }
    }
  }

  // 载入音频
  inline static void loadin_audio(const std::string& audio_full_path) {
    std::string temp;
    audiomanager->loadaudio(audio_full_path, temp);
  }

  // 卸载音频
  inline static void unload_audio(const std::string& audio_full_path) {
    auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
    if (audio_idit != audiomanager->get_handles()->end()) {
      audiomanager->unloadaudio(audio_idit->second);
    }
  }

  // 播放特定设备音频
  inline static void play_audio(const std::string& device_full_name,
                                const std::string& audio_full_path) {
    auto device_idit =
        audiomanager->get_outdevice_indicies()->find(device_full_name);
    if (device_idit == audiomanager->get_outdevice_indicies()->end()) return;
    auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
    if (audio_idit == audiomanager->get_handles()->end()) return;

    audiomanager->playAudio(device_idit->second, audio_idit->second, false);
  }

  // 暂停特定设备音频
  inline static void pause_audio(const std::string& device_full_name,
                                 const std::string& audio_full_path) {
    auto device_idit =
        audiomanager->get_outdevice_indicies()->find(device_full_name);
    if (device_idit == audiomanager->get_outdevice_indicies()->end()) return;
    auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
    if (audio_idit == audiomanager->get_handles()->end()) return;

    audiomanager->pauseAudio(device_idit->second, audio_idit->second);
  }

  // 设定特定设备音频播放位置
  inline static void set_audio_pos(const std::string& device_full_name,
                                   const std::string& audio_full_path,
                                   double time_milliseconds) {
    auto device_idit =
        audiomanager->get_outdevice_indicies()->find(device_full_name);
    if (device_idit == audiomanager->get_outdevice_indicies()->end()) return;
    auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
    if (audio_idit == audiomanager->get_handles()->end()) return;

    audiomanager->set_audio_current_pos(device_idit->second, audio_idit->second,
                                        time_milliseconds);
  }

  // 获取音频时长
  inline static double get_audio_length(const std::string& audio_full_path) {
    auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
    if (audio_idit == audiomanager->get_handles()->end()) return -1;
    auto& audio = audiomanager->get_audios()->at(audio_idit->second);
    return double(
        xutil::plannerpcmpos2milliseconds(
            audio->get_pcm_data_size(), static_cast<int>(Config::samplerate)) /
        static_cast<int>(Config::channel));
  }

  // 获取设备播放器播放状态
  inline static int32_t get_device_playerstatus(
      const std::string& device_full_name) {
    if (device_full_name == "unknown output device") return -1;
    auto device_idit =
        audiomanager->get_outdevice_indicies()->find(device_full_name);
    if (device_idit == audiomanager->get_outdevice_indicies()->end()) return -1;
    return audiomanager->get_outdevices()
                   ->at(device_idit->second)
                   ->player->paused
               ? 0
               : 1;
  }
};  // namespace BackgroundAudio

#endif  // M_BACKGROUNDAUDIO_H
