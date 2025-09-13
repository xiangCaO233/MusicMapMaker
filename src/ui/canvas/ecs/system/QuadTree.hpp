#ifndef MMM_QUADTREE_HPP
#define MMM_QUADTREE_HPP

#include <algorithm>
#include <cassert>
#include <glm/glm.hpp>
#include <info/NotePart.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

struct BoundingBox {
    glm::vec2 pos;
    glm::vec2 size;

    BoundingBox(const glm::vec2& p = {0, 0}, const glm::vec2& s = {0, 0})
        : pos(p), size(s) {}

    bool contains(glm::vec2 p) const {
        return p.x >= pos.x && p.x <= pos.x + size.x && p.y >= pos.y &&
               p.y <= pos.y + size.y;
    }
};

template <typename T>
class LooseQuadtree {
   public:
    static constexpr float LoosenessFactor = 1.0f;

   private:
    struct Node {
        BoundingBox bounds;
        BoundingBox looseBounds;
        std::vector<T> objects;  // 变更: 直接存储对象，而不是指针。
        std::unique_ptr<Node> children[4];
        Node* parent;
        int depth;

        Node(const BoundingBox& b, int d, Node* p)
            : bounds(b), parent(p), depth(d) {
            float extraW = bounds.size.x * (LoosenessFactor - 1.0f) / 2.0f;
            float extraH = bounds.size.y * (LoosenessFactor - 1.0f) / 2.0f;
            looseBounds = {{bounds.pos.x - extraW, bounds.pos.y - extraH},
                           {bounds.size.x * LoosenessFactor,
                            bounds.size.y * LoosenessFactor}};
        }
        bool isLeaf() const { return children[0] == nullptr; }
    };

    std::unique_ptr<Node> root;
    const size_t capacity;
    const int maxDepth;

   public:
    LooseQuadtree(const BoundingBox& bounds, size_t cap = 8, int max_depth = 8)
        : capacity(cap), maxDepth(max_depth) {
        root = std::make_unique<Node>(bounds, 0, nullptr);
    }

    // 移动构造函数等保持不变...
    LooseQuadtree(LooseQuadtree&& other) noexcept
        : root(std::move(other.root)),
          capacity(other.capacity),
          maxDepth(other.maxDepth) {}

    LooseQuadtree& operator=(LooseQuadtree&& other) noexcept {
        if (this != &other) {
            root = std::move(other.root);
        }
        return *this;
    }

    LooseQuadtree(const LooseQuadtree&) = delete;
    LooseQuadtree& operator=(const LooseQuadtree&) = delete;
    ~LooseQuadtree() = default;

    void clear() {
        if (root) {
            // 现在这将正确地为所有存储的 T 对象调用析构函数。
            root->objects.clear();
            for (int i = 0; i < 4; ++i) {
                root->children[i].reset();
            }
        }
    }

    // 变更: 新的插入 API。
    // 为左值提供的重载 (拷贝对象)。
    void insert(const T& object) {
        Node* node = find_best_fit_node(root.get(), get_object_bounds(object));
        insert_to_node(node, T(object));  // 创建一个副本并移动它
    }

    // 为右值提供的重载 (移动对象)。
    void insert(T&& object) {
        // 在对象被移动前，使用其数据获取边界
        Node* node = find_best_fit_node(root.get(), get_object_bounds(object));
        insert_to_node(node, std::move(object));
    }

    // 变更: 新的移除 API。
    // 按值移除对象。要求 T 类型实现了 operator==。
    bool remove(const T& object) {
        Node* node = find_best_fit_node(root.get(), get_object_bounds(object));

        auto& obj_list = node->objects;
        // std::find 现在使用 T::operator== 来比较对象
        auto it = std::find(obj_list.begin(), obj_list.end(), object);

        if (it != obj_list.end()) {
            obj_list.erase(it);
            try_merge(node);
            return true;
        }
        return false;
    }

    // 变更: Query 现在返回指向树内部拥有对象的指针。
    // 警告: 这些指针仅在下一次对四叉树进行非 const 操作
    // (insert, remove, clear 等) 之前有效。
    std::vector<T> query(const glm::vec2& point) const {
        std::vector<T> result;
        query_recursive(root.get(), point, result);
        return result;
    }

    void print_tree(std::ostream& out = std::cout) const {
        out << "\n--- Quadtree Structure (Detailed) ---\n";
        if (!root) {
            out << "Tree is empty.\n";
            return;
        }
        out << std::fixed << std::setprecision(1);
        print_node_recursive(out, root.get(), "", true, "Root");
        out << "-------------------------------------\n" << std::endl;
    }
    // 新增: 提取所有对象。这是一个破坏性操作，会清空树。
    std::vector<T> extract_all_objects() {
        std::vector<T> all_objects;
        extract_recursive(root.get(), all_objects);
        clear();  // 清空树结构
        return all_objects;
    }

   private:
    // 变更: 接受 const 引用而不是指针。
    BoundingBox get_object_bounds(const T& object) const {
        return BoundingBox(object.pos, object.size);
    }
    // 新增: extract_all_objects 的递归辅助函数
    void extract_recursive(Node* node, std::vector<T>& out_objects) {
        if (!node) return;

        // 移动节点中的对象到输出向量
        out_objects.insert(out_objects.end(),
                           std::make_move_iterator(node->objects.begin()),
                           std::make_move_iterator(node->objects.end()));
        node->objects.clear();

        // 递归处理子节点
        if (!node->isLeaf()) {
            for (int i = 0; i < 4; ++i) {
                extract_recursive(node->children[i].get(), out_objects);
            }
        }
    }

    // 用于打印的私有递归辅助函数
    void print_node_recursive(std::ostream& out, const Node* node,
                              const std::string& prefix, bool isLast,
                              const std::string& quadrantLabel) const {
        if (!node) return;
        out << prefix << (isLast ? "└── " : "├── ");
        out << quadrantLabel << " Node [Depth: " << node->depth << ", ";
        out << "Bounds: (" << node->bounds.pos.x << "," << node->bounds.pos.y
            << ")-(" << node->bounds.size.x << "x" << node->bounds.size.y
            << "), ";
        out << "Objects: " << node->objects.size() << "]\n";

        std::string objPrefix = prefix + (isLast ? "    " : "│   ");
        for (const auto& obj : node->objects) {
            out << objPrefix;
            out << "  -> Part [Entity: " << std::setw(3)
                << static_cast<uint32_t>(obj.source_entity);
            out << ", Type: " << std::setw(10) << to_string(obj.part);
            out << ", Pos: (" << std::setw(6) << obj.pos.x << ","
                << std::setw(6) << obj.pos.y << ")";
            out << ", Size: (" << std::setw(5) << obj.size.x << "x"
                << std::setw(5) << obj.size.y << ")";
            out << ", z: " << obj.zIndex << "]\n";
        }

        if (!node->isLeaf()) {
            std::string childPrefix = prefix + (isLast ? "    " : "│   ");
            // 分裂逻辑：0:左上, 1:右上, 2:左下, 3:右下
            const char* labels[] = {"左上", "右上", "左下", "右下"};
            for (int i = 0; i < 4; ++i) {
                if (node->children[i]) {
                    print_node_recursive(out, node->children[i].get(),
                                         childPrefix, i == 3, labels[i]);
                } else {
                    out << childPrefix << (i == 3 ? "└── " : "├── ")
                        << labels[i] << " (empty)\n";
                }
            }
        }
    }

    Node* find_best_fit_node(Node* startNode, const BoundingBox& bounds) {
        // 此函数逻辑不变
        Node* currentNode = startNode;
        while (!currentNode->isLeaf()) {
            int quadrant = get_quadrant(currentNode->bounds, bounds);
            if (quadrant == -1) {
                break;
            }
            if (!currentNode->children[quadrant]) break;  // 安全检查
            currentNode = currentNode->children[quadrant].get();
        }
        return currentNode;
    }

    // 变更: 接受右值引用，以便高效地将对象移动到 vector 中。
    void insert_to_node(Node* node, T&& object) {
        node->objects.push_back(std::move(object));
        if (node->isLeaf() && node->objects.size() > capacity &&
            node->depth < maxDepth) {
            subdivide(node);
        }
    }

    void subdivide(Node* node) {
        const auto& p = node->bounds.pos;
        const float hw = node->bounds.size.x * 0.5f;
        const float hh = node->bounds.size.y * 0.5f;
        const int nextDepth = node->depth + 1;

        node->children[0] = std::make_unique<Node>(
            BoundingBox{{p.x, p.y}, {hw, hh}}, nextDepth, node);
        node->children[1] = std::make_unique<Node>(
            BoundingBox{{p.x + hw, p.y}, {hw, hh}}, nextDepth, node);
        node->children[2] = std::make_unique<Node>(
            BoundingBox{{p.x, p.y + hh}, {hw, hh}}, nextDepth, node);
        node->children[3] = std::make_unique<Node>(
            BoundingBox{{p.x + hw, p.y + hh}, {hw, hh}}, nextDepth, node);

        // 变更: 重新分配对象，而不是指针。
        std::vector<T> old_objects = std::move(node->objects);
        node->objects.clear();

        for (auto& obj : old_objects) {  // 通过引用遍历以允许移动
            int quadrant = get_quadrant(node->bounds, get_object_bounds(obj));
            if (quadrant != -1) {
                // 将对象移动到子节点的 vector 中
                insert_to_node(node->children[quadrant].get(), std::move(obj));
            } else {
                // 将对象移回父节点的 vector 中
                node->objects.push_back(std::move(obj));
            }
        }
    }

    void try_merge(Node* node) {
        if (!node || node == root.get()) return;

        Node* parent = node->parent;
        if (!parent || parent->isLeaf()) return;

        size_t total_objects = parent->objects.size();  // 父节点中跨界的对象
        for (int i = 0; i < 4; ++i) {
            if (!parent->children[i] || !parent->children[i]->isLeaf()) {
                return;  // 只有当所有兄弟节点都是叶子时才合并
            }
            total_objects += parent->children[i]->objects.size();
        }

        if (total_objects <= capacity) {
            // 变更: 合并对象，而不是指针。
            std::vector<T> merged_objects = std::move(parent->objects);
            for (int i = 0; i < 4; ++i) {
                auto& child = parent->children[i];
                // 使用 std::move_iterator 高效地转移对象
                merged_objects.insert(
                    merged_objects.end(),
                    std::make_move_iterator(child->objects.begin()),
                    std::make_move_iterator(child->objects.end()));
                child.reset();
            }
            parent->objects =
                std::move(merged_objects);  // 移动最终合并的 vector

            try_merge(parent);
        }
    }

    void query_recursive(Node* node, const glm::vec2& point,
                         std::vector<T>& result) const {
        if (!node || !node->bounds.contains(point)) {  // 添加了空指针检查
            return;
        }

        // 变更: 通过引用遍历，然后存入指向内部对象的指针。
        for (const T& obj : node->objects) {
            if (get_object_bounds(obj).contains(point)) {
                result.push_back(obj);
            }
        }

        if (!node->isLeaf()) {
            int quadrant = get_quadrant_for_point(node->bounds, point);
            if (quadrant != -1 &&
                node->children[quadrant]) {  // 添加了空指针检查
                query_recursive(node->children[quadrant].get(), point, result);
            }
        }
    }

    int get_quadrant(const BoundingBox& parentBounds,
                     const BoundingBox& objBounds) const {
        // 此函数逻辑不变
        const glm::vec2 center = parentBounds.pos + parentBounds.size * 0.5f;
        bool fitsTop = objBounds.pos.y + objBounds.size.y < center.y;
        bool fitsBottom = objBounds.pos.y > center.y;
        bool fitsLeft = objBounds.pos.x + objBounds.size.x < center.x;
        bool fitsRight = objBounds.pos.x > center.x;

        if (fitsLeft) {
            if (fitsTop) return 0;
            if (fitsBottom) return 2;
        } else if (fitsRight) {
            if (fitsTop) return 1;
            if (fitsBottom) return 3;
        }
        return -1;
    }

    int get_quadrant_for_point(const BoundingBox& parentBounds,
                               const glm::vec2& p) const {
        // 此函数逻辑不变
        const auto center =
            glm::vec2{parentBounds.pos.x + parentBounds.size.x * 0.5f,
                      parentBounds.pos.y + parentBounds.size.y * 0.5f};
        if (p.x < center.x) {
            if (p.y < center.y) return 0;
            return 2;
        } else {
            if (p.y < center.y) return 1;
            return 3;
        }
    }
};

#endif  // MMM_QUADTREE_HPP
