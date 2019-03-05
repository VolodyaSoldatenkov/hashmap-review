#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <utility>
#include <vector>


template<typename Key, typename Value, typename Hash = std::hash<Key>>
class HashMap {
private:
    using StoredType = std::pair<const Key, Value>;
    using StoredIterator = typename std::list<StoredType>::iterator;
    using StoredConstIterator = typename std::list<StoredType>::const_iterator;
    using Bucket = std::list<StoredType>;
    using BucketIterator = typename std::vector<Bucket>::iterator;
    using BucketConstIterator = typename std::vector<Bucket>::const_iterator;

    std::vector<Bucket> mData;
    Hash mHash;
    size_t mSize;

private:
    double loadFactor() const {
        return mSize / mData.size();
    }

    void rehash() {
        size_t oldSize = mData.size();
        mData.resize(oldSize * 2);
        for (size_t i = 0; i != oldSize; ++i) {
            for (auto it = mData[i].begin(); it != mData[i].end();) {
                size_t newIndex = indexOf(it->first);
                if (newIndex != i) {
                    mData[newIndex].push_back(*it);
                    it = mData[i].erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    size_t indexOf(const Key& key) const {
        return mHash(key) % mData.size();
    }

public:
    explicit HashMap(Hash hash = Hash()) : mData(1), mHash(hash), mSize(0) {}

    HashMap(const HashMap& rhs) : mData(rhs.mData), mHash(rhs.mHash), mSize(rhs.mSize) {}

    HashMap(HashMap&& rhs) noexcept(std::is_nothrow_move_constructible_v<Hash>) :
            mData(std::move(rhs.mData)),
            mHash(std::move(rhs.mHash)),
            mSize(rhs.mSize) {}
            
    HashMap& operator=(HashMap rhs) {
        swap(rhs);
        return *this;
    }

    template<typename Iter>
    HashMap(Iter begin, Iter end, Hash hash = Hash()) : mHash(hash), mSize(std::distance(begin, end)) {
        mData.resize(mSize * 2 ?: 1);
        while (begin != end) {
            size_t index = indexOf(begin->first);
            mData[index].push_back(*begin++);
        }
    }

    HashMap(std::initializer_list<StoredType> init, Hash hash = Hash()) : HashMap(init.begin(), init.end(), hash) {}

    size_t size() const {
        return mSize;
    }

    bool empty() const {
        return size() == 0;
    }

    Hash hash_function() const {
        return mHash;
    }

    class iterator : public std::iterator<std::forward_iterator_tag, StoredType> {
        friend class HashMap;

    private:
        BucketIterator mBucket;
        BucketIterator mBucketEnd;
        StoredIterator mStored;

        explicit iterator(BucketIterator bucket,
                          BucketIterator bucketEnd,
                          StoredIterator stored = StoredIterator()) :
                mBucket(bucket),
                mBucketEnd(bucketEnd),
                mStored(stored) {
            while (mStored == mBucket->end()) {
                if (++mBucket == mBucketEnd) {
                    mStored = StoredIterator();
                    return;
                }
                mStored = mBucket->begin();
            }
        }

    public:
        iterator() = default;

        StoredType& operator*() const {
            return *mStored;
        }

        StoredType* const operator->() const {
            return mStored.operator->();
        }

        iterator& operator++() {
            ++mStored;
            while (mStored == mBucket->end()) {
                if (++mBucket == mBucketEnd) {
                    mStored = StoredIterator();
                    return *this;
                }
                mStored = mBucket->begin();
            }
            return *this;
        }

        iterator operator++(int) {
            iterator old = *this;
            ++*this;
            return old;
        }

        friend bool operator==(const iterator& lhs, const iterator& rhs) {
            return lhs.mBucket == rhs.mBucket && lhs.mStored == rhs.mStored;
        }

        friend bool operator!=(const iterator& lhs, const iterator& rhs) {
            return !(lhs == rhs);
        }
    };

    class const_iterator : public std::iterator<std::forward_iterator_tag, const StoredType> {
        friend class HashMap;

    private:
        BucketConstIterator mBucket;
        BucketConstIterator mBucketEnd;
        StoredConstIterator mStored;

        explicit const_iterator(BucketConstIterator bucket,
                                BucketConstIterator bucketEnd,
                                StoredConstIterator stored = StoredConstIterator()) :
                mBucket(bucket),
                mBucketEnd(bucketEnd),
                mStored(stored) {
            while (mStored == mBucket->end()) {
                if (++mBucket == mBucketEnd) {
                    mStored = StoredIterator();
                    return;
                }
                mStored = mBucket->begin();
            }
        }

    public:
        const_iterator() = default;

        const StoredType& operator*() const {
            return *mStored;
        }

        const StoredType* const operator->() const {
            return mStored.operator->();
        }

        const_iterator& operator++() {
            ++mStored;
            while (mStored == mBucket->end()) {
                if (++mBucket == mBucketEnd) {
                    mStored = StoredIterator();
                    return *this;
                }
                mStored = mBucket->begin();
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator old = *this;
            ++*this;
            return old;
        }

        friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) {
            return lhs.mBucket == rhs.mBucket && lhs.mStored == rhs.mStored;
        }

        friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs) {
            return !(lhs == rhs);
        }
    };

    iterator begin() {
        return iterator(mData.begin(), mData.end(), mData.front().begin());
    }

    iterator end() {
        return iterator(mData.end(), mData.end());
    }

    const_iterator begin() const {
        return const_iterator(mData.begin(), mData.end(), mData.front().begin());
    }

    const_iterator end() const {
        return const_iterator(mData.end(), mData.end());
    }

    std::pair<iterator, bool> insert(const StoredType& in) {
        if (loadFactor() > 0.5)
            rehash();
        size_t index = indexOf(in.first);
        for (auto it = mData[index].begin(); it != mData[index].end(); ++it) {
            if (it->first == in.first)
                return {iterator(mData.begin() + index, mData.end(), it), false};
        }
        auto pos = mData[index].insert(mData[index].end(), in);
        ++mSize;
        return {iterator(mData.begin() + index, mData.end(), pos), true};
    }

    std::pair<iterator, bool> insert(StoredType&& in) {
        if (loadFactor() > 0.5)
            rehash();
        size_t index = indexOf(in.first);
        for (auto it = mData[index].begin(); it != mData[index].end(); ++it) {
            if (it->first == in.first)
                return {iterator(mData.begin() + index, mData.end(), it), false};
        }
        auto pos = mData[index].insert(mData[index].end(), std::move(in));
        ++mSize;
        return {iterator(mData.begin() + index, mData.end(), pos), true};
    }

    void erase(const Key& key) {
        size_t index = indexOf(key);
        for (auto it = mData[index].begin(); it != mData[index].end(); ++it) {
            if (it->first == key) {
                mData[index].erase(it);
                --mSize;
                return;
            }
        }
    }

    iterator find(const Key& key) {
        size_t index = indexOf(key);
        for (auto it = mData[index].begin(); it != mData[index].end(); ++it) {
            if (it->first == key)
                return iterator(mData.begin() + index, mData.end(), it);
        }
        return end();
    }

    const_iterator find(const Key& key) const {
        size_t index = indexOf(key);
        for (auto it = mData[index].begin(); it != mData[index].end(); ++it) {
            if (it->first == key)
                return const_iterator(mData.begin() + index, mData.end(), it);
        }
        return end();
    }

    Value& operator[](const Key& key) {
        return insert(StoredType{key, Value()}).first->second;
    }

    const Value& at(const Key& key) const {
        size_t index = indexOf(key);
        for (auto it = mData[index].begin(); it != mData[index].end(); ++it) {
            if (it->first == key)
                return it->second;
        }
        throw std::out_of_range("");
    }

    void clear() {
        for (auto& bucket : mData)
            bucket.clear();
        mSize = 0;
    }
    
    void swap(HashMap& rhs) noexcept(std::is_nothrow_swappable_v<Hash>) {
        std::swap(mHash, rhs.mHash);
        mData.swap(rhs.mData);
        std::swap(mSize, rhs.mSize);
    }
};