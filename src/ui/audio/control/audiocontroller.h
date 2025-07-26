#ifndef AUDIOCONTROLLER_H
#define AUDIOCONTROLLER_H

#include <qobject.h>
#include <qstandarditemmodel.h>
#include <qtmetamacros.h>

#include <HideableToolWindow.hpp>
#include <QWidget>
#include <ice/core/SourceNode.hpp>
#include <ice/manage/AudioTrack.hpp>
#include <memory>

#include "audio/control/ProcessChain.hpp"

namespace Ui {
class AudioController;
}
class AudioTrack;
class PlayPosCallBack;

enum class TimeFormat {

};

class AudioController : public HideableToolWindow {
    Q_OBJECT

   public:
    // 定义单位枚举
    enum class PositionUnit {
        Milliseconds,
        Microseconds,
        Nanoseconds,
        Frame,
        MinSec,
        MinSecMs,
        MinSecMsUsNs
    };
    // 注册到 Qt 的元对象系统
    Q_ENUM(PositionUnit)

    explicit AudioController(QWidget *parent = nullptr);
    ~AudioController() override;

    // 传递引用
    void set_audio_track(const std::shared_ptr<ice::AudioTrack> &track);
    void set_item(QStandardItem *item);

    inline const std::shared_ptr<ice::AudioTrack> &track() const {
        return audio_track;
    }

    inline QStandardItem *item() const { return refitem; }

    inline const std::shared_ptr<ice::SourceNode> &node() const {
        return source_node;
    }

    inline const std::shared_ptr<ice::IAudioNode> &output() const {
        return output_node;
    }

    inline void set_uiframe_pos(size_t frame_pos) { uiframe_pos = frame_pos; }
    inline void set_uitime_pos(std::chrono::nanoseconds time_pos) {
        uitime_pos = time_pos;
    }

   signals:
    void update_output_node(const AudioController *controller,
                            std::shared_ptr<ice::IAudioNode> oldnode,
                            std::shared_ptr<ice::IAudioNode> newnode);

   private slots:
    void on_pause_button_toggled(bool checked);

    void on_fast_backward_button_clicked();

    void on_fast_forward_button_clicked();

    void on_time_edit_editingFinished();

    void on_unit_selection_currentIndexChanged(int index);

    void on_volume_slider_valueChanged(int value);

    // 播放完成
    void playDone();

    // 更新显示位置
    void updateDisplayPosition();

    void on_graphtype_selection_currentIndexChanged(int index);

    void on_area_scale_spinner_valueChanged(int arg1);

    void on_lock_ptr_pos_button_toggled(bool checked);

    void on_ptr_pos_value_spinner_valueChanged(double arg1);

    void on_stretcher_speed_slider_valueChanged(int value);

    void on_speed_value_spinner_valueChanged(double arg1);

   private:
    // 音频轨道
    std::shared_ptr<ice::AudioTrack> audio_track{nullptr};

    // 实际播放的源节点
    std::shared_ptr<ice::SourceNode> source_node{nullptr};

    // 音效处理链
    std::unique_ptr<ProcessChain> process_chain{nullptr};

    // 控制器的输出节点
    std::shared_ptr<ice::IAudioNode> output_node{nullptr};

    // 播放回调
    std::shared_ptr<ice::PlayCallBack> callback{nullptr};

    // 对应的在列表中的项
    QStandardItem *refitem{nullptr};

    // ui需要显示帧位置时需要展示的参考值
    size_t uiframe_pos;

    // ui需要显示时间位置时需要展示的参考值
    std::chrono::nanoseconds uitime_pos;

    Ui::AudioController *ui;
};

#endif  // AUDIOCONTROLLER_H
