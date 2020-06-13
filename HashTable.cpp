// Copyright 2020: Katunkin Mikhail
// mishanewyear@mail.ru

#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <list>
#include <stdexcept>

// HashMap is a class that implements hash table data structure.
// HashMap is using separate chaining collision resolution method.
// More about hash tables here:
// https://neerc.ifmo.ru/wiki/index.php?title=Хеш-таблица
// https://neerc.ifmo.ru/wiki/index.php?title=Разрешение_коллизий

// When hash table is halfway filled, its capacity is doubled.
// When hash table's size is less than 1/8 of its capacity, the capacity is
// reduced by half.
// Pairs key-value are contained in std::list for ease of iteration.
// The hash table contains iterators of std::list.

template <typename KeyType, typename ValueType, class Hash = std::hash<KeyType>>
class HashMap {
private:
    using ListIter =
        typename std::list<std::pair<const KeyType, ValueType>>::iterator;
    using Element = std::pair<const KeyType, ValueType>;

    size_t capacity_ = kDefaultCapacity;
    size_t size_ = 0;
    Hash hash_;

    std::vector<std::vector<ListIter>> hash_table_;
    std::list<Element> elements_;

    // Gets hash and returns indexes of position in hash table where to look for
    // element.
    // If there is no such element in hash table - second index is -1.
    // Works for expected O(1)
    std::pair<size_t, int> get_position(const KeyType& key, const size_t& raw_hash) const {
        size_t hash = raw_hash % capacity_;

        int ind = -1;
        for (int i = 0; i < static_cast<int>(hash_table_[hash].size()); i++) {
            if (hash_table_[hash][i]->first == key) {
                ind = i;
                break;
            }
        }
        return { hash, ind };
    }

    // Rebuilds hash table with given capacity.
    // Works for O(max(old capacity, new capacity)).
    void rebuild(size_t new_capacity) {
        hash_table_.assign(new_capacity, std::vector<ListIter>());
        capacity_ = new_capacity;

        for (auto it = elements_.begin(); it != elements_.end(); it++) {
            size_t h = hash_(it->first) % capacity_;
            hash_table_[h].push_back(it);
        }
    }

    // Rebuilds hash table if necessary.
    void rebuild_if_needed() {
        if (size_ >= kPersentageOfCapacityToIncrease * capacity_) {
            rebuild(capacity_ * kIncreasingFactor);
        } else if (size_ <= kPersentageOfCapacityToDecrease * capacity_
                   && capacity_ > kDefaultCapacity) {
            rebuild(capacity_ / kDecreasingFactor);
        }
    }

    // Adds element to hash table and rebuilds it, if necessary.
    // Works for expected O(1) for adding and O(size) for rebuilding.
    size_t add_element(const Element& element) {
        size_t raw_hash = hash_(element.first);
        auto p = get_position(element.first, raw_hash);
        size_t hash = p.first;
        int ind = p.second;

        if (ind == -1) {
            elements_.push_back(element);
            auto it = elements_.end();
            it--;
            hash_table_[hash].push_back(it);
            size_++;
        }

        rebuild_if_needed();
        return raw_hash;
    }

    // Deletes element from hash table and rebuilds it, if necessary.
    // Works for expected O(1) for deleting and O(size) for rebuilding.
    void delete_element(const KeyType& key) {
        size_t raw_hash = hash_(key);
        auto p = get_position(key, raw_hash);
        size_t hash = p.first;
        int ind = p.second;

        if (ind != -1) {
            std::swap(hash_table_[hash][ind], hash_table_[hash].back());
            elements_.erase(hash_table_[hash].back());
            hash_table_[hash].pop_back();
            size_--;
        }

        rebuild_if_needed();
    }


public:
    static constexpr size_t kDefaultCapacity = 1;
    static constexpr double kPersentageOfCapacityToDecrease = 1.0 / 8.0;
    static constexpr double kPersentageOfCapacityToIncrease = 1.0 / 2.0;
    static constexpr double kDecreasingFactor = 2.0;
    static constexpr double kIncreasingFactor = 2.0;

    // Creates hash table with gived hasher.
    // Works for O(default capacity).
    explicit HashMap(Hash hash = Hash{}) : hash_(hash) {
        hash_table_.resize(capacity_);
    }

    // Creates hash table with given hasher.
    // Hash table is filled with elements in range given by iterators.
    // Works for O(size).
    template <class Iter>
    HashMap(Iter begin, Iter end, Hash hash = Hash{})
        : hash_(hash) {
        hash_table_.assign(kDefaultCapacity, std::vector<ListIter>());
        while (begin != end) {
            add_element(*begin);
            begin++;
        }
    }

    // Creates hash table with given hasher.
    // Hash table is filled with elements in initializer list.
    // Works for O(size).
    HashMap(std::initializer_list<Element> init_list, Hash hash = Hash{})
        : hash_(hash) {
        hash_table_.assign(kDefaultCapacity, std::vector<ListIter>());
        auto it = init_list.begin();
        while (it != init_list.end()) {
            add_element(*it);
            it++;
        }
    }

    // Copy constructor.
    // Works for O(capacity).
    HashMap(const HashMap& other)
        : size_(other.size_), hash_(other.hash_), elements_(other.elements_)
    {
        rebuild(other.capacity_);
    }

    // Move constructor.
    // Works for expected O(1).
    HashMap(HashMap&& other)
        : capacity_(other.capacity_),
        size_(other.size_),
        hash_(std::move(other.hash_)),
        hash_table_(std::move(other.hash_table_)),
        elements_(std::move(other.elements_)) 
    {}

    // Copy assignment operator.
    // Works for O(capacity).
    HashMap& operator=(const HashMap& other) {
        hash_ = other.hash_;
        elements_ = std::list<Element>(other.elements_);
        size_ = other.size_;
        rebuild(other.capacity_);
        return *this;
    }

    // Move assignment operator.
    // Works for expected O(1).
    HashMap& operator=(HashMap&& other) {
        hash_ = std::move(other.hash_);
        elements_ = std::list<Element>(std::move(other.elements_));
        size_ = other.size_;
        capacity_ = other.capacity_;
        hash_table_ = std::move(other.hash_table_);
        return *this;
    }

    // Returns size of hash table.
    size_t size() const { return size_; }

    // Returns is hash table empty or not.
    bool empty() const { return size_ == 0; }

    // Returns hasher used in hash table.
    Hash hash_function() const { return hash_; }

    // Adds element to hash table.
    // If hash table already contains such element - does nothing.
    // Works for expected O(1).
    void insert(const Element& el) { add_element(el); }

    // Deletes element from hash table.
    // If hash table doesn't contain such element - does nothing.
    // Works for expected O(1).
    void erase(const KeyType& el) { delete_element(el); }

    using iterator =
        typename std::list<std::pair<const KeyType, ValueType>>::iterator;
    using const_iterator =
        typename std::list<std::pair<const KeyType, ValueType>>::const_iterator;

    // Returns iterator on first element of hash table.
    iterator begin() { return elements_.begin(); }

    // Returns iterator on end element of hash table.
    iterator end() { return elements_.end(); }

    // Returns iterator on first element of hash table.
    const_iterator begin() const { return elements_.begin(); }

    // Returns iterator on end element of hash table.
    const_iterator end() const { return elements_.end(); }

    // If hash table contains element with such key - returns iterator on such
    // element.
    // If not - returns end() iterator.
    // Works for expected O(1).
    iterator find(const KeyType& key) {
        size_t raw_hash = hash_(key);
        auto p = get_position(key, raw_hash);
        size_t hash = p.first;
        int ind = p.second;

        if (ind != -1) {
            return hash_table_[hash][ind];
        } else {
            return end();
        }
    }

    // If hash table contains element with such key - returns iterator on such
    // element.
    // If not - returns end() iterator.
    // Works for expected O(1).
    const_iterator find(const KeyType& key) const {
        size_t raw_hash = hash_(key);
        auto p = get_position(key, raw_hash);
        size_t hash = p.first;
        int ind = p.second;

        if (ind != -1) {
            return const_iterator(hash_table_[hash][ind]);
        } else {
            return end();
        }
    }

    // Returns value of element with given key.
    // If there is no element with such key - creates element with such key and
    // default value.
    // Works for expected O(1).
    ValueType& operator[](const KeyType& key) {
        size_t raw_hash = add_element({ key, ValueType() });

        auto p = get_position(key, raw_hash);
        size_t hash = p.first;
        int ind = p.second;

        return hash_table_[hash][ind]->second;
    }

    // Returns value of element with given key.
    // If there is no element with such key - throws an error.
    // Works for expected O(1).
    const ValueType& at(const KeyType& key) const {
        const_iterator it = find(key);
        if (it != end()) {
            return it->second;
        } else {
            throw std::out_of_range("No such key!");
        }
    }

    // Cleares hash table.
    // Works for O(capacity).
    void clear() {
        elements_.clear();
        size_ = 0;
        hash_table_.assign(kDefaultCapacity, std::vector<ListIter>());
        capacity_ = kDefaultCapacity;
    }
};
