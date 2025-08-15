#ifndef MMM_DATASTRUCTURES_HPP
#define MMM_DATASTRUCTURES_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mmm/NoteHandle.hpp>
#include <mmm/obj/Hold.hpp>
#include <mmm/obj/Note.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/timing/Timing.hpp>

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
 * @file SlottedArray.h
 * @brief 一个支持高效增删和稳定引用的对象存储容器。
 */

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

    void remove(HandleType handle) {
        if (!is_valid(handle)) return;

        Slot& slot = m_slots[handle.index];
        slot.data.reset();
        slot.generation++;  // 使所有指向此槽位的旧句柄失效
        slot.next_free_slot = m_freelist_head;
        m_freelist_head = handle.index;
    }

    T* get(HandleType handle) {
        if (!is_valid(handle)) return nullptr;
        return m_slots[handle.index].data.get();
    }

    const T* get(HandleType handle) const {
        if (!is_valid(handle)) return nullptr;
        return m_slots[handle.index].data.get();
    }

    std::unique_ptr<T>& get_mutable(HandleType handle) {
        if (!is_valid(handle)) {
            throw std::runtime_error(
                "Attempt to get mutable from invalid handle.");
        }
        return m_slots[handle.index].data;
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
        return const_reverse_iterator(end());
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

    void remove(Node* node_to_delete) {
        if (!node_to_delete) return;
        Node *y, *x;
        if (!node_to_delete->left || !node_to_delete->right)
            y = node_to_delete;
        else
            y = successor(node_to_delete);
        if (y->left)
            x = y->left;
        else
            x = y->right;
        if (x) x->parent = y->parent;
        if (!y->parent)
            m_root = x;
        else if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
        if (y != node_to_delete) {
            node_to_delete->interval = y->interval;
            node_to_delete->handle = y->handle;
        }
        update_max_end_upwards(y->parent);
        delete y;
    }

    void find_overlapping(Interval query,
                          std::vector<NoteHandle>& result) const {
        if (!m_root) return;
        find_overlapping_recursive(m_root, query, result);
    }

   private:
    Node* successor(Node* x) {
        if (!x || !x->right) return nullptr;
        Node* current = x->right;
        while (current->left) current = current->left;
        return current;
    }

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

    void remove_note(NoteHandle handle) {
        if (!m_storage.is_valid(handle)) return;
        const Note* note_to_remove = m_storage.get(handle);
        auto& notes_at_ts = m_timeline.at(note_to_remove->timestamp());
        notes_at_ts.erase(
            std::remove(notes_at_ts.begin(), notes_at_ts.end(), handle),
            notes_at_ts.end());
        if (notes_at_ts.empty()) m_timeline.erase(note_to_remove->timestamp());
        auto it = m_handle_to_interval_node.find(handle);
        if (it != m_handle_to_interval_node.end()) {
            m_interval_tree.remove(it->second);
            m_handle_to_interval_node.erase(it);
        }
        m_storage.remove(handle);
    }

    bool update_note(NoteHandle handle, std::unique_ptr<Note> new_note_data) {
        if (!m_storage.is_valid(handle) || !new_note_data) return false;
        const Note* old_note = m_storage.get(handle);
        const Note* new_note = new_note_data.get();
        bool timestamp_changed =
            (old_note->timestamp() != new_note->timestamp());
        bool interval_changed =
            (get_interval(old_note) != get_interval(new_note));
        if (timestamp_changed) {
            auto& old_notes_at_ts = m_timeline.at(old_note->timestamp());
            old_notes_at_ts.erase(std::remove(old_notes_at_ts.begin(),
                                              old_notes_at_ts.end(), handle),
                                  old_notes_at_ts.end());
            if (old_notes_at_ts.empty())
                m_timeline.erase(old_note->timestamp());
            m_timeline[new_note->timestamp()].push_back(handle);
        }
        if (interval_changed) {
            auto it = m_handle_to_interval_node.find(handle);
            if (it != m_handle_to_interval_node.end()) {
                m_interval_tree.remove(it->second);
                m_handle_to_interval_node.erase(it);
                auto* new_node_ptr =
                    m_interval_tree.insert(get_interval(new_note), handle);
                m_handle_to_interval_node[handle] = new_node_ptr;
            }
        }
        m_storage.get_mutable(handle) = std::move(new_note_data);
        return true;
    }

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
        // 这可能是要找的“大于等于”的最近者。
        auto it_after = m_timeline.lower_bound(timestamp);

        // 情况1：正好找到一个完全匹配的。
        if (it_after != m_timeline.end() && it_after->first == timestamp) {
            return it_after->second;  // 直接返回该时间戳下的所有音符句柄。
        }

        // 情况2：没有完全匹配的，需要在前一个和后一个之间选择。

        // 查找前一个元素 "it_before"
        auto it_before = it_after;
        if (it_before != m_timeline.begin()) {
            --it_before;  // it_after 不是 begin()，所以可以安全地递减。
        } else {
            // 如果 it_after 就是 begin()，说明所有元素都大于或等于 timestamp，
            // 那么最近的只能是 it_after 指向的。
            return it_after->second;
        }

        // 此时 it_after 指向 "大于" 的最近者，it_before 指向 "小于" 的最近者。

        // 如果 it_after 已经是 end()，说明所有元素都小于 timestamp，
        // 那么最近的只能是 it_before 指向的 (即最后一个元素)。
        if (it_after == m_timeline.end()) {
            return it_before->second;
        }

        // 比较两者与 timestamp 的距离
        int64_t dist_before = timestamp - it_before->first;
        int64_t dist_after = it_after->first - timestamp;

        if (dist_before < dist_after) {
            return it_before->second;
        } else if (dist_after < dist_before) {
            return it_after->second;
        } else {
            // 距离相等，一种合理的做法是返回两者的并集。
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

        // lower_bound 找到第一个不小于 timestamp 的元素。
        auto it = m_timeline.lower_bound(timestamp);

        // 我们要找的是严格小于的，所以需要看 lower_bound 找到的元素的前一个。
        if (it == m_timeline.begin()) {
            // 如果 lower_bound 返回的是 begin()，说明集合中没有元素小于
            // timestamp。
            return {};
        }

        // 安全地递减迭代器，it 现在指向的就是我们想要的结果。
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

        // upper_bound 找到第一个严格大于 timestamp 的元素。
        auto it = m_timeline.upper_bound(timestamp);

        if (it == m_timeline.end()) {
            // 如果 upper_bound 返回的是 end()，说明集合中没有元素大于
            // timestamp。
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
    std::unordered_map<NoteHandle, IntervalTree::Node*, NoteHandle::Hash>
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
     * @return 如果添加或更新成功，返回 true。
     * @note 如果该时间戳已存在一个时间点，它将被新的时间点覆盖。
     */
    bool set_timing_point(const Timing& timing) {
        if (timing.timestamp < 0) {
            return false;  // 无效时间戳
        }
        // 直接使用 map 的下标运算符，如果不存在则创建，存在则覆盖。
        m_timeline[timing.timestamp] = timing;
        return true;
    }

    /**
     * @brief 移除一个指定时间戳的时间点。
     * @param timestamp 要移除的时间点的时间戳。
     * @return 如果找到了并成功移除，返回 true。
     */
    bool remove_timing_point(int32_t timestamp) {
        // map::erase(key) 返回被删除的元素数量（0或1）。
        return m_timeline.erase(timestamp) > 0;
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
        return &(it->second);
    }

    /**
     * @brief 获取指定时间戳的精确时间点（如果存在）。
     * @param timestamp 目标时间戳。
     * @return 指向 Timing 对象的 const 指针。如果该时间戳没有定义时间点，则返回
     * nullptr。
     */
    const Timing* get_timing_point_at(int32_t timestamp) const {
        if (auto it = m_timeline.find(timestamp); it != m_timeline.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    /**
     * @brief 获取所有时间点的只读引用，按时间戳排序。
     * @return 一个对内部 map 的 const 引用。
     */
    const std::map<int32_t, Timing>& get_all_timing_points() const {
        return m_timeline;
    }

    /**
     * @brief 清空所有时间点。
     */
    void clear() { m_timeline.clear(); }

   private:
    // 使用 std::map 作为核心存储。Key 是时间戳，Value 是 Timing 对象。
    // std::map 自动按 Key 排序，并提供高效的对数时间复杂度查找。
    std::map<int32_t, Timing> m_timeline;
};

#endif  // MMM_DATASTRUCTURES_HPP
