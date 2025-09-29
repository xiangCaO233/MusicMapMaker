#ifndef MAPCONFIG_H
#define MAPCONFIG_H

#include <QWidget>

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

    bool isCompelete() const { return compeleted; }

    // 更新音频轨道列表
    void update_audio_tracklist(TrackManager *trackmanager);

    // 检查音频轨道是否存在
    int find_track_indexinCombobox();

    void update_ifcompeleted();

   private slots:
    void on_confirm_button_clicked();

    void on_cancel_button_clicked();

   private:
    Ui::MapConfig *ui;
    TrackManager *trackmanager{nullptr};
    MMap *map_ref{nullptr};
    bool compeleted{true};
};

#endif  // MAPCONFIG_H
