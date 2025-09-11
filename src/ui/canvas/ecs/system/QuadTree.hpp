#ifndef MMM_QUADTREE_HPP
#define MMM_QUADTREE_HPP

#include <algorithm>
#include <cassert>
#include <glm/glm.hpp>
#include <info/NotePart.hpp>
#include <memory>
#include <vector>

struct BoundingBox {
    glm::vec2 pos;
    glm::vec2 size;

    BoundingBox(const glm::vec2& p = {0, 0}, const glm::vec2& s = {0, 0})
        : pos(p), size(s) {}

    bool contains(const glm::vec2& p) const {
        return p.x >= pos.x && p.x <= pos.x + size.x && p.y >= pos.y &&
               p.y <= pos.y + size.y;
    }
};

// --- 松散四叉树实现 ---
template <typename T>
class LooseQuadtree {
   public:
    // 松散因子k > 1.0。k=2.0意味着节点的有效边界是其物理边界的两倍大。
    static constexpr float LoosenessFactor = 2.0f;

   private:
    struct Node {
        BoundingBox bounds;       // 节点的物理边界
        BoundingBox looseBounds;  // 节点的松散边界
        std::vector<const T*> objects;
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

    ~LooseQuadtree() = default;

    void clear() {
        if (root) {
            root->objects.clear();
            for (int i = 0; i < 4; ++i) {
                root->children[i].reset();
            }
        }
    }

    void insert(const T* object) {
        Node* node = find_best_fit_node(root.get(), get_object_bounds(object));
        insert_to_node(node, object);
    }

    bool remove(const T* object) {
        Node* node = find_best_fit_node(root.get(), get_object_bounds(object));

        auto& obj_list = node->objects;
        auto it = std::find(obj_list.begin(), obj_list.end(), object);

        if (it != obj_list.end()) {
            obj_list.erase(it);
            try_merge(node);
            return true;
        }
        // 如果在最优节点没找到，可能意味着对象移动了但我们用了旧位置查找
        // 在一个完整的系统中，通常会有一个 map<T*, Node*> 来加速查找
        // 这里为了简化，我们只在最优节点查找。
        return false;
    }

    void update(const T* object) {
        // 在松散四叉树中，更新操作非常高效
        // 1. 找到对象当前所在的节点
        Node* current_node = find_node_for_object(root.get(), object);
        if (!current_node) return;  // 对象不在树中

        // 2. 检查对象是否仍在当前节点的松散边界内
        if (current_node->looseBounds.contains(
                {object->bounds.pos.x, object->bounds.pos.y})) {
            // 仍在边界内，什么都不用做！这就是松散四叉树的优势。
            return;
        }

        // 3. 如果移出了边界，则执行完整的 remove 和 insert
        remove(object);
        insert(object);
    }

    std::vector<const T*> query(const glm::vec2& point) const {
        std::vector<const T*> result;
        query_recursive(root.get(), point, result);
        return result;
    }

   private:
    // 获取对象包围盒的辅助函数，需要特化或修改以适应您的 T 类型
    BoundingBox get_object_bounds(const T* object) const {
        return BoundingBox({object->pos.x, object->pos.y}, object->size.x,
                           object->size.y);
    }

    Node* find_best_fit_node(Node* startNode, const BoundingBox& bounds) {
        Node* currentNode = startNode;
        while (!currentNode->isLeaf()) {
            int quadrant = get_quadrant(currentNode->bounds, bounds);
            if (quadrant == -1) {
                break;  // 跨越边界，停在当前节点
            }
            currentNode = currentNode->children[quadrant].get();
        }
        return currentNode;
    }

    // 这个函数用于已经插入的对象，它必须存在于某个节点的 looseBounds 内
    Node* find_node_for_object(Node* node, const T* object) const {
        auto& obj_list = node->objects;
        if (std::find(obj_list.begin(), obj_list.end(), object) !=
            obj_list.end()) {
            return node;
        }

        if (!node->isLeaf()) {
            int quadrant =
                get_quadrant(node->bounds, get_object_bounds(object));
            if (quadrant != -1) {
                return find_node_for_object(node->children[quadrant].get(),
                                            object);
            }
        }
        return nullptr;  // Should not happen if object is in tree
    }

    void insert_to_node(Node* node, const T* object) {
        node->objects.push_back(object);
        if (node->isLeaf() && node->objects.size() > capacity &&
            node->depth < maxDepth) {
            subdivide(node);
        }
    }

    void subdivide(Node* node) {
        const auto& p = node->bounds.pos;
        const float hw = node->bounds.width * 0.5f;
        const float hh = node->bounds.height * 0.5f;
        const int nextDepth = node->depth + 1;

        node->children[0] = std::make_unique<Node>(
            BoundingBox{{p.x, p.y}, hw, hh}, nextDepth, node);  // 左上
        node->children[1] = std::make_unique<Node>(
            BoundingBox{{p.x + hw, p.y}, hw, hh}, nextDepth, node);  // 右上
        node->children[2] = std::make_unique<Node>(
            BoundingBox{{p.x, p.y + hh}, hw, hh}, nextDepth, node);  // 左下
        node->children[3] =
            std::make_unique<Node>(BoundingBox{{p.x + hw, p.y + hh}, hw, hh},
                                   nextDepth, node);  // 右下

        // 重新分配对象
        std::vector<const T*> old_objects = std::move(node->objects);
        node->objects.clear();

        for (const T* obj : old_objects) {
            int quadrant = get_quadrant(node->bounds, get_object_bounds(obj));
            if (quadrant != -1) {
                insert_to_node(node->children[quadrant].get(), obj);
            } else {
                node->objects.push_back(obj);
            }
        }
    }

    void try_merge(Node* node) {
        if (!node || node == root.get() || !node->isLeaf()) {
            return;
        }

        Node* parent = node->parent;
        if (parent->isLeaf()) return;

        size_t total_objects = 0;
        for (int i = 0; i < 4; ++i) {
            if (!parent->children[i] || !parent->children[i]->isLeaf()) {
                return;  // 只有当所有兄弟节点都是叶子时才合并
            }
            total_objects += parent->children[i]->objects.size();
        }

        if (total_objects <= capacity) {
            std::vector<const T*> merged_objects;
            for (int i = 0; i < 4; ++i) {
                auto& child = parent->children[i];
                merged_objects.insert(merged_objects.end(),
                                      child->objects.begin(),
                                      child->objects.end());
                child.reset();
            }
            parent->objects = merged_objects;

            // 尝试向上递归合并
            try_merge(parent);
        }
    }

    void query_recursive(Node* node, const glm::vec2& point,
                         std::vector<const T*>& result) const {
        if (!node->bounds.contains(point)) {
            return;
        }

        for (const T* obj : node->objects) {
            if (get_object_bounds(obj).contains(point)) {
                result.push_back(obj);
            }
        }

        if (!node->isLeaf()) {
            int quadrant = get_quadrant_for_point(node->bounds, point);
            if (quadrant != -1) {
                query_recursive(node->children[quadrant].get(), point, result);
            }
        }
    }

    int get_quadrant(const BoundingBox& parentBounds,
                     const BoundingBox& objBounds) const {
        const auto center =
            glm::vec2{parentBounds.pos.x + parentBounds.size.x * 0.5f,
                      parentBounds.pos.y + parentBounds.size.y * 0.5f};
        if (objBounds.pos.x + objBounds.size.x < center.x) {
            if (objBounds.pos.y + objBounds.size.y < center.y)
                return 0;                              // 左上
            if (objBounds.pos.y > center.y) return 2;  // 左下
        } else if (objBounds.pos.x > center.x) {
            if (objBounds.pos.y + objBounds.size.y < center.y)
                return 1;                              // 右上
            if (objBounds.pos.y > center.y) return 3;  // 右下
        }
        return -1;  // 跨越边界
    }

    int get_quadrant_for_point(const BoundingBox& parentBounds,
                               const glm::vec2& p) const {
        const auto center =
            glm::vec2{parentBounds.pos.x + parentBounds.size.x * 0.5f,
                      parentBounds.pos.y + parentBounds.size.y * 0.5f};
        if (p.x < center.x) {
            if (p.y < center.y) return 0;  // 左上
            return 2;                      // 左下
        } else {
            if (p.y < center.y) return 1;  // 右上
            return 3;                      // 右下
        }
    }
};

#endif  // MMM_QUADTREE_HPP
