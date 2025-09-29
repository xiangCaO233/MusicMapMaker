#ifndef MMM_COMPONENTINSPECTOR_HPP
#define MMM_COMPONENTINSPECTOR_HPP

#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/EffectComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TimeLineComponents.hpp>
#include <ecs/component/TimingComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <entt.hpp>

class ComponentInspector {
   private:
    // 辅助函数，将 meta_any 包装的值转换为字符串
    static std::string valueToString(entt::meta_any& value);

   public:
    /**
     * @brief 注册所有需要被检视器识别的组件及其成员。
     * 这个函数应该在程序启动时调用一次。
     */
    static void registerAllComponentsForInspector();

    // --- 默认的备用函数 (如果一个组件没有特定的打印函数) ---
    template <typename ComponentType>
    static inline void print_component_details(std::stringstream& ss,
                                               const ComponentType& comp) {
        ss << "      (No specific printer defined)\n";
    }

    // --- 为具体组件编写的重载 ---

    static inline void print_component_details(std::stringstream& ss,
                                               const TimeComponent& comp) {
        ss << "      - timestamp: " << comp.timestamp << " ms\n";
    }

    static inline void print_component_details(std::stringstream& ss,
                                               const NoteComponent& comp) {
        ss << "      - track_index: " << comp.track_index << "\n";
        ss << "      - sourceUUID: " << to_string(comp.sourceUUID) << "\n";
    }

    static inline void print_component_details(std::stringstream& ss,
                                               const HoldComponent& comp) {
        ss << "      - duration: " << comp.duration << " ms\n";
    }

    static inline void print_component_details(std::stringstream& ss,
                                               const TransformComponent& comp) {
        ss << "      - y: " << comp.main_y << "\n";
    }

    // --- 为 Tag (标签) 组件编写的重载 ---

    static inline void print_component_details(
        std::stringstream& ss, const DirtyNoteMarkComponent& comp) {
        ss << "      (Tag Component)\n";
    }

    static inline void print_component_details(
        std::stringstream& ss, const CreatingNoteComponent& comp) {
        ss << "      (Tag Component)\n";
    }

    /**
     * @brief 检视指定实体，返回其所有组件和值的格式化字符串。
     * @param registry entt::registry 的引用。
     * @param entity 要检视的实体。
     * @return 包含所有组件信息的字符串。
     */
    template <typename... ComponentTypes>
    static inline std::string inspect(entt::registry& registry,
                                      entt::entity entity) {
        if (!registry.valid(entity)) return "Invalid Entity";

        std::stringstream ss;
        ss << "--- Inspecting Entity " << static_cast<uint32_t>(entity)
           << " (Compile-time) ---\n";

        // 创建一个 Lambda 函数，用于处理【单一】组件类型
        auto process_component = [&](auto type_wrapper) {
            // 从 entt::type_identity 中解包出真实的组件类型
            using ComponentType = typename decltype(type_wrapper)::type;

            if (const auto* comp = registry.try_get<ComponentType>(entity)) {
                ss << "  - [" << typeid(ComponentType).name() << "]\n";
                print_component_details(ss, *comp);
            }
        };

        // 使用 C++17 折叠表达式和逗号操作符，对参数包中的每一种类型调用一次
        // Lambda entt::type_identity<T>{} 是一个技巧，用于将类型作为值传递给
        // Lambda
        (process_component(entt::type_identity<ComponentTypes>{}), ...);

        ss << "--- End of Inspection ---\n";
        return ss.str();
    }
};

#endif  // MMM_COMPONENTINSPECTOR_HPP
