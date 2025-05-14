#ifndef M_BACKGROUNDAUDIO_H
#define M_BACKGROUNDAUDIO_H

#include <util/utils.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include "../log/colorful-log.h"
#include "AudioManager.h"

enum class AudioType {
    MUSIC,
    EFFECT,
};

class BackgroundAudio {
    // 音频管理器
    static std::shared_ptr<XAudioManager> audiomanager;

   public:
    // 音频缓冲区偏移
    static double audio_buffer_offset;

    // 变调是否启用
    static bool enable_pitch_alt;

    // 轨道音量
    static float music_orbits_volume;
    static float effect_orbits_volume;

    // 轨道速度
    static float orbit_speed;

    // 全局速度
    static float global_speed;

    // 全局音量
    static float global_volume;

    // 初始化音频管理器
    inline static void init() {
        BackgroundAudio::audiomanager = XAudioManager::newmanager();
        // XLogger::setlevel(spdlog::level::warn);
        // audiomanager->disableLoggin();
    }

    inline static std::unordered_map<int, std::shared_ptr<XOutputDevice>>*
    devices() {
        return BackgroundAudio::audiomanager->get_outdevices();
    }

    // 添加播放回调
    inline static void add_playpos_callback(
        const std::string& device_full_name, const std::string& audio_full_path,
        std::shared_ptr<PlayposCallBack> callback) {
        if (device_full_name == "unknown output device") return;
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
        if (audio_idit == audiomanager->get_handles()->end()) return;
        auto& device = audiomanager->get_outdevices()->at(device_idit->second);
        auto orbits_it = device->player->mixer->audio_orbits.find(
            audiomanager->get_audios()->at(audio_idit->second));
        if (orbits_it == device->player->mixer->audio_orbits.end()) return;
        for (auto& orbit : orbits_it->second) {
            orbit->add_playpos_callback(callback);
        }
    }

    // 初始化设备
    inline static void init_device(const std::string& device_full_name) {
        if (device_full_name == "unknown output device") return;
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit != audiomanager->get_outdevice_indicies()->end()) {
            auto& device =
                audiomanager->get_outdevices()->at(device_idit->second);
            if (!device->player) {
                device->creat_player();
            }
        }
    }

    // 载入音频
    inline static int loadin_audio(const std::string& audio_full_path) {
        std::string temp;
        return audiomanager->loadaudio(audio_full_path, temp);
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
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
        if (audio_idit == audiomanager->get_handles()->end()) return;
        audiomanager->playAudio(device_idit->second, audio_idit->second, false);
        audiomanager->setAudioVolume(device_idit->second, audio_idit->second,
                                     music_orbits_volume);
    }

    // 从指定位置播放特定设备音频
    inline static void play_audio(const std::string& device_full_name,
                                  const std::string& audio_full_path,
                                  double pos) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
        if (audio_idit == audiomanager->get_handles()->end()) return;
        auto& device = audiomanager->get_outdevices()->at(device_idit->second);
        auto orbits_it = device->player->mixer->audio_orbits.find(
            audiomanager->get_audios()->at(audio_idit->second));
        if (orbits_it == device->player->mixer->audio_orbits.end()) {
            // 无此音轨-自动添加
            audiomanager->playAudio(device_idit->second, audio_idit->second,
                                    false);
        } else {
            for (auto& orbit : orbits_it->second) {
                // 有此音源-设置所有轨道位置后从播放
                orbit->playpos =
                    xutil::milliseconds2plannerpcmpos(
                        pos, static_cast<int>(x::Config::samplerate)) *
                    static_cast<int>(x::Config::channel);
                // 使用当前设置的音频音量
                orbit->volume = music_orbits_volume;
                orbit->paused = false;
            }
        }
    }

    // 从指定位置播放特定设备音频(新建轨道-实时音轨)
    inline static void play_audio_with_new_orbit(
        const std::string& device_full_name, const std::string& audio_full_path,
        double pos) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
        if (audio_idit == audiomanager->get_handles()->end()) return;
        auto audio_it = audiomanager->get_audios()->find(audio_idit->second);
        if (audio_it == audiomanager->get_audios()->end()) return;
        auto& device = audiomanager->get_outdevices()->at(device_idit->second);
        auto orbit = std::make_shared<XAudioOrbit>(audio_it->second);
        orbit->playpos = xutil::milliseconds2plannerpcmpos(
                             pos, static_cast<int>(x::Config::samplerate)) *
                         static_cast<int>(x::Config::channel);
        orbit->volume = effect_orbits_volume;
        // 添加此音轨到播放器
        device->player->mixer->add_orbit_immediatly(orbit);
    }

    // 暂停特定设备音频
    inline static void pause_audio(const std::string& device_full_name,
                                   const std::string& audio_full_path) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
        if (audio_idit == audiomanager->get_handles()->end()) return;

        audiomanager->pauseAudio(device_idit->second, audio_idit->second);
    }

    // 获取特定设备音频播放位置-暂停时精确
    inline static double get_audio_pos(const std::string& device_full_name,
                                       const std::string& audio_full_path) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return -1;
        auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
        if (audio_idit == audiomanager->get_handles()->end()) return -1;
        auto& device = audiomanager->get_outdevices()->at(device_idit->second);
        if (!device->player) return 0;

        auto orbitsit = device->player->mixer->audio_orbits.find(
            audiomanager->get_audios()->at(audio_idit->second));
        if (orbitsit == device->player->mixer->audio_orbits.end()) return 0;

        for (const auto& orbit : orbitsit->second) {
            return xutil::plannerpcmpos2milliseconds(
                orbit->playpos, static_cast<int>(x::Config::samplerate));
        }
        return 0;
    }

    // 设定特定设备音频播放位置
    inline static void set_audio_pos(const std::string& device_full_name,
                                     const std::string& audio_full_path,
                                     double time_milliseconds) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
        if (audio_idit == audiomanager->get_handles()->end()) return;

        audiomanager->set_audio_current_pos(
            device_idit->second, audio_idit->second, time_milliseconds);
    }

    // 获取音频时长
    inline static double get_audio_length(const std::string& audio_full_path) {
        auto audio_idit = audiomanager->get_handles()->find(audio_full_path);
        if (audio_idit == audiomanager->get_handles()->end()) return -1;
        auto& audio = audiomanager->get_audios()->at(audio_idit->second);
        return double(xutil::plannerpcmpos2milliseconds(
                          audio->get_pcm_data_size(),
                          static_cast<int>(x::Config::samplerate)) /
                      static_cast<int>(x::Config::channel));
    }

    // 获取设备播放器播放状态
    inline static int32_t get_device_playerstatus(
        const std::string& device_full_name) {
        if (device_full_name == "unknown output device") return -1;
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return -1;
        return audiomanager->get_outdevices()
                       ->at(device_idit->second)
                       ->player->paused
                   ? 0
                   : 1;
    }

    // 设置设备播放器上全部音频的播放速度
    inline static void set_play_speed(const std::string& device_full_name,
                                      double speed_ratio,
                                      bool enable_pitch_alt = false) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto& player =
            audiomanager->get_outdevices()->at(device_idit->second)->player;
        if (!player) return;
        if (enable_pitch_alt) {
            if (orbit_speed != 1.0) {
                for (const auto& [id, orbits] : player->mixer->audio_orbits) {
                    for (auto& orbit : orbits) {
                        orbit->speed = 1.0;
                    }
                }
            }
            if (global_speed != speed_ratio) {
                player->ratio(speed_ratio);
            }
            global_speed = speed_ratio;
        } else {
            if (global_speed != 1.0) {
                if (!player->paused) {
                    player->pause();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                player->ratio(1.0);
                global_speed = 1.0;
            }
            for (const auto& [id, orbits] : player->mixer->audio_orbits) {
                for (auto& orbit : orbits) {
                    orbit->speed = speed_ratio;
                }
            }
            orbit_speed = speed_ratio;
        }
    }

    // 设置全局音量
    inline static void set_global_volume(const std::string& device_full_name,
                                         float volume) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto& player =
            audiomanager->get_outdevices()->at(device_idit->second)->player;
        if (!player) return;
        global_volume = volume;
        player->global_volume = volume;
    }

    // 设置全局音乐音量
    inline static void set_music_volume(const std::string& device_full_name,
                                        float volume) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto& player =
            audiomanager->get_outdevices()->at(device_idit->second)->player;
        if (!player) return;
        music_orbits_volume = volume;
        // 设置所有非实时轨道音量
        for (auto& [sound, orbits] : player->mixer->audio_orbits) {
            for (auto& orbit : orbits) {
                orbit->volume = music_orbits_volume;
            }
        }
    }

    // 设置全局特效音量
    inline static void set_effects_volume(const std::string& device_full_name,
                                          float volume) {
        auto device_idit =
            audiomanager->get_outdevice_indicies()->find(device_full_name);
        if (device_idit == audiomanager->get_outdevice_indicies()->end())
            return;
        auto& player =
            audiomanager->get_outdevices()->at(device_idit->second)->player;
        if (!player) return;
        effect_orbits_volume = volume;
        // 设置所有实时轨道音量
        for (auto& [sound, orbits] : player->mixer->immediate_orbits) {
            for (auto& orbit : orbits) {
                orbit->volume = effect_orbits_volume;
            }
        }
    }

    inline static std::string get_audio_artist(const std::string& audio_name) {
        return audiomanager->getAudioArtistUnicode(audio_name);
    }
    inline static std::string get_audio_title(const std::string& audio_name) {
        return audiomanager->getAudioTitleUnicode(audio_name);
    }
};  // namespace BackgroundAudio

#endif  // M_BACKGROUNDAUDIO_H
