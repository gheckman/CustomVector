#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>

template <typename T>
class custom_vector
{
public:
    using iterator_t = T*;
    using const_iterator_t = const iterator_t;

    /** Default constructor */
    custom_vector() : begin_(nullptr), end_(nullptr), tail_(nullptr) {}

    /** Constructor which allocates memory
        @note No objects are constructed except the vector itself.
        @param[in] capacity Amount of objects which the vector could potentially hold. */
    custom_vector(size_t capacity)
    {
        begin_ = new data_t[capacity];
        end_ = begin_;
        tail_ = begin_ + capacity;
    }

    /** Copy constructor */
    custom_vector(const custom_vector& a) : custom_vector(a.capacity())
    {
        std::uninitialized_copy(a.cbegin(), a.cend(), as_t(begin_));
        end_ = begin_ + a.size();
    }

    /** Copy assignment operator */
    custom_vector& operator=(custom_vector a)
    {
        swap(*this, a);
        return *this;
    }

    /** Move constructor */
    custom_vector(custom_vector&& a) noexcept : custom_vector()
    {
        swap(*this, a);
    }

    /** Destructor */
    ~custom_vector()
    {
        clear();
    }

    /** Swap function
        Performs a lightweight swap of two objects for general use or for the copy-swap idiom
        @param[in, out] lhs Left hand side of the swap
        @param[in, out] rhs Right hand side of the swap */
    friend void swap(custom_vector& lhs, custom_vector& rhs)
    {
        using std::swap;
        swap(lhs.begin_, rhs.begin_);
        swap(lhs.end_, rhs.end_);
        swap(lhs.tail_, rhs.tail_);
    }

    /** Indexing operator
        @param[in] index Offset into the vector
        @return The object at the index given */
    T& operator[] (size_t index) const
    {
        return *as_t(begin_ + index);
    }

    /** Adds an object to the vector, allocating memory as needed.
        @param[in] t Generic object to add to the vector */
    void push_back(const T& t)
    {
        scale_if_required();
        new (as_t(end_++)) T(t);
    }

    /** Emplaces an object to the vector, allocating memory as needed.
        @note This avoids copying temporary objects and is generally more efficient than push_back
        @param[in] t Generic object to add to the vector */
    template <typename... Args>
    void emplace_back(Args&&... args)
    {
        scale_if_required();
        new (as_t(end_++)) T{ std::forward<Args>(args)... };
    }

    /** Destructs all objects and deallocates memory */
    void clear()
    {
        std::destroy(as_t(begin_), as_t(end_));
        delete[] begin_;
        begin_ = nullptr;
        end_ = nullptr;
        tail_ = nullptr;
    }

    /** See return
        @return Amount of objects stored by the vector */
    size_t size() const
    {
        return (begin_ && end_) ? std::distance(begin_, end_) : 0;
    }

    /** See return
        @return Potential amount of objects the vector may store */
    size_t capacity() const
    {
        return (begin_ && tail_) ? std::distance(begin_, tail_) : 0;
    }

    /** See return
        @return True if the vector currently has at least 1 object stored */
    bool empty() const
    {
        return size() == 0;
    }

    /** Increases capacity if the new capacity is greater than the current. Never reduces capacity.
        @param[in] new_cap New capacity for the vector */
    void reserve(size_t new_cap)
    {
        if (new_cap > capacity())
        {
            reallocate(new_cap);
        }
    }

    /** See return
        @return Returns the iterator to the first item in the vector */
    iterator_t begin() const
    {
        return as_t(begin_);
    }

    /** See return
        @return Returns the iterator to 1 past the last item in the vector */
    iterator_t end() const
    {
        return as_t(end_);
    }

    /** See return
        @return Returns the constant iterator to the first item in the vector */
    const_iterator_t cbegin() const
    {
        return as_t(begin_);
    }

    /** See return
        @return Returns the constant iterator to 1 past the last item in the vector */
    const_iterator_t cend() const
    {
        return as_t(end_);
    }

private:
    using data_t = std::aligned_storage_t<sizeof(T), alignof(T)>;

    data_t* begin_;
    data_t* end_;
    data_t* tail_;

    /** Gets a new capacity based on the current capacity and scale factor. Always increases by at least 1.
        @return The new scaled capacity */
    size_t get_new_scaled_capacity() const
    {
        auto current_cap = capacity();
        return std::max((size_t)(current_cap * scale_factor()), current_cap + 1);
    }

    /** See return
        @return True if the vector is at capacity and cannot hold even 1 more item */
    bool full()
    {
        return size() == capacity();
    }

    /** Scales the vector if the vector is full */
    void scale_if_required()
    {
        if (full())
        {
            reserve(get_new_scaled_capacity());
        }
    }

    /** Obtains new memory and moves (or copies) old data to the new memory block. Deletes old data and deallocates memory.
        @param[in] new_cap The new capacity for the vector */
    void reallocate(size_t new_cap)
    {
        auto old_size = size();
        auto old_begin = begin_;
        begin_ = new data_t[new_cap];

        // if there are any elements in the vector, they must be moved (or copied)
        if (!empty())
        {
            // TODO: the stl actually does 3 checks: nothrow_move, copy, move
            // Does a compile-time check to see is moving is possible. If not, then copy.
            if constexpr (std::is_move_constructible_v<T>)
            {
                std::uninitialized_move(as_t(old_begin), as_t(end_), as_t(begin_));
            }
            else
            {
                std::uninitialized_copy(as_t(old_begin), as_t(end_), as_t(begin_));
            }
            std::destroy(as_t(old_begin), as_t(end_));
        }

        delete[] old_begin;
        end_ = begin_ + old_size;
        tail_ = begin_ + new_cap;
    }

    /** Launders the raw memory pointer into an object pointer.
        @param[in] pointer to a block of raw memory
        @return A safe to use pointer to object memory */
    T* as_t(data_t* p) const
    {
        return std::launder(reinterpret_cast<T*>(p));
    }

    /** See return
        @return The constant scale factory by which the vector should expand when full */
    static const float& scale_factor()
    {
        static const float scale_factor = 1.5;
        return scale_factor;
    }
};
