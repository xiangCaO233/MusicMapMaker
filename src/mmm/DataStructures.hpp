#ifndef MMM_DATASTRUCTURES_HPP
#define MMM_DATASTRUCTURES_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mmm/ObjectHandle.hpp>
#include <mmm/obj/Hold.hpp>
#include <mmm/obj/Note.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/timing/Timing.hpp>
#include <optional>
#include <utility>
#include <vector>

// --- 辅助函数 ---

/**
 * @brief 获取任意 Note 类型的时间区间 [start, end)。
 * @param note 指向 Note 对象的指针。
 * @return 一个表示时间区间的 std::pair。点事件被视为一个极小的区间。
 */
inline std::pair<int64_t, int64_t> get_interval(const Note* note) {
    if (!note) return {0, 0};
    if (auto* hold = dynamic_cast<const Hold*>(note)) {
        return {hold->timestamp(), hold->timestamp() + hold->duration()};
    }
    if (auto* comp = dynamic_cast<const Composite*>(note)) {
        return {comp->timestamp(), comp->timestamp() + comp->total_duration()};
    }
    // 点事件（如 Note,
    // Slide）被视为一个长度为1的区间，以确保它们能被区间查询正确捕获。
    return {note->timestamp(), note->timestamp() + 1};
}

/**
 * @class SlottedArray
 * @brief 使用空闲列表管理的槽位数组，实现O(1)的增删，并提供稳定的句柄。
 * @tparam T 存储的对象类型（基类）。
 * @tparam HandleType 用于引用对象的句柄类型。
 */
template <typename T, typename HandleType>
class SlottedArray {
   private:
    struct Slot {
        std::unique_ptr<T> data = nullptr;
        uint32_t generation = 0;
        uint32_t next_free_slot;  // 用于链接空闲列表
    };

   public:
    SlottedArray() = default;

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
            slot.generation = 1;  // 初始generation为1，0可以作为无效标志
            return {index, slot.generation};
        }
    }

    std::unique_ptr<T> remove(HandleType handle) {
        if (!is_valid(handle)) return nullptr;

        Slot& slot = m_slots[handle.index];
        slot.generation++;  // 使所有指向此槽位的旧句柄失效
        slot.next_free_slot = m_freelist_head;
        m_freelist_head = handle.index;
        return std::move(slot.data);
    }

    /**
     * @brief 替换指定句柄处的数据，并返回旧数据的所有权。
     * @param handle 要替换的目标槽位的句柄。
     * @param new_item 要放入槽位的新数据的 unique_ptr。
     * @return 被替换掉的旧数据的 unique_ptr，如果句柄无效则返回 nullptr。
     */
    std::unique_ptr<T> replace(HandleType handle, std::unique_ptr<T> new_item) {
        // 验证句柄是否指向一个有效的、存活的对象。
        if (!is_valid(handle)) {
            return nullptr;
        }

        // 定位到槽位。因为 is_valid() 已通过，所以索引是安全的。
        Slot& slot = m_slots[handle.index];

        // 使用 std::swap 原子地交换新旧数据的所有权。
        // 这个操作对于 unique_ptr 是 noexcept (不会抛出异常) 的，
        // 保证了操作的异常安全性。
        std::swap(slot.data, new_item);

        // 返回现在被 new_item 持有的旧数据。
        // 函数结束后，调用者就获得了旧数据的所有权。
        return new_item;
    }

    T* get(HandleType handle) {
        if (!is_valid(handle)) return nullptr;
        return m_slots[handle.index].data.get();
    }

    const T* get(HandleType handle) const {
        if (!is_valid(handle)) return nullptr;
        return m_slots[handle.index].data.get();
    }

    bool is_valid(HandleType handle) const {
        return handle.index < m_slots.size() &&
               m_slots[handle.index].generation == handle.generation &&
               m_slots[handle.index].data != nullptr;
    }

    // --- 迭代器实现 ---
    template <bool IsConst>
    class base_iterator {
       public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = typename std::conditional<IsConst, const T*, T*>::type;
        using reference =
            typename std::conditional<IsConst, const T&, T&>::type;
        using StoragePointer =
            typename std::conditional<IsConst, const SlottedArray*,
                                      SlottedArray*>::type;

        base_iterator(StoragePointer array, uint32_t index)
            : m_array(array), m_index(index) {
            if (m_index < m_array->m_slots.size()) {
                advance_to_valid();
            }
        }

        reference operator*() const { return *m_array->m_slots[m_index].data; }
        pointer operator->() const {
            return m_array->m_slots[m_index].data.get();
        }

        base_iterator& operator++() {
            if (m_index < m_array->m_slots.size()) {
                m_index++;
                advance_to_valid();
            }
            return *this;
        }
        base_iterator operator++(int) {
            base_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        base_iterator& operator--() {
            do {
                if (m_index == 0) return *this;  // 无法再递减
                m_index--;
            } while (m_index < m_array->m_slots.size() &&
                     !m_array->m_slots[m_index].data);
            return *this;
        }
        base_iterator operator--(int) {
            base_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        friend bool operator==(const base_iterator& a, const base_iterator& b) {
            return a.m_array == b.m_array && a.m_index == b.m_index;
        }
        friend bool operator!=(const base_iterator& a, const base_iterator& b) {
            return !(a == b);
        }

        template <bool OtherIsConst, typename = typename std::enable_if<
                                         IsConst && !OtherIsConst>::type>
        base_iterator(const base_iterator<OtherIsConst>& other)
            : m_array(other.m_array), m_index(other.m_index) {}

       private:
        template <bool>
        friend class base_iterator;
        void advance_to_valid() {
            while (m_index < m_array->m_slots.size() &&
                   !m_array->m_slots[m_index].data) {
                m_index++;
            }
        }

        StoragePointer m_array;
        uint32_t m_index;
    };
    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, m_slots.size()); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, m_slots.size()); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, m_slots.size()); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

   private:
    std::vector<Slot> m_slots;
    int32_t m_freelist_head = -1;  // -1 表示空闲列表为空
};

/**
 * @class IntervalTree
 * @brief 一个增强的二叉搜索树，用于在 O(log N + k) 时间内找到所有重叠的区间。
 * @note 这是一个非自平衡的实现。在插入/删除非常频繁且模式极端的情况下，
 *       性能可能退化。对于典型的编辑器使用场景，其平均性能足够好。
 */
class IntervalTree {
   public:
    using Interval = std::pair<int64_t, int64_t>;

    struct Node {
        Interval interval;  ///< 节点代表的时间区间 [start, end)。
        NoteHandle handle;  ///< 指向该区间的音符的句柄。
        int64_t max_end;    ///< 以此节点为根的子树中所有区间的最大结束点。

        Node* left = nullptr;
        Node* right = nullptr;
        Node* parent = nullptr;

        Node(Interval iv, NoteHandle h)
            : interval(iv), handle(h), max_end(iv.second) {}
    };

    ~IntervalTree() {
        if (m_root) delete_recursive(m_root);
    }

    Node* insert(Interval interval, NoteHandle handle) {
        Node* new_node = new Node(interval, handle);
        if (!m_root) {
            m_root = new_node;
            return m_root;
        }
        Node* current = m_root;
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

    // ================== [MODIFIED START] ==================
    // 更改1：将 successor 设为 public，以便 NoteCollection 可以调用它
    Node* successor(Node* x) {
        if (!x || !x->right) return nullptr;
        Node* current = x->right;
        while (current->left) current = current->left;
        return current;
    }

    // 更改2：使用更健壮的 remove 实现
    void remove(Node* node_to_delete) {
        if (!node_to_delete) {
            return;
        }

        Node* node_to_unlink;  // 这是将要被物理上从树中移除的节点。
        Node* child;  // 这是 node_to_unlink 的单个子节点（或 nullptr）。

        // 步骤 1: 确定哪个节点需要被物理上摘除。
        if (node_to_delete->left == nullptr ||
            node_to_delete->right == nullptr) {
            node_to_unlink = node_to_delete;
        } else {
            node_to_unlink = successor(node_to_delete);
        }

        // 步骤 2: 找到 node_to_unlink 的子节点（最多只有一个）。
        if (node_to_unlink->left != nullptr) {
            child = node_to_unlink->left;
        } else {
            child = node_to_unlink->right;
        }

        // 步骤 3: 在修改任何指针之前，安全地获取将要开始更新 max_end 的父节点。
        Node* update_start_node = node_to_unlink->parent;

        // 步骤 4: 将 child 连接到 node_to_unlink 的父节点。
        if (child != nullptr) {
            child->parent = node_to_unlink->parent;
        }

        if (node_to_unlink->parent == nullptr) {
            m_root = child;
        } else if (node_to_unlink == node_to_unlink->parent->left) {
            node_to_unlink->parent->left = child;
        } else {
            node_to_unlink->parent->right = child;
        }

        // 步骤 5:
        // 如果我们摘除的是后继者节点，需要将其数据复制回最初请求删除的节点。
        if (node_to_unlink != node_to_delete) {
            node_to_delete->interval = node_to_unlink->interval;
            node_to_delete->handle = node_to_unlink->handle;
        }

        // 步骤 6: 从受影响的父节点开始，向上更新 max_end 属性。
        update_max_end_upwards(update_start_node);

        // 步骤 7: 现在可以安全地删除被摘除的节点了。
        delete node_to_unlink;
    }
    // ================== [MODIFIED END] ==================

    void find_overlapping(Interval query,
                          std::vector<NoteHandle>& result) const {
        if (!m_root) return;
        find_overlapping_recursive(m_root, query, result);
    }

   private:
    int64_t calculate_max_end(Node* node) {
        if (!node) return -1;
        int64_t max_val = node->interval.second;
        if (node->left) max_val = std::max(max_val, node->left->max_end);
        if (node->right) max_val = std::max(max_val, node->right->max_end);
        return max_val;
    }

    void update_max_end_upwards(Node* start_node) {
        Node* current = start_node;
        while (current) {
            int64_t new_max_end = calculate_max_end(current);
            if (current->max_end == new_max_end) break;
            current->max_end = new_max_end;
            current = current->parent;
        }
    }

    void find_overlapping_recursive(Node* node, const Interval& query,
                                    std::vector<NoteHandle>& result) const {
        if (!node) return;
        if (node->max_end < query.first) return;
        if (node->left) find_overlapping_recursive(node->left, query, result);
        if (node->interval.first < query.second &&
            node->interval.second > query.first) {
            result.push_back(node->handle);
        }
        if (node->interval.first > query.second) return;
        if (node->right) find_overlapping_recursive(node->right, query, result);
    }

    void delete_recursive(Node* node) {
        if (node->left) delete_recursive(node->left);
        if (node->right) delete_recursive(node->right);
        delete node;
    }

    Node* m_root = nullptr;
};

/**
 * @class NoteCollection
 * @brief 核心数据结构，管理所有音符的生命周期和查询。
 */
class NoteCollection {
   public:
    using TimelineIndex = std::map<int64_t, std::vector<NoteHandle>>;
    using Node = IntervalTree::Node;  // 方便使用

    NoteCollection() = default;

    // --- 写入/修改 API ---
    NoteHandle add_note(std::unique_ptr<Note> note) {
        if (!note) return {};
        auto handle = m_storage.add(std::move(note));
        const Note* added_note = m_storage.get(handle);
        m_timeline[added_note->timestamp()].push_back(handle);
        auto* node_ptr =
            m_interval_tree.insert(get_interval(added_note), handle);
        m_handle_to_interval_node[handle] = node_ptr;
        return handle;
    }

    // ================== [MODIFIED START] ==================
    // 更改3：重写 remove_note 以正确同步 m_handle_to_interval_node
    std::unique_ptr<Note> remove_note(NoteHandle handle) {
        if (!m_storage.is_valid(handle)) return nullptr;
        const Note* note_ptr = m_storage.get(handle);

        // a. 更新时间线索引
        auto& notes_at_ts = m_timeline.at(note_ptr->timestamp());
        notes_at_ts.erase(
            std::remove(notes_at_ts.begin(), notes_at_ts.end(), handle),
            notes_at_ts.end());
        if (notes_at_ts.empty()) m_timeline.erase(note_ptr->timestamp());

        // b. 更新区间树和反向映射索引
        auto it = m_handle_to_interval_node.find(handle);
        if (it != m_handle_to_interval_node.end()) {
            Node* node_to_delete = it->second;

            // 1. 预判：检查是否会发生复杂的“后继节点替换”情况。
            bool successor_swap_will_occur = false;
            NoteHandle successor_handle;
            if (node_to_delete->left && node_to_delete->right) {
                Node* successor_node =
                    m_interval_tree.successor(node_to_delete);
                if (successor_node) {
                    successor_handle = successor_node->handle;
                    successor_swap_will_occur = true;
                }
            }

            // 2. 执行树的删除操作。
            m_interval_tree.remove(node_to_delete);

            // 3. 同步 map！这是最关键的一步。
            if (successor_swap_will_occur) {
                // a. 后继节点的物理内存已被释放，其在 map
                // 中的条目现在是悬垂指针！必须移除。
                m_handle_to_interval_node.erase(successor_handle);

                // b. 我们请求删除的 handle 对应的节点 `node_to_delete`
                // 仍然存在，
                //    但现在包含的是后继节点的数据。因此，必须将后继节点的句柄
                //    重新映射到这个“幸存”的节点上。
                m_handle_to_interval_node[successor_handle] = node_to_delete;
            }

            // 4. 无论如何，最初请求删除的句柄在逻辑上已经不存在于树中，从 map
            // 中移除它。
            m_handle_to_interval_node.erase(handle);
        }

        // c. 最后从存储层移除
        return std::move(m_storage.remove(handle));
    }

    // 更改4：重写 update_note 以正确同步 m_handle_to_interval_node
    std::unique_ptr<Note> update_note(NoteHandle handle,
                                      std::unique_ptr<Note> new_note_data) {
        if (!m_storage.is_valid(handle) || !new_note_data) return nullptr;

        std::unique_ptr<Note> old_note_data =
            m_storage.replace(handle, std::move(new_note_data));

        if (!old_note_data) {
            return nullptr;
        }

        const Note* old_note_ptr = old_note_data.get();
        const Note* new_note_ptr = m_storage.get(handle);

        if (!new_note_ptr) {
            m_storage.replace(handle, std::move(old_note_data));
            return nullptr;
        }

        // a. 更新时间线索引 (m_timeline)
        if (old_note_ptr->timestamp() != new_note_ptr->timestamp()) {
            auto& old_notes_at_ts = m_timeline.at(old_note_ptr->timestamp());
            old_notes_at_ts.erase(std::remove(old_notes_at_ts.begin(),
                                              old_notes_at_ts.end(), handle),
                                  old_notes_at_ts.end());
            if (old_notes_at_ts.empty()) {
                m_timeline.erase(old_note_ptr->timestamp());
            }
            m_timeline[new_note_ptr->timestamp()].push_back(handle);
        }

        // b. 更新区间树索引 (m_interval_tree)
        if (get_interval(old_note_ptr) != get_interval(new_note_ptr)) {
            auto it = m_handle_to_interval_node.find(handle);
            if (it != m_handle_to_interval_node.end()) {
                Node* node_to_delete = it->second;

                // 1. 预判是否会发生“后继节点替换”。
                bool successor_swap_will_occur = false;
                NoteHandle successor_handle;
                if (node_to_delete->left && node_to_delete->right) {
                    Node* successor_node =
                        m_interval_tree.successor(node_to_delete);
                    if (successor_node) {
                        successor_handle = successor_node->handle;
                        successor_swap_will_occur = true;
                    }
                }

                // 2. 从树中删除旧的区间。
                m_interval_tree.remove(node_to_delete);

                // 3. 同步 map 以清理旧状态。
                if (successor_swap_will_occur) {
                    m_handle_to_interval_node.erase(successor_handle);
                    m_handle_to_interval_node[successor_handle] =
                        node_to_delete;
                }
                m_handle_to_interval_node.erase(handle);

                // 4. 为新数据插入新的区间节点。
                auto* new_node_ptr =
                    m_interval_tree.insert(get_interval(new_note_ptr), handle);
                m_handle_to_interval_node[handle] = new_node_ptr;
            }
        }
        return old_note_data;
    }
    // ================== [MODIFIED END] ==================

    // --- 读取/查询 API ---
    const Note* get_note(NoteHandle handle) const {
        return m_storage.get(handle);
    }

    std::vector<NoteHandle> query_range(int64_t start_time,
                                        int64_t end_time) const {
        std::vector<NoteHandle> result_handles;
        m_interval_tree.find_overlapping({start_time, end_time},
                                         result_handles);
        std::sort(result_handles.begin(), result_handles.end(),
                  [this](NoteHandle a, NoteHandle b) {
                      const Note* note_a = get_note(a);
                      const Note* note_b = get_note(b);
                      if (!note_a || !note_b) return false;
                      if (note_a->timestamp() != note_b->timestamp())
                          return note_a->timestamp() < note_b->timestamp();
                      return note_a->trackpos() < note_b->trackpos();
                  });
        return result_handles;
    }

    std::vector<NoteHandle> query_rect(int64_t start_time, int64_t end_time,
                                       int32_t start_track,
                                       int32_t end_track) const {
        std::vector<NoteHandle> handles_in_time;
        m_interval_tree.find_overlapping({start_time, end_time},
                                         handles_in_time);
        std::vector<NoteHandle> final_handles;
        final_handles.reserve(handles_in_time.size());
        for (const auto& handle : handles_in_time) {
            const Note* note = get_note(handle);
            if (note && note->trackpos() >= start_track &&
                note->trackpos() < end_track) {
                final_handles.push_back(handle);
            }
        }
        return final_handles;
    }

    // --- 遍历 API ---
    SlottedArray<Note, NoteHandle>& get_all_notes_unordered() {
        return m_storage;
    }

    const SlottedArray<Note, NoteHandle>& get_all_notes_unordered() const {
        return m_storage;
    }

    std::vector<NoteHandle> get_all_notes_ordered() const {
        std::vector<NoteHandle> all_handles;
        all_handles.reserve(m_handle_to_interval_node.size());
        for (const auto& pair : m_timeline) {
            all_handles.insert(all_handles.end(), pair.second.begin(),
                               pair.second.end());
        }
        std::sort(all_handles.begin(), all_handles.end(),
                  [this](NoteHandle a, NoteHandle b) {
                      const Note* note_a = get_note(a);
                      const Note* note_b = get_note(b);
                      if (!note_a || !note_b) return false;
                      if (note_a->timestamp() != note_b->timestamp())
                          return note_a->timestamp() < note_b->timestamp();
                      return note_a->trackpos() < note_b->trackpos();
                  });
        return all_handles;
    }

    /**
     * @brief 查找时间戳最接近给定时间戳的一个或多个音符。
     * @param timestamp 目标时间戳。
     * @return 包含一个或多个音符句柄的vector。如果集合为空，则返回空vector。
     */
    std::vector<NoteHandle> find_closest(int64_t timestamp) const {
        if (m_timeline.empty()) {
            return {};
        }

        // lower_bound 找到第一个不小于 timestamp 的元素。
        auto it_after = m_timeline.lower_bound(timestamp);

        if (it_after != m_timeline.end() && it_after->first == timestamp) {
            return it_after->second;
        }

        if (it_after == m_timeline.begin()) {
            return it_after != m_timeline.end() ? it_after->second
                                                : std::vector<NoteHandle>{};
        }

        auto it_before = std::prev(it_after);

        if (it_after == m_timeline.end()) {
            return it_before->second;
        }

        int64_t dist_before = timestamp - it_before->first;
        int64_t dist_after = it_after->first - timestamp;

        if (dist_before < dist_after) {
            return it_before->second;
        } else if (dist_after < dist_before) {
            return it_after->second;
        } else {
            std::vector<NoteHandle> result = it_before->second;
            result.insert(result.end(), it_after->second.begin(),
                          it_after->second.end());
            return result;
        }
    }

    /**
     * @brief 查找时间戳严格小于给定时间戳的、时间上最晚的一个或多个音符。
     * @param timestamp 目标时间戳（不包含）。
     * @return 包含一个或多个音符句柄的vector，如果不存在则返回空vector。
     */
    std::vector<NoteHandle> find_greatest_before(int64_t timestamp) const {
        if (m_timeline.empty()) {
            return {};
        }
        auto it = m_timeline.lower_bound(timestamp);
        if (it == m_timeline.begin()) {
            return {};
        }
        --it;
        return it->second;
    }

    /**
     * @brief 查找时间戳严格大于给定时间戳的、时间上最早的一个或多个音符。
     * @param timestamp 目标时间戳（不包含）。
     * @return 包含一个或多个音符句柄的vector，如果不存在则返回空vector。
     */
    std::vector<NoteHandle> find_smallest_after(int64_t timestamp) const {
        if (m_timeline.empty()) {
            return {};
        }
        auto it = m_timeline.upper_bound(timestamp);
        if (it == m_timeline.end()) {
            return {};
        }
        return it->second;
    };

   private:
    /// @brief 存储层：提供稳定引用和O(1)增删。
    SlottedArray<Note, NoteHandle> m_storage;

    /// @brief 索引层 1: 主时间轴索引，用于按时间点精确查找。
    TimelineIndex m_timeline;

    /// @brief 索引层 2: 区间索引，用于高效的渲染范围查询。
    IntervalTree m_interval_tree;

    /// @brief 反向映射：从句柄快速定位到其在区间树中的节点，实现O(log N)更新。
    std::unordered_map<NoteHandle, Node*, NoteHandle::Hash>
        m_handle_to_interval_node;
};

/**
 * @class TimingMap
 * @brief 高效管理时间映射点（Timing Points）的数据结构。
 * @details 内部使用 std::map 按时间戳排序，支持高效的查找、增删改。
 */
class TimingMap {
   public:
    TimingMap() = default;

    /**
     * @brief 添加或更新一个时间点。
     * @param timing 要添加的 Timing 对象。
     * @return 如果添加或更新成功，返回刚刚添加的确切地址
     * @note 如果该时间戳已存在一个时间点，它将被新的时间点覆盖。
     */
    Timing* add_timing_point(std::unique_ptr<Timing> timing) {
        // 直接使用 map 的下标运算符，如果不存在则创建，存在则向后添加
        // 查找具有给定时间戳的条目
        // 如果时间戳存在于 map 中
        if (auto map_it = m_timeline.find(timing->timestamp);
            map_it != m_timeline.end()) {
            // 获取与时间戳关联的 std::vector<Timing> 的引用
            auto& timings_vec = map_it->second;

            // 在 vector 中查找此 Timing 对象
            auto vec_it =
                std::find(timings_vec.begin(), timings_vec.end(), timing);

            // 如果在 vector 中找到了该对象
            if (vec_it != timings_vec.end()) {
                // 已有当前timing,添加失败
                return nullptr;
            } else {
                Timing* added_timing_ptr = timing.get();
                timings_vec.push_back(std::move(timing));
                m_version++;
                return added_timing_ptr;
            }
        }
        Timing* added_timing_ptr = timing.get();
        // 如果时间戳不存在，则直接添加
        m_timeline[timing->timestamp].push_back(std::move(timing));
        m_version++;
        return added_timing_ptr;
    }

    /**
     * @brief 移除一个指定时间戳的全部时间点。
     * @param timestamp 要移除的时间点的时间戳。
     * @return 如果找到了，则返回被移除的 Timing 对象列表；否则返回
     * std::nullopt。
     */
    std::optional<std::vector<std::unique_ptr<Timing>>> remove_timings_at_point(
        int32_t timestamp) {
        auto it = m_timeline.find(timestamp);
        if (it != m_timeline.end()) {
            // 使用 std::move 将 vector 的内容移出，避免不必要的拷贝
            std::vector<std::unique_ptr<Timing>> removed_timings =
                std::move(it->second);
            m_timeline.erase(it);  // 从 map 中移除该节点
            m_version++;
            return removed_timings;  // 返回被移除的数据
        }
        return std::nullopt;  // 没有找到任何东西
    }

    /**
     * @brief 移除一个指定的时间点。
     * @param timing 要移除的时间点的时间戳。
     * @return 如果找到了并成功移除，返回 true。
     */
    std::unique_ptr<Timing> remove_timing_point(Timing* timing_ptr) {
        if (!timing_ptr) return nullptr;

        auto map_it = m_timeline.find(timing_ptr->timestamp);
        if (map_it == m_timeline.end()) return nullptr;

        auto& timings_vec = map_it->second;
        auto vec_it =
            std::find_if(timings_vec.begin(), timings_vec.end(),
                         [&](const auto& p) { return *p == *timing_ptr; });

        if (vec_it != timings_vec.end()) {
            // 从 vector 中“取出”unique_ptr 的所有权
            std::unique_ptr<Timing> removed_timing = std::move(*vec_it);
            timings_vec.erase(vec_it);

            if (timings_vec.empty()) {
                m_timeline.erase(map_it);
            }
            m_version++;
            return removed_timing;  // 返回被删除对象的所有权
        }
        return nullptr;
    }

    /**
     * @brief 查找在给定时间戳下生效的时间点。
     * @param timestamp 目标时间戳。
     * @return 指向生效的 Timing 对象的 const
     * 指针。如果不存在（例如，时间戳在第一个时间点之前），则返回 nullptr。
     */
    const Timing* get_active_timing_point(int32_t timestamp) const {
        if (m_timeline.empty()) {
            return nullptr;
        }

        // upper_bound 找到第一个键 > timestamp 的元素。
        auto it = m_timeline.upper_bound(timestamp);

        // 如果 it 是 begin(), 说明 timestamp 比所有已知时间点都早。
        if (it == m_timeline.begin()) {
            return nullptr;
        }

        // 我们要找的是 it 的前一个元素，它就是小于或等于 timestamp
        // 的最后一个时间点。
        --it;
        return it->second.back().get();
    }

    /**
     * @brief 更新一个 Timing 对象，允许时间戳的改变。
     * @return 一个 pair。first 为 bool 代表成功/失败。
     *         second 为 unique_ptr，成功时持有旧数据，失败时归还新数据。
     */
    std::pair<bool, std::unique_ptr<Timing>> update_timing_point(
        Timing* old_timing_ptr, std::unique_ptr<Timing> new_timing_data) {
        // --- 1. 输入验证 ---
        if (!old_timing_ptr || !new_timing_data) {
            // 失败，归还 new_timing_data 的所有权
            return {false, std::move(new_timing_data)};
        }

        // --- 2. 查找并移除旧对象 (逻辑不变) ---
        auto old_timestamp = old_timing_ptr->timestamp;
        auto map_it_old = m_timeline.find(old_timestamp);
        if (map_it_old == m_timeline.end()) {
            return {false, std::move(new_timing_data)};
        }
        auto& vec_old = map_it_old->second;
        auto vec_it_old =
            std::find_if(vec_old.begin(), vec_old.end(),
                         [&](const auto& p) { return *p == *old_timing_ptr; });
        if (vec_it_old == vec_old.end()) {
            return {false, std::move(new_timing_data)};
        }
        std::unique_ptr<Timing> old_data_backup = std::move(*vec_it_old);
        vec_old.erase(vec_it_old);

        // --- 3. 尝试将新对象添加到新位置 (逻辑不变) ---
        auto new_timestamp = new_timing_data->timestamp;
        auto& vec_new = m_timeline[new_timestamp];
        auto vec_it_new =
            std::find_if(vec_new.begin(), vec_new.end(),
                         [&](const auto& p) { return *p == *new_timing_data; });

        if (vec_it_new != vec_new.end()) {
            // --- 失败处理：回滚并归还所有权 ---
            map_it_old->second.push_back(std::move(old_data_backup));
            // 明确地返回失败状态和 new_timing_data 的所有权
            return {false, std::move(new_timing_data)};
        }

        // --- 4. 成功：完成添加和清理 (逻辑不变) ---
        vec_new.push_back(std::move(new_timing_data));
        if (map_it_old->second.empty()) {
            m_timeline.erase(map_it_old);
        }
        m_version++;

        // 明确地返回成功状态和 old_data_backup 的所有权
        return {true, std::move(old_data_backup)};
    }

    /**
     * @brief 获取指定时间戳的精确时间点（如果存在）。
     * @param timestamp 目标时间戳。
     * @return 指向 Timing 对象的 const 指针。如果该时间戳没有定义时间点，则返回
     * nullptr。
     */
    // const Timing* get_timing_point_at(int32_t timestamp) const {
    //     if (auto it = m_timeline.find(timestamp); it != m_timeline.end()) {
    //         return it->second.;
    //     }
    //     return nullptr;
    // }

    /**
     * @brief 获取所有时间点的只读引用，按时间戳排序。
     * @return 一个对内部 map 的 const 引用。
     */
    const std::map<int32_t, std::vector<std::unique_ptr<Timing>>>&
    get_all_timing_points() const {
        return m_timeline;
    }

    /**
     * @brief 清空所有时间点。
     */
    void clear() { m_timeline.clear(); }

    // 获取版本号的接口
    uint64_t getVersion() const { return m_version; }

   private:
    // 使用 std::map 作为核心存储。Key 是时间戳，Value 是 Timing 对象。
    // std::map 自动按 Key 排序，并提供高效的对数时间复杂度查找。
    std::map<int32_t, std::vector<std::unique_ptr<Timing>>> m_timeline;
    uint64_t m_version{0};
};

#endif  // MMM_DATASTRUCTURES_HPP
