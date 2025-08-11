#ifndef MMM_RENDERDATALOOP_HPP
#define MMM_RENDERDATALOOP_HPP

#include <QElapsedTimer>
#include <QObject>
#include <memory>

class LayerManager;
class RenderDataLoop : public QObject {
    Q_OBJECT
   public:
    // 构造RenderTick
    explicit RenderDataLoop(QObject* parent = nullptr);
    // 析构RenderTick
    ~RenderDataLoop() override;

    // 启动循环
    void start();

    // 停止循环
    void stop();

    // 一个数据刻
    void tick();

    // 获取图层管理器指针
    LayerManager* layermanager() const { return layer_manager.get(); }

    // 设置目标fps
    void set_targetFPS(qreal fps);

   signals:
    void renderUpdate();

   protected:
    bool event(QEvent* e) override;

   private:
    // 持有图层管理器
    std::unique_ptr<LayerManager> layer_manager;

    // 是否正在运行
    bool isRunning{false};

    // tick时间(下限2000fps/1ms/1000us/1000000ns)
    uint64_t desiredTicktimeNs{1000000};
    uint64_t actualTicktimeNs{0};

    // 定时器
    QElapsedTimer timer;
};

#endif  // MMM_RENDERDATALOOP_HPP
