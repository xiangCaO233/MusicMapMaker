#ifndef NEWTIMINGGUIDE_H
#define NEWTIMINGGUIDE_H

#include <QDialog>
#include <memory>

class MMap;
class Timing;

namespace Ui {
class NewTimingGuide;
}

class NewTimingGuide : public QDialog {
    Q_OBJECT

   public:
    explicit NewTimingGuide(std::shared_ptr<MMap> binding_map,
                            QWidget *parent = nullptr);
    ~NewTimingGuide();

    std::shared_ptr<MMap> map;

    // 可被继承的timing
    std::shared_ptr<Timing> inheritable_timing;

    double timestamp;
    double bpm;
    double speed;
    bool inheritance_pretiming{false};

    bool inheritable{false};

    void set_time(double time);
    void set_inheritable(bool flag = true);

   private slots:
    // 控件事件
    void on_timestamp_edit_textChanged(const QString &arg1);

    void on_bpm_edit_textChanged(const QString &arg1);

    void on_play_speed_edit_textChanged(const QString &arg1);

    void on_inheritable_checkbox_checkStateChanged(const Qt::CheckState &arg1);

    void on_enable_speed_changing_checkStateChanged(const Qt::CheckState &arg1);

    void on_adapt_to_preferencebpm_clicked();

   private:
    Ui::NewTimingGuide *ui;
};

#endif  // NEWTIMINGGUIDE_H
