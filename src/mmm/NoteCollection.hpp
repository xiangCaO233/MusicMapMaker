#include <cstddef>
#include <cstdint>
#include <functional>
#include <mmm/obj/Composite.hpp>
#include <mmm/obj/Hold.hpp>
#include <mmm/obj/Note.hpp>

// 句柄
// 结合索引和generation,防止ABA问题

struct NoteHandle {
    uint32_t index;
    uint32_t generation;

    bool operator==(const NoteHandle& other) const = default;

    // 可用于哈希表
    struct Hash {
        size_t operator()(const NoteHandle& h) const {
            return std::hash<uint64_t>{}(
                (static_cast<uint64_t>(h.generation) << 32) | h.index);
        }
    };
};

// 获取物件区间
inline std::pair<int64_t, int64_t> get_interval(const Note* note) {
    if (!note) return {0, 0};
    if (auto* hold = dynamic_cast<const Hold*>(note)) {
        return {hold->timestamp(), hold->timestamp() + hold->durationtime()};
    }
    if (auto* comp = dynamic_cast<const Composite*>(note)) {
        return {comp->timestamp(),
                comp->timestamp() + comp->total_durationtime()};
    }
    // Note视为一个极小但非零的区间，以避免端点问题
    return {note->timestamp(), note->timestamp() + 1};
}

#include <memory>

// 插槽数组
template <typename T, typename HandleType>
class SlottedArray {
   public:
    struct Slot {
        std::unique_ptr<T> data = nullptr;
        uint32_t generation = 0;
        uint32_t next_free_slot;  // 用于链接空闲列表
    };

    SlottedArray() = default;

    std::unique_ptr<T>& get_mutable(HandleType handle) {
        if (!is_valid(handle)) {
            throw std::runtime_error("Invalid handle in get_mutable");
        }
        return m_slots[handle.index].data;
    }

    // 添加
    HandleType add(std::unique_ptr<T> item) {
        if (m_freelist_head != -1) {
            const uint32_t index = m_freelist_head;
            Slot& slot = m_slots[index];
            m_freelist_head = slot.next_free_slot;

            slot.data = std::move(item);
            return {index, slot.generation};
        } else {
            const uint32_t index = m_slots.size();
            Slot& slot = m_slots.emplace_back();
            slot.data = std::move(item);
            slot.generation = 1;  // 初始generation
            return {index, slot.generation};
        }
    }

    // 移除
    void remove(HandleType handle) {
        if (!is_valid(handle)) return;

        Slot& slot = m_slots[handle.index];
        slot.data.reset();
        slot.generation++;  // 使所有旧句柄失效
        slot.next_free_slot = m_freelist_head;
        m_freelist_head = handle.index;
    }

    // 根据句柄获取
    T* get(HandleType handle) {
        if (!is_valid(handle)) return nullptr;
        return m_slots[handle.index].data.get();
    }

    // 根据句柄获取
    const T* get(HandleType handle) const {
        if (!is_valid(handle)) return nullptr;
        return m_slots[handle.index].data.get();
    }

    // 句柄查询是否有效
    bool is_valid(HandleType handle) const {
        return handle.index < m_slots.size() &&
               m_slots[handle.index].generation == handle.generation &&
               m_slots[handle.index].data != nullptr;
    }

   private:
    std::vector<Slot> m_slots;
    int32_t m_freelist_head = -1;
};

// 区间树-基于红黑树
class IntervalTree {
   public:
    using Interval = std::pair<int64_t, int64_t>;

    struct Node {
        Interval interval;
        NoteHandle handle;
        int64_t max_end;  // 子树中最大的结束点

        Node* left = nullptr;
        Node* right = nullptr;
        Node* parent = nullptr;

        Node(Interval iv, NoteHandle h)
            : interval(iv), handle(h), max_end(iv.second) {}
    };

    // 返回插入的节点指针，以便于删除
    Node* insert(Interval interval, NoteHandle handle) {
        if (!m_root) {
            m_root = new Node(interval, handle);
            return m_root;
        }

        Node* current = m_root;
        Node* new_node = new Node(interval, handle);

        while (true) {
            current->max_end = std::max(current->max_end, new_node->max_end);
            if (new_node->interval.first < current->interval.first) {
                if (!current->left) {
                    current->left = new_node;
                    new_node->parent = current;
                    break;
                }
                current = current->left;
            } else {
                if (!current->right) {
                    current->right = new_node;
                    new_node->parent = current;
                    break;
                }
                current = current->right;
            }
        }
        return new_node;
    }

    // 简化版删除。生产代码需要处理复杂的红黑树平衡。
    void remove(Node* z) {
        if (!z) return;

        Node* y;  // 要被物理删除或移动的节点
        Node* x;  // y 的子节点，用来接替 y 的位置

        // 确定 y：如果 z 最多只有一个子节点，y就是z；否则y是z的后继
        if (!z->left || !z->right) {
            y = z;
        } else {
            y = successor(z);
        }

        // 确定 x：x 是 y 的唯一子节点，或者nullptr
        if (y->left) {
            x = y->left;
        } else {
            x = y->right;
        }

        // 将 x 连接到 y 的父节点
        if (x) {
            x->parent = y->parent;
        }

        if (!y->parent) {
            m_root = x;  // y 是根节点
        } else if (y == y->parent->left) {
            y->parent->left = x;
        } else {
            y->parent->right = x;
        }

        // 如果 y 是 z 的后继，需要将 y 的数据复制到 z
        if (y != z) {
            // 我们不能直接交换指针，因为外部的句柄映射还指着z
            // 所以我们只交换内容
            z->interval = y->interval;
            z->handle = y->handle;
            // z 的 max_end 将在下面的 update_max_end_upwards 中被正确更新
        }

        // 从 y 的父节点开始，向上更新 max_end
        update_max_end_upwards(y->parent);

        delete y;  // 物理删除节点 y
    }

    void find_overlapping(Interval query,
                          std::vector<NoteHandle>& result) const {
        if (!m_root) return;
        find_overlapping_recursive(m_root, query, result);
    }

    ~IntervalTree() {
        // 清理所有节点
        if (m_root) delete_recursive(m_root);
    }

   private:
    // 找到节点x的中序后继（右子树中的最小值）
    Node* successor(Node* x) {
        if (!x || !x->right) return nullptr;
        Node* current = x->right;
        while (current->left) {
            current = current->left;
        }
        return current;
    }

    // 计算一个节点的 max_end
    int64_t calculate_max_end(Node* node) {
        if (!node) return -1;  // 或者一个合适的最小值
        int64_t max_val = node->interval.second;
        if (node->left) {
            max_val = std::max(max_val, node->left->max_end);
        }
        if (node->right) {
            max_val = std::max(max_val, node->right->max_end);
        }
        return max_val;
    }

    // 从指定节点开始，向上回溯更新所有祖先的 max_end
    void update_max_end_upwards(Node* start_node) {
        Node* current = start_node;
        while (current) {
            int64_t new_max_end = calculate_max_end(current);
            if (current->max_end == new_max_end) {
                // 如果 max_end 没有变化，则其上的祖先也不会变，可以提前退出
                break;
            }
            current->max_end = new_max_end;
            current = current->parent;
        }
    }

    void find_overlapping_recursive(Node* node, const Interval& query,
                                    std::vector<NoteHandle>& result) const {
        if (!node) return;

        // 智能剪枝：如果当前节点的子树最大结束点都小于查询起点，则无需搜索
        if (node->max_end < query.first) {
            return;
        }

        // 递归左子树
        if (node->left) {
            find_overlapping_recursive(node->left, query, result);
        }

        // 检查当前节点是否重叠
        if (node->interval.first < query.second &&
            node->interval.second > query.first) {
            result.push_back(node->handle);
        }

        // 只有当查询起点在当前节点起点之后，才需要搜索右子树
        if (query.first < node->interval.first) {
            return;  // 优化：因为树是按起点排序的
        }

        // 递归右子树
        if (node->right) {
            find_overlapping_recursive(node->right, query, result);
        }
    }

    void delete_recursive(Node* node) {
        if (node->left) delete_recursive(node->left);
        if (node->right) delete_recursive(node->right);
        delete node;
    }

    Node* m_root = nullptr;
};

#include <algorithm>
#include <map>

class NoteCollection {
   public:
    using TimelineIndex = std::map<int64_t, std::vector<NoteHandle>>;

    NoteCollection() = default;

    // --- 写入 API ---
    NoteHandle add_note(std::unique_ptr<Note> note) {
        if (!note) return {};

        auto handle = m_storage.add(std::move(note));
        const Note* added_note = m_storage.get(handle);

        // 1. 更新时间轴索引
        m_timeline[added_note->timestamp()].push_back(handle);

        // 2. 更新区间树
        auto interval = get_interval(added_note);
        auto* node_ptr = m_interval_tree.insert(interval, handle);

        // 3. 更新反向映射
        m_handle_to_interval_node[handle] = node_ptr;

        return handle;
    }

    void remove_note(NoteHandle handle) {
        if (!m_storage.is_valid(handle)) return;

        const Note* note_to_remove = m_storage.get(handle);

        // 1. 从时间轴索引中移除
        auto& notes_at_ts = m_timeline[note_to_remove->timestamp()];
        notes_at_ts.erase(
            std::remove(notes_at_ts.begin(), notes_at_ts.end(), handle),
            notes_at_ts.end());
        if (notes_at_ts.empty()) {
            m_timeline.erase(note_to_remove->timestamp());
        }

        // 2. 从区间树中移除
        auto it = m_handle_to_interval_node.find(handle);
        if (it != m_handle_to_interval_node.end()) {
            m_interval_tree.remove(it->second);
            m_handle_to_interval_node.erase(it);
        }

        // 3. 最后从存储层移除
        m_storage.remove(handle);
    }

    // 健壮的、原地更新的实现
    bool update_note(NoteHandle handle, std::unique_ptr<Note> new_note_data) {
        if (!m_storage.is_valid(handle) || !new_note_data) {
            return false;
        }

        const Note* old_note = m_storage.get(handle);
        const Note* new_note = new_note_data.get();  // 获取裸指针以便访问

        // 检查关键索引属性是否发生变化
        bool timestamp_changed =
            (old_note->timestamp() != new_note->timestamp());
        bool interval_changed =
            (get_interval(old_note) != get_interval(new_note));

        // 1. 如果需要，更新时间轴索引
        if (timestamp_changed) {
            // 从旧的时间戳向量中移除
            auto& old_notes_at_ts = m_timeline.at(old_note->timestamp());
            old_notes_at_ts.erase(std::remove(old_notes_at_ts.begin(),
                                              old_notes_at_ts.end(), handle),
                                  old_notes_at_ts.end());
            if (old_notes_at_ts.empty()) {
                m_timeline.erase(old_note->timestamp());
            }
            // 添加到新的时间戳向量中
            m_timeline[new_note->timestamp()].push_back(handle);
        }

        // 2. 如果需要，更新区间树
        if (interval_changed) {
            auto it = m_handle_to_interval_node.find(handle);
            if (it != m_handle_to_interval_node.end()) {
                auto node_ptr = it->second;
                // 从树中移除旧的区间节点
                m_interval_tree.remove(node_ptr);
                m_handle_to_interval_node.erase(it);

                // 插入新的区间节点
                auto new_interval = get_interval(new_note);
                auto new_node_ptr =
                    m_interval_tree.insert(new_interval, handle);
                m_handle_to_interval_node[handle] = new_node_ptr;
            }
        }

        // 在存储层中替换数据,句柄不变
        m_storage.get_mutable(handle) = std::move(new_note_data);
        return true;
    }

    // --- 读取 API ---
    const Note* get_note(NoteHandle handle) const {
        return m_storage.get(handle);
    }

    // 渲染热路径
    std::vector<NoteHandle> query_range(int64_t start_time,
                                        int64_t end_time) const {
        std::vector<NoteHandle> result_handles;
        m_interval_tree.find_overlapping({start_time, end_time},
                                         result_handles);

        // 区间树返回的结果是无序的。
        // 渲染时需要根据时间戳等对这些少量结果进行排序。
        std::sort(result_handles.begin(), result_handles.end(),
                  [this](NoteHandle a, NoteHandle b) {
                      const Note* note_a = get_note(a);
                      const Note* note_b = get_note(b);
                      if (!note_a || !note_b) return false;

                      if (note_a->timestamp() != note_b->timestamp()) {
                          return note_a->timestamp() < note_b->timestamp();
                      }
                      return note_a->trackpos() < note_b->trackpos();
                      // 更复杂的类型排序加这里
                  });
        return result_handles;
    }

    // 编辑器框选
    std::vector<NoteHandle> query_rect(int64_t start_time, int64_t end_time,
                                       int32_t start_track,
                                       int32_t end_track) const {
        std::vector<NoteHandle> handles_in_time;
        m_interval_tree.find_overlapping({start_time, end_time},
                                         handles_in_time);

        std::vector<NoteHandle> final_handles;
        for (const auto& handle : handles_in_time) {
            const Note* note = get_note(handle);
            if (note && note->trackpos() >= start_track &&
                note->trackpos() < end_track) {
                final_handles.push_back(handle);
            }
        }
        // 这里也可能需要排序
        return final_handles;
    }

   private:
    SlottedArray<Note, NoteHandle> m_storage;
    TimelineIndex m_timeline;
    IntervalTree m_interval_tree;
    std::unordered_map<NoteHandle, IntervalTree::Node*, NoteHandle::Hash>
        m_handle_to_interval_node;
};
