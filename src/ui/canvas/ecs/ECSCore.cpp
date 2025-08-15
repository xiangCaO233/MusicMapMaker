#include <ecs/ECSCore.hpp>
#include <ecs/NoteComponents.hpp>
#include <ecs/RelationComponents.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/Hold.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>

// 构造ECSCore
ECSCore::ECSCore() = default;

// 析构ECSCore
ECSCore::~ECSCore() = default;

// 获取ecs reg
entt::registry& ECSCore::ecs_registry() { return registry; }

// 更新map
void ECSCore::updateMap(MMap* mmap) {
    map = mmap;
    registry.clear();
    // 更新registry
    for (const auto& note : mmap->note_set().get_all_notes_unordered()) {
        auto noteptr = &note;
        createEntity(noteptr);
    }
}

// 创建实体
entt::entity ECSCore::createEntity(const Note* note) {
    auto note_entity = registry.create();
    // 附加note组件(time,track,source)
    registry.emplace<NoteComponent>(note_entity, note->timestamp(),
                                    note->trackpos(), note);
    switch (note->notetype()) {
        case NoteType::NORMAL: {
            break;
        }
        case NoteType::HOLD: {
            auto hold_note = static_cast<const Hold*>(note);
            registry.emplace<HoldComponent>(note_entity, hold_note->duration());
            break;
        }
        case NoteType::SLIDE: {
            auto slide_note = static_cast<const Slide*>(note);
            registry.emplace<FlickComponent>(note_entity,
                                             slide_note->delta_track());
        }
        case NoteType::COMPOSITE: {
            auto composed_note = static_cast<const Composite*>(note);
            std::vector<entt::entity> children;
            for (const auto& child_note : composed_note->children()) {
                auto child_note_entity = createEntity(child_note.get());

                // 附加父实体组件
                registry.emplace<ChildOfComponent>(child_note_entity,
                                                   note_entity);
                // 添加实体到父实体的复合组件的子实体列表
                children.push_back(child_note_entity);
            }
            // 父实体附加子实体列表组件
            registry.emplace<CompositeRootComponent>(note_entity, children);
        }
    }
    return note_entity;
}
