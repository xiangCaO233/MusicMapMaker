#ifndef TIMECONTROLLER_H
#define TIMECONTROLLER_H

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

    // 更新全局音量按钮(主题)
    void update_global_volume_button();

    // 更新音频状态
    void update_audio_status();

    // 更新暂停按钮
    void update_pause_button();

   public slots:
    // page选择了新map事件
    void on_selectnewmap(std::shared_ptr<MMap> &map);

    // 画布时间变化事件
    void on_canvas_timestamp_changed(double time);

    // 画布暂停槽
    void on_canvas_pause(bool paused);

    // 实时信息变化槽
    void on_current_bpm_changed(double bpm);
    void on_current_timeline_speed_changed(double timeline_speed);

    // 画布调节时间线缩放
    void on_canvas_adjust_timeline_zoom(int value);

   signals:
    // 时间控制器编辑时间信号
    void time_edited(double time);

    // 时间控制器暂停信号
    void pause_button_changed(bool paused);

    // 时间控制器播放速度变化信号
    void playspeed_changed(double speed);

    // 音乐位置同步信号
    void music_pos_synchronized(double time);

   private slots:
    // 暂停按钮
    void on_pausebutton_clicked();

    // 变速设置
    void on_doubleSpinBox_valueChanged(double arg1);

    // 变速设置完成
    void on_doubleSpinBox_editingFinished();

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

    // 静音全局按钮事件
    void on_reset_global_volume_button_clicked();

    // 静音音乐按钮事件
    void on_reset_music_volume_button_clicked();

    // 静音效果按钮事件
    void on_reset_effect_volume_button_clicked();

    // 时间编辑框内容变化事件
    void on_lineEdit_textChanged(const QString &arg1);

    // 时间编辑框回车按下事件
    void on_lineEdit_returnPressed();

    // 时间编辑框编辑完成事件
    void on_lineEdit_editingFinished();

    // 物件宽度缩放调节事件
    void on_owidth_scale_slider_valueChanged(int value);

    // 物件高度缩放调节事件
    void on_oheight_scale_slider_valueChanged(int value);

    // 时间线缩放调节事件
    void on_timeline_zoom_slider_valueChanged(int value);

    // 重置宽度缩放按钮
    void on_owidth_scale_button_clicked();

    // 重置高度缩放按钮
    void on_oheight_scale_button_clicked();

    // 重置时间线缩放按钮
    void on_timeline_zoom_button_clicked();

    // 切换时间格式按钮事件
    void on_time_format_button_clicked();

   private:
    Ui::audio_time_controller *ui;

    // 时间线缩放同步锁
    bool timeline_zoom_sync_lock{false};
};

#endif  // TIMECONTROLLER_H
