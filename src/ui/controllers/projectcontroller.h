#ifndef PROJECTCONTROLLER_H
#define PROJECTCONTROLLER_H

#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

namespace Ui {
class MProjectController;
}

class MapWorkProject;
class MMap;

class MProjectController : public QWidget {
    Q_OBJECT

   public slots:
    // 新项目槽函数
    void new_project(std::shared_ptr<MapWorkProject> &project);

   public:
    explicit MProjectController(QWidget *parent = nullptr);

    ~MProjectController();

    // 设备同步锁
    bool device_sync_lock{false};

    // 当前选中的制谱工程
    std::shared_ptr<MapWorkProject> selected_project{nullptr};

    // 制谱工程映射表
    std::unordered_map<std::string, std::shared_ptr<MapWorkProject>>
        project_mapping;

    // 最后一次打开的文件目录
    QString last_select_directory;

    // 选择项目
    void select_project(std::shared_ptr<MapWorkProject> &project);

   signals:
    void select_map(std::shared_ptr<MMap> &map);

   private slots:
    // 选择项目事件
    void on_project_selector_currentIndexChanged(int index);

    // 谱面列表双击事件
    void on_map_list_view_doubleClicked(const QModelIndex &index);

    // 音频列表双击事件
    void on_audio_list_view_doubleClicked(const QModelIndex &index);

    // 图片列表双击事件
    void on_image_list_view_doubleClicked(const QModelIndex &index);

    // 视频列表双击事件
    void on_video_list_view_doubleClicked(const QModelIndex &index);

    // 谱面列表上下文菜单事件
    void on_map_list_view_customContextMenuRequested(const QPoint &pos);

    // 音频列表上下文菜单事件
    void on_audio_list_view_customContextMenuRequested(const QPoint &pos);

    // 图片列表上下文菜单事件
    void on_image_list_view_customContextMenuRequested(const QPoint &pos);

    // 视频列表上下文菜单事件
    void on_video_list_view_customContextMenuRequested(const QPoint &pos);

    // 关闭项目事件
    void on_close_project_button_clicked();

    // 为项目选择音频输出设备事件
    void on_audio_device_selector_currentTextChanged(const QString &arg1);

   private:
    Ui::MProjectController *ui;
};

#endif  // PROJECTCONTROLLER_H
