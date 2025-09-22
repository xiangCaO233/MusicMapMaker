#include <audio/control/audiocontroller.h>

#include <action/modules/canvas/EditorActionHandler.hpp>
#include <action/modules/file/FileActionHandler.hpp>
#include <map/MapCanvas.hpp>
#include <util/mutil.hpp>

// 连接action
void MapCanvas::connectActions() {
    auto thiscp = this;
    // 空格action
    connect(EditorActionHandler::instance(),
            &EditorActionHandler::pause_or_resume_canvas, [thiscp]() {
                auto maintrack = thiscp->map->base_metadata().main_audio_path;
                auto controller = thiscp->audio_callback->getController(
                    maintrack.generic_string());
                auto sourcenode = controller->node();
                auto mapinfo = thiscp->info<MapCanvasInfo>();
                if (auto node = sourcenode.lock()) {
                    // 切换播放状态
                    node->isplaying() ? node->pause() : node->play();
                    mapinfo->realTimeInfo.is_playing = node->isplaying();
                }
            });

    // 保存action
    connect(
        FileActionHandler::instance(), &FileActionHandler::save, [thiscp]() {
            // 直接保存为.mmm
            if (thiscp->map) {
                auto defaultName = QString::fromStdString(
                    thiscp->map->base_metadata().title_unicode + "-" +
                    std::to_string(thiscp->map->base_metadata().track_count) +
                    "k-" + thiscp->map->base_metadata().version);
                qDebug() << "保存文件:" << defaultName;
            }
        });
    // 另存为action
    connect(
        FileActionHandler::instance(), &FileActionHandler::save_as, [thiscp]() {
            // 选择位置和文件名保存为.mmm
            if (thiscp->map) {
                auto defaultName = QString::fromStdString(
                    thiscp->map->base_metadata().title_unicode + "-" +
                    std::to_string(thiscp->map->base_metadata().track_count) +
                    "k-" + thiscp->map->base_metadata().version);
                auto file = mutil::getSaveAsFile(nullptr, tr("save as file"),
                                                 tr("MMM Map File"), ".mmm",
                                                 defaultName);
                if (!file.isEmpty()) {
                    auto fpath = file.toStdString();
                    // 保存为文件
                    qDebug() << "另存为:" << fpath;
                } else {
                    // 取消打开
                    qDebug() << "取消另存为";
                }
            }
        });
    // 导出action
    connect(
        FileActionHandler::instance(), &FileActionHandler::export_as,
        [thiscp]() {
            // 选择位置和文件名保存(可导出多种格式)
            if (thiscp->map) {
                QMap<QString, QString> formats;
                auto mmmf = tr("mmm mapfile");
                auto imdf = tr("imd mapfile");
                auto osuf = tr("osu mapfile");
                formats[mmmf] = ".mmm";
                formats[imdf] = ".imd";
                formats[osuf] = ".osu";

                QMap<QString, QString> defaultNames;

                defaultNames[mmmf] = QString::fromStdString(
                    thiscp->map->base_metadata().title_unicode + "-" +
                    std::to_string(thiscp->map->base_metadata().track_count) +
                    "k-" + thiscp->map->base_metadata().version);
                defaultNames[imdf] = QString::fromStdString(
                    thiscp->map->base_metadata().title_unicode + "_" +
                    std::to_string(thiscp->map->base_metadata().track_count) +
                    "k_" + thiscp->map->base_metadata().version);
                defaultNames[osuf] = QString::fromStdString(
                    thiscp->map->base_metadata().artist + " - " +
                    thiscp->map->base_metadata().title + "(" +
                    thiscp->map->base_metadata().author + ")" + "[" +
                    thiscp->map->base_metadata().version + "]");

                // 指定PNG Image作为默认格式
                auto selected_file = mutil::getSaveDirectoryWithFilename(
                    nullptr, tr("Export As"), tr("File Formats:"), formats,
                    defaultNames, mmmf);

                if (selected_file != "") {
                    thiscp->map->writeOut(selected_file.toStdString());
                    qDebug() << "尝试导出到:" << selected_file;
                    // map->write_to_file(selected_file.toStdString().c_str());
                }
            }
        });
}
