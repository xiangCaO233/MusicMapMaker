#include <audio/control/audiocontroller.h>
#include <ui_audiocontroller.h>

void AudioController::on_pause_button_toggled(bool checked) {
    checked ? source_node->pause() : source_node->play();

    // 设置时间编辑框可编辑性(暂停时可编辑)
    ui->time_edit->setEnabled(checked);
}

void AudioController::on_fast_backward_button_clicked() {
    // 快退5s
    source_node->set_playpos(
        uitime_pos - std::chrono::nanoseconds(5LL * 1000 * 1000 * 1000));
    updateDisplayPosition();
}

void AudioController::on_fast_forward_button_clicked() {
    // 快进5s
    source_node->set_playpos(
        uitime_pos + std::chrono::nanoseconds(5LL * 1000 * 1000 * 1000));
    updateDisplayPosition();
}

void AudioController::on_time_edit_editingFinished() {
    // 设置音频时间
    if (!source_node) return;

    auto unit = ui->unit_selection->currentData().value<PositionUnit>();
    QString text = ui->time_edit->text();
    bool ok = true;
    size_t frame = 0;

    switch (unit) {
        case PositionUnit::Frame: {
            frame = text.toULongLong(&ok);
            if (ok) source_node->set_playpos(frame);
            break;
        }
        case PositionUnit::Nanoseconds: {
            long long ns = text.toLongLong(&ok);
            if (ok) {
                source_node->set_playpos(std::chrono::nanoseconds(ns));
                frame = source_node->get_playpos();
            }
            break;
        }
        case PositionUnit::Microseconds: {
            long long us = text.toLongLong(&ok);
            if (ok) {
                source_node->set_playpos(std::chrono::microseconds(us));
                frame = source_node->get_playpos();
            }
            break;
        }
        case PositionUnit::Milliseconds: {
            long long ms = text.toLongLong(&ok);
            if (ok) {
                source_node->set_playpos(std::chrono::milliseconds(ms));
                frame = source_node->get_playpos();
            }
            break;
        }
        case PositionUnit::MinSec: {
            QStringList parts = text.split(':');
            if (parts.size() == 2) {
                long long min = parts[0].toLongLong(&ok);
                if (!ok) break;
                long long sec = parts[1].toLongLong(&ok);
                if (!ok) break;
                source_node->set_playpos(std::chrono::minutes(min) +
                                         std::chrono::seconds(sec));
                frame = source_node->get_playpos();
            }
            break;
        }
        case PositionUnit::MinSecMs: {
            QStringList min_sec = text.split(':');
            if (min_sec.size() != 2) break;
            QStringList sec_ms = min_sec[1].split('.');
            if (sec_ms.size() != 2) break;

            long long min = min_sec[0].toLongLong(&ok);
            if (!ok) break;
            long long sec = sec_ms[0].toLongLong(&ok);
            if (!ok) break;
            long long ms = sec_ms[1].toLongLong(&ok);
            if (!ok) break;

            source_node->set_playpos(std::chrono::minutes(min) +
                                     std::chrono::seconds(sec) +
                                     std::chrono::milliseconds(ms));
            frame = source_node->get_playpos();
            break;
        }
        case PositionUnit::MinSecMsUsNs: {
            // 这个格式非常复杂，用 split 解析
            QStringList parts =
                text.split(QRegularExpression("[:.]"));  // 用:或.分割
            if (parts.size() != 5) break;

            long long min = parts[0].toLongLong(&ok);
            if (!ok) break;
            long long sec = parts[1].toLongLong(&ok);
            if (!ok) break;
            long long ms = parts[2].toLongLong(&ok);
            if (!ok) break;
            long long us = parts[3].toLongLong(&ok);
            if (!ok) break;
            long long ns = parts[4].toLongLong(&ok);
            if (!ok) break;

            source_node->set_playpos(
                std::chrono::minutes(min) + std::chrono::seconds(sec) +
                std::chrono::milliseconds(ms) + std::chrono::microseconds(us) +
                std::chrono::nanoseconds(ns));
            frame = source_node->get_playpos();
            break;
        }
    }

    updateDisplayPosition();
}

void AudioController::on_unit_selection_currentIndexChanged(int index) {
    updateDisplayPosition();
}

void AudioController::on_volume_slider_valueChanged(int value) {
    source_node->setvolume(float(value) / 100.f);
    ui->main_graph->chain()->source->setvolume(float(value) / 100.f);
    // 更新标签
    ui->volume_value_label->setText(QString::number(value));
}
