#ifndef MMM_RENDERDATALOOP_HPP
#define MMM_RENDERDATALOOP_HPP

#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <map/MapCanvasClock.hpp>
#include <memory>

class LayerManager;
class Renderer2D;
class MMap;
class SharedCanvasInfo;
class RenderLoopWorker;

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
    friend class RenderLoopWorker;  // 允许私有工作类访问RenderDataLoop的成员
    // --- 您的应用程序对象 (保留) ---
    Renderer2D* render;
    MapCanvasClock canvas_clock;
    std::unique_ptr<LayerManager> layer_manager;
    // ... getinfo() 和 pre/tick/after_tickEvent() 的声明 ...

    // --- 循环控制 (保留) ---
    std::atomic<bool> isRunning{false};
    QElapsedTimer timer;

    // --- 节拍器相关成员 (来自新方案) ---
    double desired_fps{60.0};
    qint64 desired_frame_time_ns{16666666};
    qint64 next_tick_time_ns{0};

    // --- 平滑 Delta Time (保留) ---
    const double m_delta_smoothing_factor = 0.1;
    double m_smoothed_delta_ms{16.6};
    // 【新增】一个指向内部工作线程的指针
    QThread* m_worker_thread = nullptr;
    RenderLoopWorker* m_worker = nullptr;  // 工作者对象
};

#endif  // MMM_RENDERDATALOOP_HPP
