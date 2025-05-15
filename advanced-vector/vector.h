#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:

    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {
        Deallocate(buffer_);
        buffer_ = std::exchange(other.buffer_, nullptr);
        capacity_ = std::exchange(other.capacity_, 0);
    }
    
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        buffer_ = std::exchange(rhs.buffer_, nullptr);
        capacity_ = std::exchange(rhs.capacity_, 0);            
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:

    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept {
        data_ = std::move(other.data_);
        size_ = std::exchange(other.size_, 0);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.Size() > Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {                
                size_t min_size = std::min(rhs.Size(), Size());
                std::copy_n(rhs.begin(), min_size, begin());
                if (rhs.Size() == min_size) {
                    std::destroy_n(begin() + rhs.Size(), Size() - rhs.Size());
                } else {
                    std::uninitialized_copy_n(rhs.begin() + Size(), rhs.Size() - Size(), begin() + Size()); 
                }
                size_ = rhs.Size();
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        data_ = std::move(rhs.data_);
        size_ = std::exchange(rhs.size_, 0);
        return *this;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < Size());
        return data_[index];
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(begin(), Size(), new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(begin(), Size(), new_data.GetAddress());
        }

        std::destroy_n(begin(), Size());
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size > Size()) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + Size(), new_size - Size());
        } else {
            std::destroy_n(data_.GetAddress() + new_size, Size() - new_size);
        }
        size_ = new_size;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        return *(Emplace(cend(), std::forward<Args>(args)...));
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() {
        if (Size() > 0) {
            std::destroy_at(end() - 1);
            --size_;
        }
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(begin() <= pos && pos <= end());
        size_t index = std::distance(begin(), iterator(pos));

        if (Size() == Capacity()) {
            EmplaceWithDataRelocation(pos, index, std::forward<Args>(args)...);
        } else {
            EmplaceWithoutDataRelocation(pos, index, std::forward<Args>(args)...);
        }
        
        ++size_;
        return begin() + index;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    iterator Erase(const_iterator pos) {
        assert(begin() <= pos && pos <= end());
        size_t index = std::distance(begin(), iterator(pos));
        std::move(std::make_move_iterator(iterator(pos) + 1), std::make_move_iterator(end()), iterator(pos));
        std::destroy_at(end() - 1);
        --size_;
        return begin() + index;
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    template <typename... Args>
    void EmplaceWithDataRelocation(const_iterator pos, size_t index, Args&&... args) {
        RawMemory<T> new_data(Size() == 0 ? 1 : Size() * 2);
        new (new_data + index) T(std::forward<Args>(args)...);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(begin(), index, new_data.GetAddress());
            std::uninitialized_move_n(iterator(pos), Size() - index, new_data.GetAddress() + index + 1);
        } else {
            std::uninitialized_copy_n(begin(), index, new_data.GetAddress());
            std::uninitialized_copy_n(iterator(pos), Size() - index, new_data.GetAddress() + index + 1);
        }

        std::destroy_n(begin(), Size());
        data_.Swap(new_data);
    }

    template <typename... Args>
    void EmplaceWithoutDataRelocation(const_iterator pos, size_t index, Args&&... args) {
        if (end() != pos) {
            T temp_obj(std::forward<Args>(args)...);
            new (data_ + size_) T(std::forward<T>(*(end() - 1)));
            std::move_backward(iterator(pos), end() - 1, end()); 
            data_[index] = std::forward<T>(temp_obj);
        } else {
            new (data_ + index) T(std::forward<Args>(args)...);
        }
    }

};
