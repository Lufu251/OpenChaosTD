#pragma once

#include <vector>
#include <stdexcept>
#include <string>

template<typename T>
class Grid2D {
public:
    Grid2D() = default;

    Grid2D(int width, int height, const T& fill = T{})
        : m_width(width)
        , m_height(height)
        , m_data(width * height, fill)
    {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument(
                "Grid2D: dimensions must be positive, got " +
                std::to_string(width) + "x" + std::to_string(height)
            );
    }

    // Resize — resets all cells to fill value
    void Resize(int width, int height, const T& fill = T{}) {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument(
                "Grid2D: dimensions must be positive, got " +
                std::to_string(width) + "x" + std::to_string(height)
            );
        m_width  = width;
        m_height = height;
        m_data.assign(width * height, fill);
    }

    // Access
    std::vector<T>& GetVector() {
        return m_data;
    }

    T& Get(int x, int y) {
        CheckBounds(x, y);
        return m_data[y * m_width + x];
    }

    const T& Get(int x, int y) const {
        CheckBounds(x, y);
        return m_data[y * m_width + x];
    }

    void Set(int x, int y, const T& value) {
        CheckBounds(x, y);
        m_data[y * m_width + x] = value;
    }

    // Query
    bool InBounds(int x, int y) const {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

    int GetWidth()  const { return m_width;  }
    int GetHeight() const { return m_height; }
    int GetSize()   const { return m_width * m_height; }

    // Fill entire grid
    void Fill(const T& value) {
        std::fill(m_data.begin(), m_data.end(), value);
    }

    // Iterators — allows range-based for loops
    typename std::vector<T>::iterator begin() { return m_data.begin(); }
    typename std::vector<T>::iterator end() { return m_data.end(); }
    typename std::vector<T>::const_iterator begin() const { return m_data.begin(); }
    typename std::vector<T>::const_iterator end() const { return m_data.end(); }

private:
    void CheckBounds(int x, int y) const {
        if (!InBounds(x, y))
            throw std::out_of_range(
                "Grid2D: (" + std::to_string(x) + ", " + std::to_string(y) +
                ") out of range for " + std::to_string(m_width) +
                "x" + std::to_string(m_height) + " grid"
            );
    }

    int m_width  = 0;
    int m_height = 0;
    std::vector<T> m_data;
};