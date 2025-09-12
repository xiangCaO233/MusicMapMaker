#ifndef MMM_MAPLAYERMANAGER_HPP
#define MMM_MAPLAYERMANAGER_HPP

#include <layer/LayerManager.hpp>
#include <layer/note/NoteLayerGenerator.hpp>
#include <tool/ToolCommandQueue.hpp>
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
                                           double prebpm) {
        // 检查TimingMap的版本号是否已更新
        if (!m_cached_converter || m_cached_version != timings.getVersion() ||
            !current_status_version || status != *current_status_version) {
            // 版本不匹配或首次创建，需要重建
            m_cached_converter =
                std::make_unique<TimePixelConverter>(timings, status, prebpm);
            current_status_version = &status;
            m_cached_version = timings.getVersion();
            // std::cout << "TimePixelConverter Rebuilt! Version: " <<
            // m_cached_version << std::endl;
        }
        return *m_cached_converter;
    }

   private:
    const BaseCanvasStatus* current_status_version{nullptr};
    std::unique_ptr<TimePixelConverter> m_cached_converter{nullptr};
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
    const ToolSystem* const get_tool_system() const {
        auto notelayer = layer_generators().find(LayerType::NOTE);
        if (notelayer != layer_generators().end()) {
            return static_cast<NoteLayerGenerator*>(notelayer->second.get())
                ->get_tool_system();
        } else {
            return nullptr;
        }
    }

   private:
    // 核心ecs
    ECSCore map_ecs_core;

    // 时间转换管理器
    TimePixelConverterManager time_converter_manager;

    // map引用
    MMap* mapref{nullptr};

    // 工具命令队列
    ToolCommandQueue toolCommandQueue;

    // 工具交互状态
    ToolInteractionState toolInteractionState;
};

#endif  // MMM_MAPLAYERMANAGER_HPP
