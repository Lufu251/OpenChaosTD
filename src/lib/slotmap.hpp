#pragma once

#include <vector>
#include <cassert>
#include <cstdint>

template<typename T>
class SlotMap {
public:
    struct Key {
        uint32_t index;
        uint32_t generation;

        bool operator==(const Key&) const = default;
    };

    static constexpr Key INVALID_KEY = { UINT32_MAX, 0 };

private:
    struct Slot {
        T value;
        uint32_t generation = 0;
        bool occupied = false;
    };

    std::vector<Slot> m_slots;
    std::vector<uint32_t> m_freeList;

    // Validity check
    bool IsKeyValid(Key key) const {
        if (key.index >= m_slots.size()) return false;
        const Slot& slot = m_slots[key.index];
        return slot.occupied && slot.generation == key.generation;
    }

public:
    // Insert
    Key Insert(T value) {
        uint32_t idx;

        if (!m_freeList.empty()) {
            // Existing free slot available
            idx = m_freeList.back(); // Get the id of a free slot
            m_freeList.pop_back(); // Remove the reused id
        } else {
            // No Existing free slot available
            idx = static_cast<uint32_t>(m_slots.size()); // Get the id for the new slot at the back
            m_slots.emplace_back(); // Create the new slot with the id
        }

        Slot& slot = m_slots[idx];
        slot.value = std::move(value);
        slot.occupied = true;
        // generation was already incremented on erase

        return { idx, slot.generation };
    }

    // Remove
    bool Erase(Key key) {
        if (!IsKeyValid(key)) return false;

        Slot& slot = m_slots[key.index];
        slot.occupied = false;
        slot.generation++; // invalidates all existing keys
        m_freeList.push_back(key.index);
        return true;
    }

    // Lookup
    T* Get(Key key) {
        if (!IsKeyValid(key)) return nullptr;
        return &m_slots[key.index].value;
    }

    const T* Get(Key key) const {
        if (!IsKeyValid(key)) return nullptr;
        return &m_slots[key.index].value;
    }

    size_t Size() const {
        // Number of live (occupied) elements
        return m_slots.size() - m_freeList.size();
    }

    // Clears everything
    void Clear(){
        m_slots.clear();
        m_freeList.clear();
    }
};