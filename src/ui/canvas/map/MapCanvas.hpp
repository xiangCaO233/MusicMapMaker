#ifndef MMM_MAPCANVAS_HPP
#define MMM_MAPCANVAS_HPP

#include <qhash.h>
#include <qobject.h>

#include <canvas/GLCanvas.hpp>
#include <ice/core/PlayCallBack.hpp>
#include <info/MapCanvasInfo.hpp>
#include <map/skin/MSkin.hpp>
#include <memory>
#include <mmm/map/editor/MMapEditor.hpp>
#include <tool/BaseTool.hpp>
#include <unordered_map>
#include <util/StringHash.hpp>

class MMap;
class ToolSystem;

class MapCanvas : public GLCanvas {
    Q_OBJECT
   public:
    // 构造MapCanvas
    MapCanvas();

    // 析构MapCanvas
    ~MapCanvas() override;

    // 切换到图
    void switch_map(MMap *smap);

    // 使用工具
    void use_tool(const QString &tool_name);

    // 获取map
    MMap *get_map() { return map; }

    // 设置主音轨位置
    void set_maintrack_pos(std::chrono::milliseconds time);

    // 绑定音频载入回调
   public slots:
    void onAudioLoadcbkInitialized(AudioLoadCallback *cbk) override;
    void onUpdateTexinfo() override;
    void gotoTiming(Timing *timing) override;
    void onLayoutUpdated();
    void onJudgelinePosUpdated(float pos);

   signals:
    void skinInitialized();
    void toolcmdqInitialized(ThreadSafeQueue<ToolCommand> *q);

   protected:
    void initializeGL() override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

    void resizeEvent(QResizeEvent *event) override;

   private:
    class CanvasAudioPlayCallback : public ice::PlayCallBack {
       public:
        explicit CanvasAudioPlayCallback(MapCanvas *cvs) : canvas(cvs) {};
        // 播放完成完整一遍回调(传入是否循环)
        void play_done(bool loop) const override {}

        // 帧基
        void frameplaypos_updated(size_t frame_pos) override {}

        // 时间基
        void timeplaypos_updated(std::chrono::nanoseconds time_pos) override {
            // 只报告原始音频时间
            if (auto mapinfo = canvas->info<MapCanvasInfo>()) {
                double timems =
                    std::chrono::duration<double, std::milli>(time_pos).count();
                mapinfo->realTimeInfo.current_time_info.raw_audio_time_ms.store(
                    timems);
            }
        }

        // 画布
        MapCanvas *canvas;
    };

    // 主音轨播放回调
    std::shared_ptr<CanvasAudioPlayCallback> maintrack_callback;

    AudioLoadCallback *audio_callback{nullptr};

    // 绑定的谱面
    MMap *map{nullptr};

    // 持有全部的编辑器皮肤
    std::unordered_map<std::string, std::unique_ptr<MSkin>, StringHash,
                       std::equal_to<>>
        editor_skins;

    // 当前使用的皮肤
    MSkin *skin;

    // 工具集合
    QHash<QString, BaseTool *> tools;

    // 当前工具
    BaseTool *current_tool{nullptr};

    // 滚动累积量
    float wheelDyAccumulator{0.f};

    // 创建工具
    void creatTools(ToolSystem *const toolsystem,
                    ThreadSafeQueue<ToolCommand> *const cmdq,
                    ToolInteractionState *const toolIntState);

    // 连接action
    void connectActions();

    friend class MapEditor;
};
#endif  // MMM_MAPCANVAS_HPP
