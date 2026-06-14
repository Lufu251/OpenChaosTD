#pragma once

#include <vector>
#include <cassert>
#include <cstddef>

// Fixed-capacity pool with O(1) acquire and release.
//
// Active objects occupy a contiguous range [0, Size()) with no gaps — safe
// to iterate with a plain index loop or range-for. Release uses swap-and-pop:
// the released slot is overwritten by the last active element, so do NOT
// advance the loop index after a ReleaseAt call.
//
// Memory is allocated once in the constructor; Acquire/Release never allocate.
template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t capacity) : m_size(0) {
        m_objects.resize(capacity); // pre-allocate all slots upfront
    }

    // Returns a pointer to the next free slot, or nullptr if at capacity.
    // The caller is responsible for initialising the returned object.
    T* Acquire() {
        if (m_size >= m_objects.size()) return nullptr;
        return &m_objects[m_size++];
    }

    // O(1) swap-and-pop. Overwrites index with the last active element.
    // After this call the element formerly at the end is now at index —
    // do not increment the loop counter on the same iteration.
    void ReleaseAt(size_t index) {
        assert(index < m_size);
        m_objects[index] = std::move(m_objects[--m_size]);
    }

    // operator[]/ReleaseAt assert the index in debug only; under NDEBUG the assert is gone, so an
    // out-of-range index is undefined behaviour — callers must keep index < Size().
    T& operator[](size_t index) {
        assert(index < m_size);
        return m_objects[index];
    }

    const T& operator[](size_t index) const {
        assert(index < m_size);
        return m_objects[index];
    }

    // Iteration covers only active objects — no gaps, no dead slots.
    T* begin() { return m_objects.data(); }
    T* end()   { return m_objects.data() + m_size; }
    const T* begin() const { return m_objects.data(); }
    const T* end()   const { return m_objects.data() + m_size; }

    size_t Size()     const { return m_size; }

    // Marks all slots as free without touching their contents.
    void Clear() { m_size = 0; }

private:
    std::vector<T> m_objects; // pre-sized to capacity, never resized again
    size_t m_size = 0;        // number of active objects in [0, m_size)
};
