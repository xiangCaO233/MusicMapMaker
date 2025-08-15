#ifndef MMM_RENDERDATALOOP_HPP
#define MMM_RENDERDATALOOP_HPP

#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <memory>

class LayerManager;
class Renderer2D;
class MMap;
class SharedCanvasInfo;

class RenderDataLoop : public QObject {
    Q_OBJECT
   public:
    // 构造RenderTick
    explicit RenderDataLoop(Renderer2D* renderer, QObject* parent = nullptr);
    // 析构RenderTick
    ~RenderDataLoop() override;

    // 初始化层管理器
    virtual void initializeLayerManager();

    // 获取图层管理器指针
    LayerManager* layermanager() const { return layer_manager.get(); }

    // 更新信息
    void update_info(SharedCanvasInfo* newinfo);

    // 设置目标fps
    void set_targetFPS(qreal fps);

    // 启动循环
    void start();

    // 停止循环
    void stop();

   public slots:
    // 1s接收一个
    void updateFPS(int fps);

   signals:
    void renderUpdate();

   protected:
    // 一个数据刻
    void tick();

    // 获取信息
    SharedCanvasInfo* getinfo() { return info; }

    // qt的事件
    bool event(QEvent* e) override;

    // 内部访问原始的图层管理器
    std::unique_ptr<LayerManager>& manager() { return layer_manager; }

    // map信息
    SharedCanvasInfo* info;

    // 可重写的tick事件(执行其他任务)
    virtual void pre_tickEvent();
    virtual void tickEvent();
    virtual void after_tickEvent();

    // 访问渲染器
    Renderer2D* renderer() { return render; }

    // 可重写的更新map接口
    virtual void updateMap(MMap* map);

   private:
    Renderer2D* render;
    // 持有图层管理器
    std::unique_ptr<LayerManager> layer_manager;

    // 是否正在运行
    bool isRunning{false};

    // tick时间(下限2000fps/1ms/1000us/1000000ns)
    uint64_t desiredTicktimeNs{1000000};
    uint64_t actualTicktimeNs{0};

    qreal desiredFps;

    // 当前fps
    qreal current_fps;

    // 一个变量来平滑地存储我们计算出的、理想的睡眠时间
    std::atomic<int64_t> sleepAdjustmentNs{0};

    // 积分项，累积误差
    double integral_error_ns{0.0};

    // 需要记录上一次的误差
    double last_error_ns{0.0};

    // 过去N秒的FPS历史记录 (滑动窗口)
    QQueue<int> fps_history;

    // 窗口大小：5秒
    const int HISTORY_SECONDS = 5;

    // 定时器
    QElapsedTimer timer;
};

#endif  // MMM_RENDERDATALOOP_HPP
