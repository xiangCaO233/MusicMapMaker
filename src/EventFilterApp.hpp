#ifndef M_EVENTFILTERAPP_H
#define M_EVENTFILTERAPP_H

#include <qapplication.h>

#include <QObject>

#include "CanvasContainer.h"
#include "meditorarea.h"
#include "ui/mainwindow.h"

class EventFilterApp : public QApplication {
   public:
    MainWindow w;
    EventFilterApp(int &argc, char **argv) : QApplication(argc, argv) {}

    bool notify(QObject *receiver, QEvent *event) override {
        if (event->type() == QEvent::KeyPress ||
            event->type() == QEvent::KeyRelease) {
            // 获取全局的 MapWorkspaceCanvas 实例（需通过单例或全局指针）
            if (auto *canvas =
                    w.page->edit_area_widget->canvas_container->canvas.data()) {
                canvas->requestActivate();
                QApplication::sendEvent(canvas, event);
                return true;
            }
        }
        return QApplication::notify(receiver, event);
    }
};

#endif  // M_EVENTFILTERAPP_H
