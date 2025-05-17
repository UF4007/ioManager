

template <typename T, std::size_t Capacity>
class inplace_vector {
public:
    // Type definitions
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors & Destructor
    constexpr inplace_vector() noexcept : size_(0) {}

    constexpr inplace_vector(size_type count, const T& value) : size_(0) {
        assign(count, value);
    }

    template <typename InputIt,
        typename = std::enable_if_t<!std::is_integral<InputIt>::value>>
        constexpr inplace_vector(InputIt first, InputIt last) : size_(0) {
        assign(first, last);
    }

    constexpr inplace_vector(std::initializer_list<T> init) : size_(0) {
        assign(init);
    }

    constexpr inplace_vector(const inplace_vector& other) : size_(0) {
        assign(other.begin(), other.end());
    }

    constexpr inplace_vector(inplace_vector&& other) noexcept : size_(0) {
        for (size_type i = 0; i < other.size_; ++i) {
            construct(&storage_[i], std::move(other.storage_[i]));
        }
        size_ = other.size_;
        other.size_ = 0;
    }

    constexpr inplace_vector& operator=(const inplace_vector& other) {
        if (this != &other) {
            clear();
            assign(other.begin(), other.end());
        }
        return *this;
    }

    constexpr inplace_vector& operator=(inplace_vector&& other) noexcept {
        if (this != &other) {
            clear();
            for (size_type i = 0; i < other.size_; ++i) {
                construct(&storage_[i], std::move(other.storage_[i]));
            }
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    constexpr inplace_vector& operator=(std::initializer_list<T> ilist) {
        clear();
        assign(ilist);
        return *this;
    }

    ~inplace_vector() {
        clear();
    }

    // Assign operations
    constexpr void assign(size_type count, const T& value) {
        clear();
        if (count > Capacity) {
            IO_THROW(std::length_error("inplace_vector::assign count exceeds capacity"))
        }
        for (size_type i = 0; i < count; ++i) {
            construct(&storage_[i], value);
        }
        size_ = count;
    }

    template <typename InputIt,
        typename = std::enable_if_t<!std::is_integral<InputIt>::value>>
        constexpr void assign(InputIt first, InputIt last) {
        clear();
        for (; first != last && size_ < Capacity; ++first) {
            construct(&storage_[size_], *first);
            ++size_;
        }
        if (first != last) {
            IO_THROW(std::length_error("inplace_vector::assign range exceeds capacity"))
        }
    }

    constexpr void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    // Element access
    constexpr reference at(size_type pos) {
        if (pos >= size_) {
            IO_THROW(std::out_of_range("inplace_vector::at: pos out of range"))
        }
        return storage_[pos];
    }

    constexpr const_reference at(size_type pos) const {
        if (pos >= size_) {
            IO_THROW(std::out_of_range("inplace_vector::at: pos out of range"))
        }
        return storage_[pos];
    }

    constexpr reference operator[](size_type pos) noexcept {
        return storage_[pos];
    }

    constexpr const_reference operator[](size_type pos) const noexcept {
        return storage_[pos];
    }

    constexpr reference front() noexcept {
        return storage_[0];
    }

    constexpr const_reference front() const noexcept {
        return storage_[0];
    }

    constexpr reference back() noexcept {
        return storage_[size_ - 1];
    }

    constexpr const_reference back() const noexcept {
        return storage_[size_ - 1];
    }

    constexpr T* data() noexcept {
        return reinterpret_cast<T*>(&storage_[0]);
    }

    constexpr const T* data() const noexcept {
        return reinterpret_cast<const T*>(&storage_[0]);
    }

    // Iterators
    constexpr iterator begin() noexcept {
        return iterator(&storage_[0]);
    }

    constexpr const_iterator begin() const noexcept {
        return const_iterator(&storage_[0]);
    }

    constexpr const_iterator cbegin() const noexcept {
        return const_iterator(&storage_[0]);
    }

    constexpr iterator end() noexcept {
        return iterator(&storage_[size_]);
    }

    constexpr const_iterator end() const noexcept {
        return const_iterator(&storage_[size_]);
    }

    constexpr const_iterator cend() const noexcept {
        return const_iterator(&storage_[size_]);
    }

    constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    constexpr const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    constexpr const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    // Capacity
    [[nodiscard]] constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    constexpr size_type size() const noexcept {
        return size_;
    }

    constexpr size_type max_size() const noexcept {
        return Capacity;
    }

    constexpr size_type capacity() const noexcept {
        return Capacity;
    }

    // Modifiers
    constexpr void clear() noexcept {
        for (size_type i = 0; i < size_; ++i) {
            destroy(&storage_[i]);
        }
        size_ = 0;
    }

    constexpr iterator insert(const_iterator pos, const T& value) {
        return insert_impl(pos, value);
    }

    constexpr iterator insert(const_iterator pos, T&& value) {
        return insert_impl(pos, std::move(value));
    }

    constexpr iterator insert(const_iterator pos, size_type count, const T& value) {
        difference_type index = pos - cbegin();
        if (size_ + count > Capacity) {
            IO_THROW(std::length_error("inplace_vector::insert count exceeds capacity"))
        }

        // Shift elements to make space
        for (difference_type i = size_ - 1; i >= index; --i) {
            construct(&storage_[i + count], std::move(storage_[i]));
            destroy(&storage_[i]);
        }

        // Insert new elements
        for (size_type i = 0; i < count; ++i) {
            construct(&storage_[index + i], value);
        }

        size_ += count;
        return iterator(&storage_[index]);
    }

    template <typename InputIt,
        typename = std::enable_if_t<!std::is_integral<InputIt>::value>>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
        difference_type index = pos - cbegin();
        size_type count = static_cast<size_type>(std::distance(first, last));

        if (size_ + count > Capacity) {
            IO_THROW(std::length_error("inplace_vector::insert range exceeds capacity"))
        }

        // Shift elements to make space
        for (difference_type i = size_ - 1; i >= index; --i) {
            construct(&storage_[i + count], std::move(storage_[i]));
            destroy(&storage_[i]);
        }

        // Insert new elements
        size_type i = 0;
        for (auto it = first; it != last; ++it, ++i) {
            construct(&storage_[index + i], *it);
        }

        size_ += count;
        return iterator(&storage_[index]);
    }

    constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename... Args>
    constexpr iterator emplace(const_iterator pos, Args&&... args) {
        difference_type index = pos - cbegin();

        if (size_ >= Capacity) {
            IO_THROW(std::length_error("inplace_vector::emplace capacity exceeded"))
        }

        // Shift elements to make space
        for (difference_type i = size_ - 1; i >= index; --i) {
            construct(&storage_[i + 1], std::move(storage_[i]));
            destroy(&storage_[i]);
        }

        // Construct new element
        construct(&storage_[index], std::forward<Args>(args)...);

        ++size_;
        return iterator(&storage_[index]);
    }

    constexpr iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

    constexpr iterator erase(const_iterator first, const_iterator last) {
        difference_type start_idx = first - cbegin();
        difference_type count = last - first;

        if (count <= 0) {
            return iterator(&storage_[start_idx]);
        }

        // Destroy erased elements
        for (difference_type i = 0; i < count; ++i) {
            destroy(&storage_[start_idx + i]);
        }

        // Move elements down
        for (size_type i = start_idx; i + count < size_; ++i) {
            construct(&storage_[i], std::move(storage_[i + count]));
            destroy(&storage_[i + count]);
        }

        size_ -= count;
        return iterator(&storage_[start_idx]);
    }

    constexpr void push_back(const T& value) {
        if (size_ >= Capacity) {
            IO_THROW(std::length_error("inplace_vector::push_back capacity exceeded"))
        }
        construct(&storage_[size_], value);
        ++size_;
    }

    constexpr void push_back(T&& value) {
        if (size_ >= Capacity) {
            IO_THROW(std::length_error("inplace_vector::push_back capacity exceeded"))
        }
        construct(&storage_[size_], std::move(value));
        ++size_;
    }

    template <typename... Args>
    constexpr reference emplace_back(Args&&... args) {
        if (size_ >= Capacity) {
            IO_THROW(std::length_error("inplace_vector::emplace_back capacity exceeded"))
        }
        construct(&storage_[size_], std::forward<Args>(args)...);
        ++size_;
        return storage_[size_ - 1];
    }

    constexpr void pop_back() {
        if (size_ > 0) {
            destroy(&storage_[--size_]);
        }
    }

    constexpr void resize(size_type new_size) {
        resize(new_size, T());
    }

    constexpr void resize(size_type new_size, const value_type& value) {
        if (new_size > Capacity) {
            IO_THROW(std::length_error("inplace_vector::resize capacity exceeded"))
        }

        if (new_size > size_) {
            // Grow
            for (size_type i = size_; i < new_size; ++i) {
                construct(&storage_[i], value);
            }
        }
        else if (new_size < size_) {
            // Shrink
            for (size_type i = new_size; i < size_; ++i) {
                destroy(&storage_[i]);
            }
        }

        size_ = new_size;
    }

    constexpr void swap(inplace_vector& other) noexcept {
        inplace_vector temp = std::move(*this);
        *this = std::move(other);
        other = std::move(temp);
    }

private:
    // Storage using aligned storage
    using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    StorageType storage_[Capacity];
    size_type size_;

    // Helper functions for object lifetime management
    template <typename... Args>
    static constexpr void construct(void* ptr, Args&&... args) {
        new (ptr) T(std::forward<Args>(args)...);
    }

    static constexpr void destroy(void* ptr) {
        static_cast<T*>(ptr)->~T();
    }

    template <typename U>
    constexpr iterator insert_impl(const_iterator pos, U&& value) {
        difference_type index = pos - cbegin();

        if (size_ >= Capacity) {
            IO_THROW(std::length_error("inplace_vector::insert capacity exceeded"))
        }

        // Shift elements to make space
        for (difference_type i = size_ - 1; i >= index; --i) {
            construct(&storage_[i + 1], std::move(storage_[i]));
            destroy(&storage_[i]);
        }

        // Insert new element
        construct(&storage_[index], std::forward<U>(value));

        ++size_;
        return iterator(&storage_[index]);
    }
};

// Swap specialization for ADL
template <typename T, std::size_t Capacity>
constexpr void swap(inplace_vector<T, Capacity>& lhs, inplace_vector<T, Capacity>& rhs) noexcept {
    lhs.swap(rhs);
}

// Comparison operators
template <typename T, std::size_t Capacity>
constexpr bool operator==(const inplace_vector<T, Capacity>& lhs, const inplace_vector<T, Capacity>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, std::size_t Capacity>
constexpr bool operator!=(const inplace_vector<T, Capacity>& lhs, const inplace_vector<T, Capacity>& rhs) {
    return !(lhs == rhs);
}

template <typename T, std::size_t Capacity>
constexpr bool operator<(const inplace_vector<T, Capacity>& lhs, const inplace_vector<T, Capacity>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, std::size_t Capacity>
constexpr bool operator<=(const inplace_vector<T, Capacity>& lhs, const inplace_vector<T, Capacity>& rhs) {
    return !(rhs < lhs);
}

template <typename T, std::size_t Capacity>
constexpr bool operator>(const inplace_vector<T, Capacity>& lhs, const inplace_vector<T, Capacity>& rhs) {
    return rhs < lhs;
}

template <typename T, std::size_t Capacity>
constexpr bool operator>=(const inplace_vector<T, Capacity>& lhs, const inplace_vector<T, Capacity>& rhs) {
    return !(lhs < rhs);
}