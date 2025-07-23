#ifndef MMM_HIDEABLETOOLWINDOW_HPP
#define MMM_HIDEABLETOOLWINDOW_HPP

#include <qtmetamacros.h>
#include <qwidget.h>

class HideableToolWindow : public QWidget {
    Q_OBJECT
   public:
    // 构造HideableToolWindow
    HideableToolWindow(QWidget* parent = nullptr);

    // 析构HideableToolWindow
    ~HideableToolWindow() override;
   signals:
    void close_signal();

   protected:
    void closeEvent(QCloseEvent* event) override;

   protected:
};

#endif  // MMM_HIDEABLETOOLWINDOW_HPP
