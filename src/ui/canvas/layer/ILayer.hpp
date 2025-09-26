#ifndef MMM_ILAYER_HPP
#define MMM_ILAYER_HPP

#include <array>
#include <render/Renderer2D.hpp>
#include <render/command/RenderCommand.hpp>
#include <string_view>
#include <vector>

enum class LayerType : uint32_t {
    // 背景图层
    BACKGROUND = 0,
    // 时间线图层
    TIMELINE = 1,
    // 物件图层
    NOTE = 2,
    // 预览图层
    PREVIEW = 3,
    // 特效图层
    EFFECT = 4,
    // 交互图层
    INTERACT = 5,
};

class SharedCanvasInfo;
class Renderer2D;
class ECSCore;

struct RenderDataBuffer {
    // 统一的、保证顺序的索引表
    std::vector<CommandHandle> all_command_handles;

    // 分离存储不同渲染指令
    std::vector<QuadCommand> quad_command_list;
    std::vector<MeshCommand> mesh_command_list;
    std::vector<PrimitiveCommand> primitive_command_list;
    std::vector<CurveCommand> curve_command_list;

    void add_QuadCommand(const QuadCommand& cmd) {
        quad_command_list.push_back(cmd);
        all_command_handles.push_back(
            {CommandType::QUAD, quad_command_list.size() - 1});
    }

    void add_MeshCommand(const MeshCommand& cmd) {
        mesh_command_list.push_back(cmd);
        all_command_handles.push_back(
            {CommandType::MESH, mesh_command_list.size() - 1});
    }

    void add_PrimitiveCommand(const PrimitiveCommand& cmd) {
        primitive_command_list.push_back(cmd);
        all_command_handles.push_back(
            {CommandType::PRIMITIVE, primitive_command_list.size() - 1});
    }
    void add_PrimitiveCommand(std::vector<PrimitiveCommand>&& cmds) {
        for (const auto& cmd : cmds) {
            primitive_command_list.push_back(cmd);
            all_command_handles.push_back(
                {CommandType::PRIMITIVE, primitive_command_list.size() - 1});
        }
    }

    void add_CurveCommand(const CurveCommand& cmd) {
        curve_command_list.push_back(cmd);
        all_command_handles.push_back(
            {CommandType::CURVE, curve_command_list.size() - 1});
    }

    void clear() {
        quad_command_list.clear();
        mesh_command_list.clear();
        primitive_command_list.clear();
        curve_command_list.clear();
        all_command_handles.clear();
    }
};

// 图层
class ILayer {
   public:
    // 构造ILayer
    ILayer(Renderer2D* renderer, ECSCore* ecore)
        : rendererRef(renderer), core(ecore) {}

    // 析构ILayer
    virtual ~ILayer() = default;

    // 获取图层类型
    LayerType type() const { return layer_type; }

    // 获取信息
    SharedCanvasInfo* info() { return info_ref; }

    // 交换前后缓冲区
    void swapBuffers() {
        // 原子翻转索引 (0 -> 1 , 1 -> 0)
        int currentIndex = frontBufferIndex.load(std::memory_order_relaxed);
        int nextIndex = 1 - currentIndex;
        // 使用 acquire-release 内存顺序确保内存可见性
        frontBufferIndex.store(nextIndex, std::memory_order_release);
    }

    // 访问后端缓冲区
    RenderDataBuffer& backbuffer() {
        int backIndex = 1 - frontBufferIndex.load(std::memory_order_relaxed);
        return buffers[backIndex];
    }

    // 访问前端缓冲区
    const RenderDataBuffer& frontbuffer() const {
        int frontIndex = frontBufferIndex.load(std::memory_order_relaxed);
        return buffers[frontIndex];
    }

    // 获取纹理信息
    std::optional<TextureInfo> get(std::string_view path) const {
        return rendererRef->texture_pool()->get(path);
    }

    // 获取字符串绘制后的总尺寸
    glm::vec2 stringMetrics(std::string_view family, size_t font_size,
                            const std::u32string& str) {
        glm::vec2 size{0};
        auto max_bearingy{0};
        for (const auto& character : str) {
            auto fontoption = get(family, font_size, character);
            if (fontoption.has_value()) {
                auto& charInfo = fontoption.value();
                auto& charTexture = charInfo.character_texinfo;
                size.x += charInfo.xadvance / 64.f;
                if (size.y < charInfo.height) {
                    size.y = charInfo.height;
                }
                if (max_bearingy < charInfo.bearing.y) {
                    max_bearingy = charInfo.bearing.y;
                }
            }
        }
        size.y += max_bearingy;
        return size;
    }

    // 生成字符串绘制指令
    std::vector<PrimitiveCommand> generateStringCommands(
        std::string_view family, size_t font_size, const std::u32string& str,
        glm::vec2 strpos, glm::vec4 color) {
        std::vector<PrimitiveCommand> commands;
        uint32_t xoffset{0};
        uint32_t yoffset{0};

        for (const auto& character : str) {
            // 获取字符纹理信息
            auto fontoption = get(family, font_size, character);
            if (fontoption.has_value()) {
                auto& charInfo = fontoption.value();
                auto& charTexture = charInfo.character_texinfo;
                // 计算当前字符应该处于的位置
                // strpos.x += xoffset;
                // strpos.y += 8;
                // 提交渲染指令
                PrimitiveCommand charcommand{
                    {CommandType::PRIMITIVE,
                     {charTexture, TexAlignMode::CENTER,
                      TexScaleMode::CHARACTER}},
                    glm::vec2{strpos.x + xoffset,
                              strpos.y - charInfo.bearing.y + 8},
                    charTexture.origin_size,
                    0.f,
                    color,
                    true,
                    {charTexture.uv_offset},
                    PrimitiveType::QUAD};
                commands.push_back(charcommand);

                xoffset += charInfo.xadvance / 64;
            }
        }
        return commands;
    }

    // 获取字符纹理信息
    std::optional<CharacterGlyph> get(std::string_view family, size_t font_size,
                                      char32_t character) const {
        return rendererRef->font_pool()->get(family, font_size, character);
    }

   protected:
    // 更新信息
    void updateInfo(SharedCanvasInfo* info) { info_ref = info; }
    void setType(LayerType t) { layer_type = t; }

    // 获取纹理信息
    TextureInfo textureInfo(std::string_view texpath) const {
        auto texinfoOption = rendererRef->texture_pool()->get(texpath);
        return texinfoOption.value_or(TextureInfo{});
    }

   private:
    // 图层类型
    LayerType layer_type;

    // 画布信息引用
    SharedCanvasInfo* info_ref;

    // 渲染器引用
    Renderer2D* rendererRef;

    // ecs核心引用
    ECSCore* core;

    // 双缓冲
    // 两个物理缓冲区
    std::array<RenderDataBuffer, 2> buffers;

    // 使用原子整数作为索引
    std::atomic<int> frontBufferIndex;

    friend class LayerManager;
};

#endif  // MMM_ILAYER_HPP
