#include <audio/control/audiocontroller.h>

#include <action/modules/canvas/EditorActionHandler.hpp>
#include <action/modules/file/FileActionHandler.hpp>
#include <layer/MapLayerManager.hpp>
#include <map/MapCanvas.hpp>
#include <util/mutil.hpp>

// 连接action
void MapCanvas::connectActions() {
    auto thiscp = this;
    // 空格action
    connect(EditorActionHandler::instance(),
            &EditorActionHandler::pause_or_resume_canvas, [thiscp]() {
                if (thiscp->map) {
                    auto maintrack =
                        thiscp->map->base_metadata().main_audio_path;
                    auto controller = thiscp->audio_callback->getController(
                        maintrack.generic_string());
                    auto sourcenode = controller->node();
                    auto mapinfo = thiscp->info<MapCanvasInfo>();
                    if (auto node = sourcenode.lock()) {
                        // 切换播放状态
                        node->isplaying() ? node->pause() : node->play();
                        mapinfo->realTimeInfo.is_playing = node->isplaying();
                    }
                }
            });
    // 复制action
    connect(EditorActionHandler::instance(), &EditorActionHandler::copy,
            [thiscp]() {
                auto layermanager = static_cast<MapLayerManager*>(
                    thiscp->dataloop()->layermanager());
                auto toolInteractionState =
                    layermanager->get_tool_interaction_state();
                if (toolInteractionState->hasSelected()) {
                    // 发送copy指令
                    layermanager->get_tool_cmdq()->push(CopyCommand{
                        toolInteractionState->getSelection(Qt::LeftButton)});
                } else {
                    qDebug() << "未选中任何物件";
                }
            });
    // 剪切action
    connect(EditorActionHandler::instance(), &EditorActionHandler::cut,
            [thiscp]() {
                auto layermanager = static_cast<MapLayerManager*>(
                    thiscp->dataloop()->layermanager());
                auto toolInteractionState =
                    layermanager->get_tool_interaction_state();
                if (toolInteractionState->hasSelected()) {
                    // 发送cut指令
                    layermanager->get_tool_cmdq()->push(CutCommand{
                        toolInteractionState->getSelection(Qt::LeftButton)});
                } else {
                    qDebug() << "未选中任何物件";
                }
            });

    // 粘贴action
    connect(EditorActionHandler::instance(), &EditorActionHandler::paste,
            [thiscp]() {
                auto layermanager = static_cast<MapLayerManager*>(
                    thiscp->dataloop()->layermanager());
                // 发送paste指令
                layermanager->get_tool_cmdq()->push(PasteCommand{});
            });

    // 删除action
    connect(
        EditorActionHandler::instance(), &EditorActionHandler::delete_signal,
        [thiscp]() {
            auto layermanager = static_cast<MapLayerManager*>(
                thiscp->dataloop()->layermanager());
            auto toolInteractionState =
                layermanager->get_tool_interaction_state();
            if (toolInteractionState->hasSelected()) {
                // 连续发送delete标记和确认指令
                layermanager->get_tool_cmdq()->push(MarkDeleteCommand{
                    {}, toolInteractionState->getSelection(Qt::LeftButton)});
                layermanager->get_tool_cmdq()->push(ConfirmDeleteCommand{true});
            } else {
                qDebug() << "未选中任何物件";
            }
        });

    // 镜像action
    connect(EditorActionHandler::instance(), &EditorActionHandler::mirror,
            [thiscp]() {
                auto layermanager = static_cast<MapLayerManager*>(
                    thiscp->dataloop()->layermanager());
                auto toolInteractionState =
                    layermanager->get_tool_interaction_state();
                if (toolInteractionState->hasSelected()) {
                    // 发送镜像指令
                    layermanager->get_tool_cmdq()->push(MirrorCommand{});
                } else {
                    qDebug() << "未选中任何物件";
                }
            });

    // 保存action
    connect(FileActionHandler::instance(), &FileActionHandler::save,
            [thiscp]() {
                // 直接保存为.mmm
                if (thiscp->map) {
                    auto defaultName =
                        QString::fromStdString(
                            thiscp->map->base_metadata().title_unicode + "-" +
                            std::to_string(
                                thiscp->map->base_metadata().track_count) +
                            "k-" + thiscp->map->base_metadata().version) +
                        ".mmm";
                    auto file =
                        thiscp->map->base_metadata().map_path.parent_path() /
                        defaultName.toStdString();
                    auto fpath = file.generic_string();

                    thiscp->map->writeOut(fpath);
                    XINFO("保存文件到:" + fpath);
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
                    thiscp->map->writeOut(fpath);
                    // 保存为文件
                    XINFO("另存为:" + fpath);
                } else {
                    // 取消打开
                    XINFO("取消另存为");
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
                    XINFO("尝试导出到:" + selected_file.toStdString());
                }
            }
        });
}
