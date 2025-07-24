#ifndef MMM_AUTORESIZETABLEWIDGET_H
#define MMM_AUTORESIZETABLEWIDGET_H

#include <qlist.h>
#include <qtablewidget.h>
#include <qtmetamacros.h>
#include <qwidget.h>

class AutoResizeTableWidget : public QTableWidget {
    Q_OBJECT
   public:
    // 构造AutoResizeTableWidget
    explicit AutoResizeTableWidget(QWidget* parent = nullptr);

    // 设置拉伸比例
    void setColumStretchs(QList<int>&& stretchs);

   private slots:
    void columsection_resized(int logicalIndex, int oldSize, int newSize);

   protected:
    void resizeEvent(QResizeEvent* e) override;

    bool eventFilter(QObject* watched, QEvent* event) override;

   private:
    // 列拉伸比例
    QList<int> colum_stretchs;
    bool user_interective{false};
    void updateStretchRatios();
};

#endif  // MMM_AUTORESIZETABLEWIDGET_H
