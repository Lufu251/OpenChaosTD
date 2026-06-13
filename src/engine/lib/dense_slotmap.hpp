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

    // Public so external (non-intrusive) serializers can persist and restore the exact
    // sparse bookkeeping needed for key/handle stability. The map itself stays IO-agnostic.
    struct Slot {
        uint32_t generation = 0;
        uint32_t dense_index = 0;  // where in m_values this element lives
        bool occupied = false;
    };

private:
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
        // Accepted limit: generation is a uint32_t and is not saturated, so after 2^32 erase/reuse
        // cycles on the same slot it wraps and a long-dead key could alias the reused slot. This is
        // unreachable in practice (billions of reuses of one slot); document rather than guard.
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

    Key KeyOf(const T* it) const {
        assert(it >= m_values.data() && it < m_values.data() + m_values.size());
        const uint32_t denseIdx = static_cast<uint32_t>(it - m_values.data());
        const uint32_t slotIdx  = m_erase[denseIdx];
        return { slotIdx, m_slots[slotIdx].generation };
    }

    // Iteration — zero overhead, no gaps, perfect for render/update loops
    T* begin() { return m_values.data(); }
    T* end() { return m_values.data() + m_values.size(); }
    const T* begin() const { return m_values.data(); }
    const T* end() const { return m_values.data() + m_values.size(); }

    size_t Size() const { return m_values.size(); }

    // --- Serialization support (generic; no IO/json dependency here) ---
    // Raw access to the internal state. An external serializer reads these to persist the
    // container and calls RawAssign to restore it, preserving exact key/handle stability.
    const std::vector<Slot>&     RawSlots()    const { return m_slots; }
    const std::vector<T>&        RawValues()   const { return m_values; }
    const std::vector<uint32_t>& RawErase()    const { return m_erase; }
    const std::vector<uint32_t>& RawFreeList() const { return m_freeList; }

    // Restore exact internal state (deserialization only). The caller guarantees the four
    // vectors are mutually consistent (occupied slots point at valid dense indices, etc.).
    void RawAssign(std::vector<Slot> slots, std::vector<T> values,
                   std::vector<uint32_t> erase, std::vector<uint32_t> freeList) {
        m_slots    = std::move(slots);
        m_values   = std::move(values);
        m_erase    = std::move(erase);
        m_freeList = std::move(freeList);
    }

    // Clears everything
    void Clear() {
        m_slots.clear();
        m_freeList.clear();
        m_values.clear();
        m_erase.clear();
    }
};