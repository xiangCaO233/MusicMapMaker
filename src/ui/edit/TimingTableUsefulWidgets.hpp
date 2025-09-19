#ifndef MMM_TIMINGTABLEUSEFULWIDGETS_HPP
#define MMM_TIMINGTABLEUSEFULWIDGETS_HPP

#include <qnamespace.h>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

class Timing;
class MMap;

// 时间编辑组件
class TimeEditWidget : public QWidget {
    Q_OBJECT
   public:
    TimeEditWidget(Timing *&timing, QTableWidget *parent, int index,
                   QIntValidator *validator);
    QTableWidget *parent;
    QHBoxLayout *layout;
    QLabel *timingIndex;
    QLineEdit *timeEdit;
    QPushButton *gotoButton;
};

class TimingParameterEditor : public QWidget {
    Q_OBJECT
   public:
    TimingParameterEditor(Timing *&timing, QTableWidget *parent,
                          QDoubleValidator *validator);
    QHBoxLayout *layout;
    QString speedTitle = tr("speed:");
    QString bpmTitle = "bpm:";
    QLabel *title;
    QLineEdit *bpmEdit;
    QDoubleSpinBox *speedSpinBox;
};

// 时间点设置组件
class TimingSettingWidget : public QWidget {
    Q_OBJECT
   public:
    TimingSettingWidget(Timing *&timing, QTableWidget *parent);
    QTableWidget *parent;
    QHBoxLayout *layout;
    QSpacerItem *spacer;
    QPushButton *deleteButton;
    QPushButton *doneButton;
};

class TimingRowItem : public QObject {
    Q_OBJECT
   public:
    TimingRowItem(MMap *map, Timing *&timing, QTableWidget *parent, int index,
                  QIntValidator *intvalidator,
                  QDoubleValidator *doublevalidator);
    Timing *timing;
    TimeEditWidget *timeEditWgt;
    QComboBox *uninheritedComboBox;
    TimingParameterEditor *paramEditor;
    TimingSettingWidget *timingSettingWgt;
};

class AddTimingItem {
   public:
    AddTimingItem(QTableWidget *parent);
    QTableWidget *parent;
    QWidget *setting_widget;
    QHBoxLayout *setting_layout;
    QSpacerItem *setting_spacer;
    QPushButton *insertButton;
};

#endif  // MMM_TIMINGTABLEUSEFULWIDGETS_HPP
