#ifndef MAPCONFIG_H
#define MAPCONFIG_H

#include <GlobalSettings.hpp>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>
#include <ice/manage/AudioTrack.hpp>
#include <memory>

namespace Ui {
class MapConfig;
}

class TrackManager;
class MMap;

class MapConfig : public QWidget {
    Q_OBJECT

   public:
    MapConfig(MMap *map, QWidget *parent = nullptr);
    ~MapConfig();
   signals:
    // 当配置的有效/完整状态改变时，发出此信号
    void validityChanged(bool isValid);

   public:
    bool isCompelete() const { return compeleted; }

    // 更新音频轨道列表
    void update_audio_tracklist(TrackManager *trackmanager);

    // 检查音频轨道是否存在
    int find_track_indexinCombobox();

    // 从元数据更新组件
    void update_components_frommeta();

    // 确认完成性
    void update_ifcompeleted();

    void update_ifcompeleted_dialog();

    void use_theme(GlobalTheme theme);

    QPushButton *confirmButton;
    QPushButton *cancleButton;

   public slots:
    void on_confirm_button_clicked();
   private slots:

    void on_cancel_button_clicked();

    void on_load_audio_immediately_button_clicked();

    void on_probe_audio_title_button_clicked();

    void on_probe_audio_artist_button_clicked();

    void on_probe_audio_length_button_clicked();

    void on_audio_track_selectbox_currentTextChanged(const QString &arg1);

    void on_audio_file_browser_button_clicked();

    void on_cover_file_browser_button_clicked();

   private:
    Ui::MapConfig *ui;
    TrackManager *trackmanager{nullptr};
    MMap *map_ref{nullptr};
    bool compeleted{true};
    bool dialog_compeleted{true};
    std::shared_ptr<ice::AudioTrack> selected_track;

    void updateLineEditStatus(QLineEdit *lineEdit);
};

#endif  // MAPCONFIG_H
