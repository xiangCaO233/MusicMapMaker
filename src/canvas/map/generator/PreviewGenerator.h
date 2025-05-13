#ifndef M_PREVIEWGENERATOR_H
#define M_PREVIEWGENERATOR_H

#include <memory>
#include <unordered_map>

class MapEditor;
enum class NoteType;
class ObjectGenerator;

class PreviewGenerator {
   public:
    // 构造PreviewGenerator
    PreviewGenerator(std::shared_ptr<MapEditor>& editor);
    // 析构PreviewGenerator
    virtual ~PreviewGenerator();

    // 物件生成器
    std::unordered_map<NoteType, std::shared_ptr<ObjectGenerator>>
        objgenerators;

    // 编辑器引用
    std::shared_ptr<MapEditor> editor_ref;

    // 生成预览
    void generate();
};

#endif  // M_PREVIEWGENERATOR_H
