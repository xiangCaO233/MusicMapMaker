#ifndef M_MAPWORKSPACE_H
#define M_MAPWORKSPACE_H

#include <qpaintdevice.h>
#include <qpoint.h>
#include <qtmetamacros.h>

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../../mmm/Beat.h"
#include "../../mmm/map/MMap.h"
#include "../../threads/ThreadSafeEffect.h"
#include "../GLCanvas.h"
#include "MapWorkspaceSkin.h"
#include "editor/MapEditor.h"
#include "generator/AreaInfoGenerator.h"
#include "generator/BeatGenerator.h"
#include "generator/JudgelineGenerator.h"
#include "generator/ObjectGenerator.h"
#include "generator/OrbitGenerator.h"
#include "generator/PreviewGenerator.h"
#include "generator/TimeInfoGenerator.h"
#include "threads/BackupThread.h"
#include "threads/EffectThread.h"
#include "threads/ThreadPool.h"

class MapWorkspaceCanvas : public GLCanvas {
    Q_OBJECT
   protected:
    // qt事件
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void initializeGL() override;
    // 线程池
    ThreadPool canvas_tpool;

    BackupThread backup_thread;

    /*
     *图形相关
     */
    // 物件生成器
    std::unordered_map<NoteType, std::shared_ptr<ObjectGenerator>>
        objgenerators;

    // 节拍生成器
    std::shared_ptr<BeatGenerator> beatgenerator;

    // 时间区域信息生成器
    std::shared_ptr<AreaInfoGenerator> areagenerator;

    // 时间信息生成器
    std::shared_ptr<TimeInfoGenerator> timegenerator;

    // 预览生成器
    std::shared_ptr<PreviewGenerator> previewgenerator;

    // 轨道生成器
    std::shared_ptr<OrbitGenerator> orbitgenerator;

    // 判定线生成器
    std::shared_ptr<JudgelineGenerator> judgelinegenerator;

    // x位置-特效帧队列
    std::unordered_map<double, ThreadSafeEffect> effect_frame_queue_map;

    /*
     *成员函数
     */
    // 绘制轨道底板
    void draw_orbits(BufferWrapper *bufferwrapper);

    // 绘制判定线
    void draw_judgeline(BufferWrapper *bufferwrapper);

    // 绘制选中框
    void draw_select_bound(BufferWrapper *bufferwrapper);

    // 绘制信息区
    void draw_infoarea(BufferWrapper *bufferwrapper);

    // 绘制拍
    void draw_beats(BufferWrapper *bufferwrapper);

    enum class HitObjectEffect {
        // 通常模式
        NORMAL,
        // 虚影模式
        SHADOW,
    };

    // 绘制物件
    void draw_hitobject(
        BufferWrapper *bufferwrapper,
        std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> &objects,
        HitObjectEffect e);

    // 绘制顶部栏
    void draw_top_bar(BufferWrapper *bufferwrapper);

    // 绘制背景
    void draw_background(BufferWrapper *bufferwrapper);

    // 绘制预览
    void draw_preview_content(BufferWrapper *bufferwrapper);

    void draw_effect_frame(BufferWrapper *bufferwrapper);

   public:
    // 构造MapWorkspaceCanvas
    explicit MapWorkspaceCanvas(QWidget *parent = nullptr);

    // 析构MapWorkspaceCanvas
    ~MapWorkspaceCanvas() override;

    // 实际的帧更新时间-qt
    double actual_update_time{0};

    // 平均帧更新时间-qt
    double average_update_time{0};

    // 编辑器
    std::shared_ptr<MapEditor> editor;

    // 效果线程
    std::unique_ptr<EffectThread> effect_thread;

    // 正在工作的图
    std::shared_ptr<MMap> working_map;

    std::mutex skin_mtx;
    // 皮肤
    MapWorkspaceSkin skin;

    // 播放过特效的物件
    std::unordered_set<std::shared_ptr<HitObject>> played_effects_objects;

    // 播放特效
    void play_effect(double xpos, double ypos, double play_time,
                     EffectType etype);

    // 切换到指定图
    void switch_map(std::shared_ptr<MMap> map);

    // 获取暂停状态
    inline bool &is_paused() { return editor->cstatus.canvas_pasued; }

    // 渲染实际图形
    void push_shape(BufferWrapper *current_back_buffer) override;

    void remove_objects(std::shared_ptr<HitObject> o);
    // 更新fps显示
    virtual void updateFpsDisplay(int fps) override;

    void save();

   public slots:
    // 时间控制器暂停按钮触发
    void on_timecontroller_pause_button_changed(bool paused);

    // 时间控制器播放速度变化
    void on_timecontroller_speed_changed(double speed);

    // 同步音频播放时间
    void on_music_pos_sync(double time);

    // 时间编辑器设置精确时间
    void on_timeedit_setpos(double time);

   signals:
    // 时间戳更新信号
    void current_time_stamp_changed(double current_time_stamp);

    // 根据快捷键切换模式
    void switch_edit_mode(MouseEditMode mode);

    // 绝对bpm变化信号
    void current_absbpm_changed(double bpm);

    // 时间线速度变化信号
    void current_timeline_speed_changed(double timeline_speed);

    // 选中物件的信号
    void select_object(Beat *beatinfo, std::shared_ptr<HitObject> obj,
                       std::shared_ptr<Timing> ref_timing);

    // 选中timing的信号
    void select_timing(std::vector<std::shared_ptr<Timing>> *timings);

    // 是否暂停播放信号
    void pause_signal(bool paused);

    // 时间线缩放调节信号
    void timeline_zoom_adjusted(int value);
};

#endif  // M_MAPWORKSPACE_H
