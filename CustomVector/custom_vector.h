#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>

template <typename T, class Allocator = std::allocator<T>>
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
    custom_vector(size_t capacity) : custom_vector()
    {
        reallocate(capacity);
    }

    /** Constructor which allocates memory and copies objects.
        @param[in] capacity Amount of objects which the vector could potentially hold. */
    custom_vector(size_t capacity, const T& t) : custom_vector(capacity)
    {
        for (auto it = begin_; it != tail_; ++it)
        {
            new (as_t(it)) T{ t };
        }
        end_ = tail_;
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
        swap(a);
        return *this;
    }

    /** Move constructor */
    custom_vector(custom_vector&& a) noexcept : custom_vector()
    {
        swap(a);
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
    void swap(custom_vector& a) noexcept
    {
        using std::swap;
        swap(begin_, a.begin_);
        swap(end_, a.end_);
        swap(tail_, a.tail_);
    }

    /** Swap function
        Performs a lightweight swap of two objects for general use or for the copy-swap idiom
        @note Friend version for generic support
        @param[in, out] lhs Left hand side of the swap
        @param[in, out] rhs Right hand side of the swap */
    friend void swap(custom_vector& lhs, custom_vector& rhs) noexcept
    {
        lhs.swap(rhs);
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
        new (as_t(end_++)) T{ t };
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
    void clear() noexcept
    {
        std::destroy(as_t(begin_), as_t(end_));
        delete[] begin_;
        begin_ = nullptr;
        end_ = nullptr;
        tail_ = nullptr;
    }

    /** See return
        @return Amount of objects stored by the vector */
    size_t size() const noexcept
    {
        return (begin_ && end_) ? std::distance(begin_, end_) : 0;
    }

    /** See return
        @return Potential amount of objects the vector may store */
    size_t capacity() const noexcept
    {
        return (begin_ && tail_) ? std::distance(begin_, tail_) : 0;
    }

    /** See return
        @return True if the vector currently has at least 1 object stored */
    bool empty() const noexcept
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
        @return Returns a pointer to the first item in the vector */
    T* data() const noexcept
    {
        return as_t(begin_);
    }

    /** See return
        @return Returns the iterator to the first item in the vector */
    iterator_t begin() const noexcept
    {
        return as_t(begin_);
    }

    /** See return
        @return Returns the iterator to 1 past the last item in the vector */
    iterator_t end() const noexcept
    {
        return as_t(end_);
    }

    /** See return
        @return Returns the constant iterator to the first item in the vector */
    const_iterator_t cbegin() const noexcept
    {
        return as_t(begin_);
    }

    /** See return
        @return Returns the constant iterator to 1 past the last item in the vector */
    const_iterator_t cend() const noexcept
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
    size_t get_new_scaled_capacity() const noexcept
    {
        auto current_cap = capacity();
        return std::max((size_t)(current_cap * scale_factor()), current_cap + 1);
    }

    /** See return
        @return True if the vector is at capacity and cannot hold even 1 more item */
    bool full() const noexcept
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

        // Allocate new memory. If it fails, reset the begin pointer and return.
        begin_ = new (std::nothrow) data_t[new_cap];
        if (!begin_)
        {
            begin_ = old_begin;
            return;
        }

        // if there are any elements in the vector, they must be moved/copied
        if (!empty())
        {
            // Try to move/copy the objects. If it throws an exception (presumedly because a constructor threw it)
            // destroy the new objects, delete the new memory, reset the begin_ pointer, and return
            auto new_it = begin_;
            try
            {
                for (auto old_it = old_begin; old_it != end_; ++old_it, ++new_it)
                {
                    new (as_t(new_it)) T{ std::move_if_noexcept(*as_t(old_it)) };
                }
            }
            catch (std::exception)
            {
                std::destroy(as_t(begin_), as_t(new_it));
                delete[] begin_;
                begin_ = old_begin;
                return;
            }

            // The old objects must now be destroyed before the memory they occupy can be freed.
            std::destroy(as_t(old_begin), as_t(end_));
        }

        delete[] old_begin;
        end_ = begin_ + old_size;
        tail_ = begin_ + new_cap;
    }

    /** Launders the raw memory pointer into an object pointer.
        @param[in] pointer to a block of raw memory
        @return A safe to use pointer to object memory */
    T* as_t(data_t* p) const noexcept
    {
        return std::launder(reinterpret_cast<T*>(p));
    }

    /** See return
        @return The constant scale factory by which the vector should expand when full */
    static const float& scale_factor() noexcept
    {
        static const float scale_factor = 1.5;
        return scale_factor;
    }
};
