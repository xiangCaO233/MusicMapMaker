#ifndef MMM_TOOLINTERACTIONSTATE_HPP
#define MMM_TOOLINTERACTIONSTATE_HPP

#include <qnamespace.h>
#include <qreadwritelock.h>

#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <entt.hpp>
#include <glm/glm.hpp>
#include <info/NotePart.hpp>
#include <list>
#include <mmm/obj/Note.hpp>
#include <mutex>
#include <unordered_map>
#include <util/mutil.hpp>
#include <vector>

// 鼠标状态
struct MouseState {
    // 当前光标位置 (高频更新)
    glm::vec2 current_pos;
    // 鼠标按下的位置
    std::unordered_map<Qt::MouseButton, glm::vec2> press_pos;
    // 鼠标轨迹 (用于特效)
    std::vector<glm::vec2> trail;
    // 当前按下的按钮
    QFlags<Qt::MouseButton> pressed_buttons;
};

// 选择状态
struct SelectionState {
    // 按按钮类型对选择区域进行分组存储
    // Key: 鼠标按钮, Value: 该按钮创建的所有选择框的列表
    std::map<Qt::MouseButton, std::vector<glm::vec4>> per_button_absolute_areas;

    // Key: 鼠标按钮, Value: 该按钮创建的所有选择框的时间区域列表
    // vec4:xstart,starttime,width,duration
    std::map<Qt::MouseButton, std::vector<glm::vec4>> per_button_areas;

    struct ActiveSession {
        // 绝对的鼠标按下时的像素坐标
        glm::vec2 start_absolute_pos;
        // 鼠标按下时的时间坐标
        // vec2:xstart,starttime
        glm::vec2 start_pos;
        size_t area_index_in_button_vector;
    };

    // 用一个 map 跟踪所有活动的会话
    std::map<Qt::MouseButton, ActiveSession> active_sessions;
    std::map<Qt::MouseButton, std::unordered_set<entt::entity>>
        all_selected_entities;
};

// 剪切板
struct NoteClipboard {
    bool is_copy;
    std::unordered_set<NoteUUID> uuids;
};

// 删除标记状态
struct DeleteMarkStates {
    std::unordered_set<entt::entity> marked_entities;
};

// 拖拽状态
enum class DragMode {
    // 无拖动内容:生成选择框
    None,
    // 拖拽单个实体 (或多个已选实体)
    Entity,
    // 拖拽选择框
    Marquee,
};

enum class CreateMode {
    // 无创建内容
    None,
    // 单键
    Normal,
    // 复合
    Composite,
};

struct MapAxis {
    int64_t time{0};
    int64_t mousetime{0};
    int64_t track{0};
    int64_t x{0};
    int64_t y{0};

    bool operator==(const MapAxis& other) const {
        return time == other.time && track == other.track;
    }

    MapAxis operator-(const MapAxis& other) const {
        return {time - other.time, mousetime - other.mousetime,
                track - other.track, x - other.x, y - other.y};
    }

    MapAxis operator+(const MapAxis& other) const {
        return {time + other.time, mousetime + other.mousetime,
                track + other.track, x + other.x, y + other.y};
    }
};

struct DragState {
    // 拖拽模式
    DragMode mode{DragMode::None};
    // 拖拽操作是否有效
    bool is_valid{true};
    // 仅在 mode == Entity 时有效
    // 拖拽开始时的命中信息 (部位、实体等)
    MeshPartInfo drag_start_hit;
    // 实际被拖拽的实体集合
    std::unordered_map<entt::entity, MapAxis> dragged_entitiesWithRes;
};

struct CreateState {
    // 创建模式
    CreateMode mode;
    // 创建结果是否合法
    bool is_valid{true};
    // 创建出的节点路径
    std::list<MapAxis> createState_nodes;
};

// 主结构ToolInteractionState
class ToolInteractionState {
   public:
    // 构造函数
    ToolInteractionState() = default;

    // 线程安全的公共接口

    // 鼠标相关 (由UI线程写入, 所有线程读取)
    void updateMousePress(const glm::vec2& pos, Qt::MouseButton button,
                          QFlags<Qt::MouseButton> allButtons);
    void updateMouseMove(const glm::vec2& pos,
                         QFlags<Qt::MouseButton> allButtons);
    void updateMouseRelease(const glm::vec2& pos, Qt::MouseButton button,
                            QFlags<Qt::MouseButton> allButtons);

    MouseState getMouseState() const;

    glm::vec2 getCurrentMousePos() const;
    glm::vec2 getMousePressPos(const Qt::MouseButton button) const;

    // 悬浮相关 (由 pretick 写入, 工作线程读取)
    void setHover(const std::optional<MeshPartInfo> hover);
    std::optional<MeshPartInfo> getHover() const;

    // 拖拽相关 (由 pretick 写入, 工作线程读取)
    void startDrag(DragMode mode, MeshPartInfo hit,
                   const std::unordered_map<entt::entity, MapAxis>& selection);
    void endDrag();

    void setDragValidity(bool isValid);
    void setDragValidRes(const entt::entity& e, const MapAxis& axis);

    DragState getDragState() const;

    // 创建相关(由 pretick 写入, 工作线程读取)
    void startCreate(CreateMode mode);
    void updateCreateNode(const MapAxis& axis);
    void setCreateValidity(bool isValid);
    CreateState getCreateState() const;
    void endCreate();

    // 删除相关
    void startDeleteCheck(const std::unordered_set<entt::entity>& selection);
    void endDeleteCheck();
    DeleteMarkStates getDeleteMarkStates() const;

    // 选择相关 (由 pretick 写入, 所有线程读取)
    void startNewSelectArea(bool append, Qt::MouseButton button,
                            const glm::vec2& start_absolute_pos,
                            const glm::vec2& start_time_pos);
    void updateSelectArea(QFlags<Qt::MouseButton> current_buttons,
                          const TimePixelConverter& converter,
                          float canvas_height, float judgeline_abspos,
                          float current_canvas_time);
    void endNewSelectArea(Qt::MouseButton released_button);
    void setSelection(Qt::MouseButton button,
                      const std::unordered_set<entt::entity>& entities);
    std::unordered_set<entt::entity> getSelection(Qt::MouseButton button);

    bool isSelected(entt::entity entity_to_check) const;
    bool hasSelected() const;
    SelectionState getSelectionState() const;

    // 剪切板相关
    void setClipBoard(const std::unordered_set<NoteUUID>& uuids, bool is_copy);
    NoteClipboard getClipBoard() const;

    // 操作/快捷键相关 (由UI/Action系统写入, pretick读取)
    // 这个可以用一个更简单的命令队列，或者一个原子标志位
    // void triggerAction(ActionType action);

   private:
    // 使用一个互斥锁保护所有状态的读写，确保一致性
    // 对于高频读写的场景，可以考虑为不同子结构使用不同的锁，或读写锁
    mutable std::mutex m_mutex;

    // 私有状态变量
    MouseState m_mouseState;
    // mutable是关键, 它允许在const成员函数中锁定和解锁非const的锁
    mutable QReadWriteLock m_mouseStateLock;
    // 当前悬浮的对象
    std::optional<MeshPartInfo> m_hovered;
    DragState m_dragState;
    CreateState m_createState;
    SelectionState m_selectionState;
    DeleteMarkStates m_deleteMarkState;

    // 剪切板
    NoteClipboard m_noteClipboard;

    // 聚合选中区
    std::unordered_set<entt::entity> m_aggregated_selection;

    // 重建聚合选中区
    void rebuildAggregatedSelection();
};

#endif  // MMM_TOOLINTERACTIONSTATE_HPP
