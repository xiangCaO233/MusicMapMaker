#ifndef MMM_RENDERLOOPWORKER_HPP
#define MMM_RENDERLOOPWORKER_HPP

#include <QObject>

class RenderDataLoop;
class RenderLoopWorker : public QObject {
    Q_OBJECT
   public:
    explicit RenderLoopWorker(RenderDataLoop* outer);
   public slots:
    void doWork();  // 这就是我们的新主循环
   private:
    RenderDataLoop* d;  // 指向“大脑” RenderDataLoop 的指针
};

#endif  // MMM_RENDERLOOPWORKER_HPP
