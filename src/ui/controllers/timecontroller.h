#ifndef TIMECONTROLLER_H
#define TIMECONTROLLER_H

#include <qevent.h>
#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

#include "../GlobalSettings.h"

class MMap;
enum class GlobalTheme;

namespace Ui {
class audio_time_controller;
}

class TimeController : public QWidget {
    Q_OBJECT

   public:
    explicit TimeController(QWidget *parent = nullptr);
    ~TimeController();

    // 当前主题
    GlobalTheme current_theme;

    // 暂停状态
    bool pause{true};

    // 变调是否启用
    bool enable_pitch_alt{false};

    // 时间控制器绑定的map
    std::shared_ptr<MMap> binding_map;

    // 时间编辑器最后一次的时间值
    QString latest_time_edit_value{"0"};

    // 时间格式
    TimeFormat tformat{TimeFormat::MILLISECONDS};

    // 使用主题
    void use_theme(GlobalTheme theme);

    // 更新album
    void update_album();

   public slots:
    // page选择了新map事件
    void on_selectnewmap(std::shared_ptr<MMap> &map);

    // 画布时间变化事件
    void oncanvas_timestampChanged(double time);

    // 画布暂停槽
    void on_canvasPause(bool paused);

    // 实时信息变化槽
    void on_currentBpmChanged(double bpm);

    void on_currentTimelineSpeedChanged(double timeline_speed);

    // 画布调节时间线缩放
    void on_canvasAdjustTimelineZoom(int value);

    // 更新音频状态
    void update_audio_status();

   signals:
    // 时间控制器编辑时间信号
    void time_edited(double time);

    // 时间控制器播放速度变化信号
    void playspeed_changed(double speed);

    // 音乐位置同步信号
    void music_pos_synchronized(double time);

   private slots:

    // 变速设置
    void on_audiospeed_spinbox_valueChanged(double arg1);

    // 变速设置完成
    void on_audiospeed_spinbox_editingFinished();

    // 重置变速按钮事件
    void on_resetspeedbutton_clicked();

    // 启用变速变调按钮事件
    void on_enablepitchaltbutton_clicked();

    // 全局音频音量slider值变化事件
    void on_global_volume_slider_valueChanged(int value);

    // 音乐音量slider值变化事件
    void on_music_volume_slider_valueChanged(int value);

    // 效果音量slider值变化事件
    void on_effect_volume_slider_valueChanged(int value);

    // 时间编辑框内容变化事件
    void on_time_lineedit_textChanged(const QString &arg1);

    // 时间编辑框回车按下事件
    void on_time_lineedit_returnPressed();

    // 时间编辑框编辑完成事件
    void on_time_lineedit_editingFinished();

    // 物件宽度缩放调节事件
    void on_owidth_scale_slider_valueChanged(int value);

    // 物件高度缩放调节事件
    void on_oheight_scale_slider_valueChanged(int value);

    // 时间线缩放调节事件
    void on_timeline_zoom_slider_valueChanged(int value);

    // 切换时间格式按钮事件
    void on_time_format_button_clicked();

    // 新建timing按钮事件
    void on_new_timing_button_clicked();

   protected:
    void resizeEvent(QResizeEvent *e) override;

   private:
    Ui::audio_time_controller *ui;

    // 时间线缩放同步锁
    bool timeline_zoom_sync_lock{false};
};

#endif  // TIMECONTROLLER_H
