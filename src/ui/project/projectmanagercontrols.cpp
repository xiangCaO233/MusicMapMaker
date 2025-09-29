#include <mainwindow.h>
#include <projectmanager.h>
#include <qdir.h>
#include <qlogging.h>
#include <ui_projectmanager.h>

#include <QDesktopServices>
#include <map/MapCanvas.hpp>
#include <util/mutil.hpp>

void ProjectManager::on_map_listView_doubleClicked(const QModelIndex& mindex) {
    // 打开谱面

    // 获取当前激活的项目
    auto project = service->currentPorject();
    auto map = qobject_cast<QStandardItemModel*>(ui->map_listView->model())
                   ->data(mindex, Qt::UserRole + 1)
                   .value<MMap*>();
    service->selectMap(project->project_path.generic_string(), map);
}

void ProjectManager::on_map_listView_customContextMenuRequested(
    const QPoint& pos) {
    // 谱面列表上下文菜单事件
    // 使用当前选中项而不是点击位置
    QModelIndex index = ui->map_listView->currentIndex();

    // 生成菜单
    QMenu menu;

    // 根据点击位置添加菜单项
    if (!index.isValid()) {
    } else {
        // 获得选中的map
        auto selected_map =
            qobject_cast<QStandardItemModel*>(ui->map_listView->model())
                ->data(index, Qt::UserRole + 1)
                .value<MMap*>();

        // 在文件管理器中打开
        menu.addAction(
            tr("Open In FileBrowser"), [
                                           // 复制捕获防止智能指针引用丢失
                                           = ]() {
                // qDebug() << selected_map->map_name;
                // 获取文件上一级路径
                QDir dir(selected_map->base_metadata().map_path.parent_path());
                // 转换为本地文件URL
                QUrl url = QUrl::fromLocalFile(dir.absolutePath());
                // 使用文件管理器打开
                QDesktopServices::openUrl(url);
            });
        // 打开配置菜单
        menu.addAction(tr("Config"),
                       [selected_map]() { selected_map->show_configui(); });
    }

    // 添加通用菜单项
    menu.addSeparator();
    menu.addAction(tr("Import Map"), [&]() {
        auto options = QFileDialog::DontUseNativeDialog;
        auto fileNames = QFileDialog::getOpenFileNames(
            this, tr("Select Map"), XLogger::last_select_directory,
            tr("Map File(*.osu *.imd *.mc)"), nullptr, options);

        // TODO: 实现项目中导入谱面
        for (auto& name : fileNames) {
            XINFO("Selected:" + name.toStdString());
        }
    });

    menu.addAction(tr("Create New Map"), [&]() {
        // 调用创建谱面函数
        XINFO("Create New Map");
    });

    // 显示菜单
    menu.exec(ui->map_listView->viewport()->mapToGlobal(pos));
}
