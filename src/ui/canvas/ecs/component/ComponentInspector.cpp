#include <ecs/component/ComponentInspector.hpp>

template <typename ComponentType>
class ComponentRegistrar {
   public:
    // 构造函数：开始注册并设置类型名称
    ComponentRegistrar(entt::id_type type_name_hash) {
        factory.type(type_name_hash);
    }

    // 析构函数：在对象生命周期结束时，自动添加 get_instance 辅助函数
    ~ComponentRegistrar() {
        using namespace entt::literals;
        factory.func(
            "get_instance"_hs,
            +[](const entt::registry& reg,
                entt::entity entt) -> entt::meta_any {
                // 这里使用了模板参数 ComponentType，实现了真正的泛型！
                if (const auto* comp = reg.try_get<ComponentType>(entt)) {
                    return entt::meta_any{std::cref(*comp)};
                }
                return {};
            });
    }

    // 包装了原始的 .data() 方法，以支持链式调用
    template <auto Data, typename... Opts>
    auto& data(entt::id_type name_hash, Opts... opts) {
        factory.template data<Data>(name_hash, std::forward<Opts>(opts)...);
        return *this;
    }

   private:
    entt::meta_factory<ComponentType> factory{};
};

std::string ComponentInspector::valueToString(entt::meta_any& value) {
    if (!value) {
        return "[null]";
    }

    auto type = value.type().info();

    if (type == entt::type_id<bool>()) {
        return value.cast<bool>() ? "true" : "false";
    }
    if (type == entt::type_id<int>() || type == entt::type_id<int32_t>() ||
        type == entt::type_id<int64_t>()) {
        return std::to_string(value.cast<int64_t>());
    }
    if (type == entt::type_id<uint32_t>() || type == entt::type_id<size_t>()) {
        return std::to_string(value.cast<uint64_t>());
    }
    if (type == entt::type_id<float>()) {
        return std::to_string(value.cast<float>());
    }
    if (type == entt::type_id<double>()) {
        return std::to_string(value.cast<double>());
    }
    if (type == entt::type_id<std::string>()) {
        return "\"" + value.cast<std::string>() + "\"";
    }
    if (type == entt::type_id<entt::entity>()) {
        return std::to_string(
                   static_cast<uint32_t>(value.cast<entt::entity>())) +
               "(e)";
    }
    if (type == entt::type_id<glm::vec4>()) {
        return to_string(value.cast<glm::vec4>());
    }
    if (type == entt::type_id<NoteUUID>()) {
        return to_string(value.cast<NoteUUID>());
    }
    if (type == entt::type_id<EffectTextureType>()) {
        return to_string(value.cast<EffectTextureType>());
    }

    // --- 容器处理 ---
    if (type == entt::type_id<std::vector<entt::entity>>()) {
        std::stringstream ss;
        ss << "[";
        auto& vec = value.cast<const std::vector<entt::entity>&>();
        for (size_t i = 0; i < vec.size(); ++i) {
            ss << static_cast<uint32_t>(vec[i]) << "(e)";
            if (i < vec.size() - 1) ss << ", ";
        }
        ss << "]";
        return ss.str();
    }
    if (type == entt::type_id<std::map<SoundEffectType, uint32_t>>()) {
        std::stringstream ss;
        ss << "{";
        auto& map = value.cast<const std::map<SoundEffectType, uint32_t>&>();
        for (auto it = map.begin(); it != map.end(); ++it) {
            ss << to_string(it->first) << ": " << it->second;
            if (std::next(it) != map.end()) ss << ", ";
        }
        ss << "}";
        return ss.str();
    }

    return "[unhandled type]";
}

void ComponentInspector::registerAllComponentsForInspector() {
    // --- 使用 entt::meta_factory<T>() 进行注册 ---
    using namespace entt::literals;
    // ComponentRegistrar<TimeComponent>("TimeComponent"_hs)
    //     .data<&TimeComponent::timestamp>("timestamp"_hs);
    // ComponentRegistrar<NoteComponent>("NoteComponent"_hs)
    //     .data<&NoteComponent::track_index>("track_index"_hs)
    //     .data<&NoteComponent::sourceUUID>("sourceUUID"_hs);

    // ComponentRegistrar<TransformComponent>("TransformComponent"_hs)
    //     .data<&TransformComponent::y>("y"_hs);

    // ComponentRegistrar<HoldComponent>("HoldComponent"_hs)
    //     .data<&HoldComponent::duration>("duration"_hs);

    // entt::meta_factory<TimeComponent>()
    //     .type("TimeComponent"_hs)
    //     .data<&TimeComponent::timestamp>("timestamp"_hs)
    //     .func<>(
    //         "get_instance"_hs,
    //         +[](const entt::registry& reg,
    //             entt::entity entt) -> entt::meta_any {
    //             // 使用模板版本的 try_get，这是类型安全的
    //             if (const auto* comp = reg.try_get<TimeComponent>(entt)) {
    //                 // 使用 std::cref 进行安全的 const 引用包装，避免拷贝
    //                 return entt::meta_any{std::cref(*comp)};
    //             }
    //             return {};
    //         });
    entt::meta_factory<VisualsComponent>()
        .type("VisualsComponent"_hs)
        .data<&VisualsComponent::color>("color"_hs)
        .data<&VisualsComponent::texture_path>("texture_path"_hs);
    entt::meta_factory<GhostComponent>()
        .type("GhostComponent"_hs)
        .data<&GhostComponent::confirm>("confirm"_hs);
    entt::meta_factory<DeleteMarkComponent>()
        .type("DeleteMarkComponent"_hs)
        .data<&DeleteMarkComponent::confirm>("confirm"_hs);
    entt::meta_factory<DirtyMarkComponent>().type("DirtyMarkComponent"_hs);
    entt::meta_factory<EffectComponent>()
        .type("EffectComponent"_hs)
        .data<&EffectComponent::track>("track"_hs)
        .data<&EffectComponent::texture_type>("texture_type"_hs)
        .data<&EffectComponent::duration>("duration"_hs)
        .data<&EffectComponent::frame_index>("frame_index"_hs);
    entt::meta_factory<SoundStateComponent>()
        .type("SoundStateComponent"_hs)
        .data<&SoundStateComponent::pending_sounds>("pending_sounds"_hs);
    entt::meta_factory<TrackIdentifierComponent>()
        .type("TrackIdentifierComponent"_hs)
        .data<&TrackIdentifierComponent::track>("track"_hs);
    // entt::meta_factory<NoteComponent>()
    //     .type("NoteComponent"_hs)
    //     .data<&NoteComponent::track_index>("track_index"_hs)
    //     .data<&NoteComponent::sourceUUID>("sourceUUID"_hs);
    entt::meta_factory<CreatingNoteComponent>().type(
        "CreatingNoteComponent"_hs);
    // entt::meta_factory<HoldComponent>()
    //     .type("HoldComponent"_hs)
    //     .data<&HoldComponent::duration>("duration"_hs);
    entt::meta_factory<FlickComponent>()
        .type("FlickComponent"_hs)
        .data<&FlickComponent::delta_track>("delta_track"_hs);
    entt::meta_factory<CompositeRootComponent>()
        .type("CompositeRootComponent"_hs)
        .data<&CompositeRootComponent::children>("children"_hs);
    entt::meta_factory<ChildOfComponent>()
        .type("ChildOfComponent"_hs)
        .data<&ChildOfComponent::parent>("parent"_hs)
        .data<&ChildOfComponent::child_index>("child_index"_hs);
    entt::meta_factory<BeatComponent>()
        .type("BeatComponent"_hs)
        .data<&BeatComponent::divisors>("divisors"_hs)
        .data<&BeatComponent::beatLength>("beatLength"_hs)
        .data<&BeatComponent::beat_index>("beat_index"_hs);
    entt::meta_factory<TimingComponent>()
        .type("TimingComponent"_hs)
        .data<&TimingComponent::bpm>("bpm"_hs)
        .data<&TimingComponent::beat_length>("beat_length"_hs)
        .data<&TimingComponent::is_base_timing>("is_base_timing"_hs);
    // entt::meta_factory<TransformComponent>()
    //     .type("TransformComponent"_hs)
    //     .data<&TransformComponent::y>("y"_hs);
    // 1. 定义我们想要的 to_string 函数指针的确切类型 (不变)
    using ToStringFuncPtr = std::string (*)(const GeneratedMesh&);
    // 2. 使用正确的语法进行注册
    entt::meta_factory<GeneratedMesh>()
        .type("GeneratedMesh"_hs)
        // 将函数指针作为模板参数，名称作为函数参数
        .func<static_cast<ToStringFuncPtr>(&to_string)>("to_string"_hs);
    // entt::meta_factory<GeneratedMesh>()
    //     .type("GeneratedMesh"_hs)
    //     // 我们注册一个名为 "to_string_caller" 的“数据成员”。
    //     // 它不是一个真正的成员变量，而是一个 getter lambda。
    //     // 这个 getter 接受一个 GeneratedMesh 实例...
    //     .data<+[](const GeneratedMesh& mesh) {
    //         // ...然后返回一个 std::function 对象。
    //         return std::function<std::string()>{
    //             // 这个 std::function 对象包装了对我们实际 to_string
    //             // 函数的调用。
    //             [&mesh]() { return to_string(mesh); }};
    //     }>("to_string_caller"_hs);
    entt::meta_factory<EffectTextureType>()
        .type("EffectTextureType"_hs)
        .data<EffectTextureType::NONE>("NONE"_hs)
        .data<EffectTextureType::NORMAL>("TYPE_A"_hs)
        .data<EffectTextureType::SLIDE_END>("TYPE_B"_hs);
}
