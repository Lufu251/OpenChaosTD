#pragma once

#include <vector>
#include <cassert>
#include <cstdint>

template<typename T>
class DenseSlotMap {
public:
    struct Key {
        uint32_t index;
        uint32_t generation;

        bool operator==(const Key&) const = default;
    };

    static constexpr Key INVALID_KEY = { UINT32_MAX, 0 };

private:
    struct Slot {
        uint32_t generation  = 0;
        uint32_t dense_index = 0;  // where in m_values this element lives
        bool occupied    = false;
    };

    std::vector<Slot> m_slots; // sparse — indexed by Key.index
    std::vector<T> m_values; // dense — iterate this for rendering/updates
    std::vector<uint32_t> m_erase; // dense — maps m_values[i] back to its slot index
    std::vector<uint32_t> m_freeList; // recycled slot indices

    // Validity check
    bool IsKeyValid(Key key) const {
        if (key.index >= m_slots.size()) return false;
        const Slot& slot = m_slots[key.index];
        return slot.occupied && slot.generation == key.generation;
    }

public:
    // Insert — O(1) amortised
    Key Insert(T value) {
        uint32_t slotIdx;

        if (!m_freeList.empty()) {
            slotIdx = m_freeList.back();
            m_freeList.pop_back();
        } else {
            slotIdx = static_cast<uint32_t>(m_slots.size());
            m_slots.emplace_back();
        }

        // The new value goes at the back of the dense array
        const uint32_t denseIdx = static_cast<uint32_t>(m_values.size());

        m_values.push_back(std::move(value));
        m_erase.push_back(slotIdx);          // dense position → slot index

        Slot& slot        = m_slots[slotIdx];
        slot.dense_index  = denseIdx;
        slot.occupied     = true;
        // generation was already incremented on erase

        return { slotIdx, slot.generation };
    }

    // Erase — O(1) via swap-and-pop
    bool Erase(Key key) {
        if (!IsKeyValid(key)) return false;

        Slot& slot = m_slots[key.index];
        const uint32_t denseIdx = slot.dense_index;
        const uint32_t lastIdx  = static_cast<uint32_t>(m_values.size()) - 1;

        // Swap the erased element with the last element in the dense array
        if (denseIdx != lastIdx) {
            // Move last value into the erased slot
            m_values[denseIdx] = std::move(m_values[lastIdx]);
            m_erase[denseIdx]  = m_erase[lastIdx];

            // Fix the moved element's slot so it points to its new dense position
            m_slots[ m_erase[denseIdx] ].dense_index = denseIdx;
        }

        m_values.pop_back();
        m_erase.pop_back();

        // Invalidate the erased slot
        slot.occupied = false;
        slot.generation++;           // invalidates all existing keys to this slot
        m_freeList.push_back(key.index);

        return true;
    }

    // Lookup — O(1)
    T* Get(Key key) {
        if (!IsKeyValid(key)) return nullptr;
        return &m_values[ m_slots[key.index].dense_index ];
    }

    const T* Get(Key key) const {
        if (!IsKeyValid(key)) return nullptr;
        return &m_values[ m_slots[key.index].dense_index ];
    }

    // Iteration — zero overhead, no gaps, perfect for render/update loops
    T* begin() { return m_values.data(); }
    T* end() { return m_values.data() + m_values.size(); }
    const T* begin() const { return m_values.data(); }
    const T* end() const { return m_values.data() + m_values.size(); }

    size_t Size() const { return m_values.size(); }

    // Clears everything
    void Clear() {
        m_slots.clear();
        m_freeList.clear();
        m_values.clear();
        m_erase.clear();
    }
};