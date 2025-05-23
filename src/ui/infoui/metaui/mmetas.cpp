#include "mmetas.h"

#include <qmainwindow.h>

#include <memory>

#include "../../../mmm/MapWorkProject.h"
#include "../../../mmm/map/MMap.h"
#include "../../../util/mutil.h"
#include "../../GlobalSettings.h"
#include "ui_mmetas.h"

QStringList MMetas::metaNames() {
    return {tr("title"),          tr("title_unicode"),
            tr("artist"),         tr("artist_unicode"),
            tr("author"),         tr("version"),
            tr("audio"),          tr("bg"),
            tr("preference_bpm"), tr("map_length"),
            tr("orbits")};
}

MMetas::MMetas(QWidget *parent) : QWidget(parent), ui(new Ui::MMetas) {
    ui->setupUi(this);
    objinfo_ref = ui->objinfo_widget;
    timinginfo_ref = ui->timinginfo_widget;
    time_audio_controller_ref = ui->time_controller;
    ui->meta_table->setColumnCount(2);
    ui->meta_table->setRowCount(11);
    // 填充数据
    for (int row = 0; row < 11; ++row) {
        table_sync_lock = true;
        QTableWidgetItem *nameitem = new QTableWidgetItem(metaNames()[row]);
        nameitem->setFlags(nameitem->flags() & ~Qt::ItemIsEditable);
        ui->meta_table->setItem(row, 0, nameitem);

        table_sync_lock = true;
        QTableWidgetItem *valueitem = new QTableWidgetItem("");
        valueitem->setFlags(valueitem->flags() | Qt::ItemIsEditable);
        ui->meta_table->setItem(row, 1, valueitem);
    }
    // 设置各列宽度
    ui->meta_table->setColumnWidth(0, 120);
    ui->meta_table->setColumnWidth(1, 200);
}

MMetas::~MMetas() { delete ui; }

void MMetas::switch_map(std::shared_ptr<MMap> map) {
    binding_map = map;
    if (binding_map) {
        // 更新谱面元数据
        table_sync_lock = true;
        ui->meta_table->item(0, 1)->setText(
            QString::fromStdString(binding_map->title));
        table_sync_lock = true;
        ui->meta_table->item(1, 1)->setText(
            QString::fromStdString(binding_map->title_unicode));
        table_sync_lock = true;
        ui->meta_table->item(2, 1)->setText(
            QString::fromStdString(binding_map->artist));
        table_sync_lock = true;
        ui->meta_table->item(3, 1)->setText(
            QString::fromStdString(binding_map->artist_unicode));
        table_sync_lock = true;
        ui->meta_table->item(4, 1)->setText(
            QString::fromStdString(binding_map->author));
        table_sync_lock = true;
        ui->meta_table->item(5, 1)->setText(
            QString::fromStdString(binding_map->version));
        table_sync_lock = true;
        ui->meta_table->item(6, 1)->setText(QString::fromStdString(
            binding_map->audio_file_rpath.generic_string()));
        table_sync_lock = true;
        ui->meta_table->item(7, 1)->setText(
            QString::fromStdString(binding_map->bg_rpath.generic_string()));
        table_sync_lock = true;
        ui->meta_table->item(8, 1)->setText(
            QString::number(binding_map->preference_bpm, 'f', 2));
        table_sync_lock = true;
        ui->meta_table->item(9, 1)->setText(
            QString::number(binding_map->map_length));
        table_sync_lock = true;
        ui->meta_table->item(10, 1)->setText(
            QString::number(binding_map->orbits));
    }
}

// 使用主题
void MMetas::use_theme(GlobalTheme theme) {
    objinfo_ref->use_theme(theme);
    timinginfo_ref->use_theme(theme);

    // 设置时间控制器主题
    ui->time_controller->use_theme(theme);
}

// 表格编辑完成
void MMetas::on_meta_table_cellChanged(int row, int column) {
    // 设置map元数据

    if (column != 1) return;
    auto res = ui->meta_table->item(row, column)->text().toStdString();
    qDebug() << "edit" << row << "," << column;
    switch (row) {
        case 0: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                binding_map->title = mutil::sanitizeFilename_ascii(res);
                table_sync_lock = true;
                ui->meta_table->item(row, column)
                    ->setText(QString::fromStdString(res));
            }
            break;
        }
        case 1: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                binding_map->title_unicode = res;
            }
            break;
        }
        case 2: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                binding_map->artist = mutil::sanitizeFilename_ascii(res);
                table_sync_lock = true;
                ui->meta_table->item(row, column)
                    ->setText(QString::fromStdString(res));
            }
            break;
        }
        case 3: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                binding_map->artist_unicode = res;
            }
            break;
        }
        case 4: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                binding_map->author = res;
            }
            break;
        }
        case 5: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                binding_map->version = res;
            }
            break;
        }
        case 6: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                if (std::filesystem::exists(
                        binding_map->project_reference->ppath / res)) {
                    binding_map->audio_file_rpath = res;
                    binding_map->audio_file_abs_path =
                        binding_map->project_reference->ppath / res;
                    // TODO(xiang 2025-05-23): 更新音频文件
                }
            }
            break;
        }
        case 7: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                if (std::filesystem::exists(
                        binding_map->project_reference->ppath / res)) {
                    binding_map->bg_rpath = res;
                    binding_map->bg_path =
                        binding_map->project_reference->ppath / res;
                    // TODO(xiang 2025-05-23): 更新背景文件
                }
            }
            break;
        }
        case 8: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                // bpm
                if (mutil::isStringAllDigits_Iteration(
                        QString::fromStdString(res))) {
                    binding_map->preference_bpm = std::stod(res);
                }
            }
            break;
        }
        case 9: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                // 长度
                if (mutil::isStringAllDigits_Iteration(
                        QString::fromStdString(res))) {
                    binding_map->map_length = std::stoi(res);
                }
            }
            break;
        }
        case 10: {
            if (table_sync_lock) {
                table_sync_lock = false;
            } else {
                // 轨道数
                if (mutil::isStringAllDigits_Iteration(
                        QString::fromStdString(res))) {
                    binding_map->orbits = std::stoi(res);
                }
            }
            break;
        }
        default:
            break;
    }
}
