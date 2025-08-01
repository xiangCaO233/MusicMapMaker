#include <audio/control/audiocontroller.h>
#include <ui_audiocontroller.h>

void AudioController::on_gainslider_31_valueChanged(int value) {
    ui->db_31gain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(0, value);
    ui->main_graph->chain()->eq->set_band_gain_db(0, value);
}

void AudioController::on_gainslider_63_valueChanged(int value) {
    ui->db_63gain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(1, value);
    ui->main_graph->chain()->eq->set_band_gain_db(1, value);
}

void AudioController::on_gainslider_125_valueChanged(int value) {
    ui->db_125gain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(2, value);
    ui->main_graph->chain()->eq->set_band_gain_db(2, value);
}

void AudioController::on_gainslider_250_valueChanged(int value) {
    ui->db_250gain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(3, value);
    ui->main_graph->chain()->eq->set_band_gain_db(3, value);
}

void AudioController::on_gainslider_500_valueChanged(int value) {
    ui->db_500gain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(4, value);
    ui->main_graph->chain()->eq->set_band_gain_db(4, value);
}

void AudioController::on_gainslider_1k_valueChanged(int value) {
    ui->db_1kgain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(5, value);
    ui->main_graph->chain()->eq->set_band_gain_db(5, value);
}

void AudioController::on_gainslider_2k_valueChanged(int value) {
    ui->db_2kgain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(6, value);
    ui->main_graph->chain()->eq->set_band_gain_db(6, value);
}

void AudioController::on_gainslider_4k_valueChanged(int value) {
    ui->db_4kgain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(7, value);
    ui->main_graph->chain()->eq->set_band_gain_db(7, value);
}

void AudioController::on_gainslider_8k_valueChanged(int value) {
    ui->db_8kgain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(8, value);
    ui->main_graph->chain()->eq->set_band_gain_db(8, value);
}

void AudioController::on_gainslider_16k_valueChanged(int value) {
    ui->db_16kgain->setText(QString("%1db").arg(value));
    process_chain->eq->set_band_gain_db(9, value);
    ui->main_graph->chain()->eq->set_band_gain_db(9, value);
}

void AudioController::on_q_31_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(0, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(0, arg1);
}

void AudioController::on_q_63_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(1, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(1, arg1);
}

void AudioController::on_q_125_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(2, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(2, arg1);
}

void AudioController::on_q_250_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(3, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(3, arg1);
}

void AudioController::on_q_500_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(4, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(4, arg1);
}

void AudioController::on_q_1k_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(5, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(5, arg1);
}

void AudioController::on_q_2k_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(6, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(6, arg1);
}

void AudioController::on_q_4k_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(7, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(7, arg1);
}

void AudioController::on_q_8k_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(8, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(8, arg1);
}

void AudioController::on_q_16k_valueChanged(double arg1) {
    process_chain->eq->set_band_q_factor(9, arg1);
    ui->main_graph->chain()->eq->set_band_q_factor(9, arg1);
}
