#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <vector>
#include <algorithm>
#include <iostream> // For std::cout in the print method, replace with appropriate logging if needed

/**
 * @brief A template class that implements a fixed-size buffer which maintains elements in a heap.
 *
 * The RingBuffer class is designed to manage a collection of elements in a fixed-size container.
 * It automatically maintains the elements in a heap structure, ensuring that they are ordered
 * according to a comparator. The class provides methods for adding and removing elements, accessing
 * the buffer contents, and clearing the buffer.
 *
 * @tparam T The type of elements stored in the buffer.
 * @tparam Comparator A comparator class used to maintain the order of the elements in the heap.
 *                    The comparator should provide a boolean operator() that takes two elements of
 *                    type T and returns true if the first element is considered less than the
 *                    second element. The default is std::less<T>, which orders elements in ascending
 *                    order.
 */
template <typename T, typename Comparator = std::less<T>>
class RingBuffer
{
private:
    std::vector<T> buffer; ///< Stores the elements in the heap.
    size_t capacity; ///< The maximum number of elements the buffer can hold.
    Comparator comp; ///< The comparator used to order the elements in the buffer.

public:
    explicit RingBuffer(size_t cap);
    ~RingBuffer();

    void add(T item);
    const std::vector<T> &getBuffer() const;
    void clear();
    void remove(const T &item);
    void print() const;
};

#include "RingBuffer.tpp" // Implementation of template methods

#endif // RING_BUFFER_HPP
