#ifndef MMM_MAPLAYERMANAGER_HPP
#define MMM_MAPLAYERMANAGER_HPP

#include <ecs/system/ToolSystem.hpp>
#include <ecs/system/time2pixel/EffectedTimeConverter.hpp>
#include <ecs/system/time2pixel/LinearTimeConverter.hpp>
#include <layer/LayerManager.hpp>
#include <layer/note/NoteLayerGenerator.hpp>
#include <tool/ThreadSafeQueue.hpp>
#include <tool/ToolInteractionState.hpp>

class TimePixelConverterManager {
   public:
    TimePixelConverterManager() = default;

    /**
     * @brief 获取一个最新的、可用的TimePixelConverter实例。
     * @details 内部处理缓存和重建逻辑。
     */
    const TimePixelConverter& getConverter(const TimingMap& timings,
                                           const BaseCanvasStatus& status,
                                           const MapCanvasInfo* info,
                                           double prebpm) {
        // 检查TimingMap的版本号是否已更新
        if (!m_effected_cached_converter || !m_linear_cached_converter ||
            m_cached_version != timings.getVersion() ||
            !current_status_version || status != *current_status_version) {
            std::lock_guard<std::mutex> lock(build_newconverter_mtx);

            // 版本不匹配或首次创建，需要重建
            m_effected_cached_converter =
                std::make_unique<EffectedTimeConverter>(timings, status,
                                                        prebpm);
            // 重建线性映射转换器
            m_linear_cached_converter =
                std::make_unique<LinearTimeConverter>(info);

            current_status_version = &status;
            m_cached_version = timings.getVersion();
            // std::cout << "TimePixelConverter Rebuilt! Version: " <<
            // m_cached_version << std::endl;
        }

        return status.timeline_mapping_type == TimeLineMappingType::LINEAR
                   ? *m_linear_cached_converter
                   : *m_effected_cached_converter;
    }

    // debug

    const TimePixelConverter& getConverterEffected(
        const TimingMap& timings, const BaseCanvasStatus& status,
        const MapCanvasInfo* info, double prebpm) {
        // 检查TimingMap的版本号是否已更新
        if (!m_effected_cached_converter || !m_linear_cached_converter ||
            m_cached_version != timings.getVersion() ||
            !current_status_version || status != *current_status_version) {
            std::lock_guard<std::mutex> lock(build_newconverter_mtx);

            // 版本不匹配或首次创建，需要重建
            m_effected_cached_converter =
                std::make_unique<EffectedTimeConverter>(timings, status,
                                                        prebpm);
            // 重建线性映射转换器
            m_linear_cached_converter =
                std::make_unique<LinearTimeConverter>(info);

            current_status_version = &status;
            m_cached_version = timings.getVersion();
            // std::cout << "TimePixelConverter Rebuilt! Version: " <<
            // m_cached_version << std::endl;
        }

        return *m_effected_cached_converter;
    }
    const TimePixelConverter& getConverterLinear(const TimingMap& timings,
                                                 const BaseCanvasStatus& status,
                                                 const MapCanvasInfo* info,
                                                 double prebpm) {
        // 检查TimingMap的版本号是否已更新
        if (!m_effected_cached_converter || !m_linear_cached_converter ||
            m_cached_version != timings.getVersion() ||
            !current_status_version || status != *current_status_version) {
            std::lock_guard<std::mutex> lock(build_newconverter_mtx);

            // 版本不匹配或首次创建，需要重建
            m_effected_cached_converter =
                std::make_unique<EffectedTimeConverter>(timings, status,
                                                        prebpm);
            // 重建线性映射转换器
            m_linear_cached_converter =
                std::make_unique<LinearTimeConverter>(info);

            current_status_version = &status;
            m_cached_version = timings.getVersion();
            // std::cout << "TimePixelConverter Rebuilt! Version: " <<
            // m_cached_version << std::endl;
        }

        return *m_linear_cached_converter;
    }

   private:
    const BaseCanvasStatus* current_status_version{nullptr};
    std::mutex build_newconverter_mtx;
    std::unique_ptr<TimePixelConverter> m_effected_cached_converter{nullptr};
    std::unique_ptr<TimePixelConverter> m_linear_cached_converter{nullptr};
    uint64_t m_cached_version{0};
};

class MapLayerManager : public LayerManager {
   public:
    // 构造MapLayerManager
    using LayerManager::LayerManager;

    // 析构MapLayerManager
    ~MapLayerManager() override;

    // 更新map
    void updateMap(MMap* map) override;

    // 初始化图层
    void initializeLayers() override;

    // 获取map引用
    MMap* map() const { return mapref; }

    // 获取ecs核心
    ECSCore& core() { return map_ecs_core; }

    // 获取时间转换管理器
    TimePixelConverterManager* get_time_converter_manager() {
        return &time_converter_manager;
    }

    // 获取工具系统
    ToolSystem* get_tool_system() { return &toolSystem; }

    // 获取工具命令队列
    ThreadSafeQueue<ToolCommand>* get_tool_cmdq() { return &toolCommandQueue; }

    ThreadSafeQueue<MMapEditEvent>* get_edit_eventq() {
        return &editEventQueue;
    }

    // 获取工具交互管理
    ToolInteractionState* get_tool_interaction_state() {
        return &toolInteractionState;
    }

   private:
    // 核心ecs
    ECSCore map_ecs_core;

    // 时间转换管理器
    TimePixelConverterManager time_converter_manager;

    // map引用
    MMap* mapref{nullptr};

    // 工具同步系统(ecs)
    ToolSystem toolSystem{map_ecs_core.ecs_registry()};

    // 工具命令队列
    ThreadSafeQueue<ToolCommand> toolCommandQueue;
    // 编辑事件队列
    ThreadSafeQueue<MMapEditEvent> editEventQueue;

    // 工具交互状态
    ToolInteractionState toolInteractionState;
};

#endif  // MMM_MAPLAYERMANAGER_HPP
