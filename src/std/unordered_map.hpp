/*
 * Fast open addressing hash table with tombstone bit map.
 *
 * Copyright (c) 2020 Michael Clark <michaeljclark@mac.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <cassert>

#include "std/hash.hpp"
#include "type_traits.hpp"

#include <unordered_map>
#include <memory>

namespace zedland {

/*
 * This open addressing hashmap uses a 2-bit entry per slot bitmap
 * that eliminates the need for empty and deleted key sentinels.
 * The hashmap has a simple array of key and value pairs and the
 * tombstone bitmap, which are allocated in a single call to malloc.
 */

template<typename T>
struct equal_to {
    constexpr bool operator()(const T& x, const T& y) const noexcept {
        return x == y;
    }
};

template <class Key, class Value, class Hash = xstd::hash<Key>, class Pred = equal_to<Key>>
struct hashmap
{
    static const size_t default_size =    (2<<3);  /* 16 */
    static const size_t load_factor =     (2<<15); /* 0.5 */
    static const size_t load_multiplier = (2<<16); /* 1.0 */

    static inline Hash _hasher;
    static inline Pred _compare;

    struct data_type {
        Key first;
        Value second;
    };

    typedef Key key_type;
    typedef Value mapped_type;
    typedef data_type value_type;
    typedef Hash hasher;
    typedef Pred key_equal;
    typedef data_type& reference;
    typedef const data_type& const_reference;

    size_t used;
    size_t tombs;
    size_t limit;
    data_type *data;
    uint64_t *bitmap;

    /*
     * scanning iterator
     */

    struct iterator
    {
        hashmap *h;
        size_t i;

        iterator(hashmap* h, size_t i) noexcept {
            this->h = h;
            this->i = step(i);
        }

        size_t step(size_t i) {
            while (i < h->limit &&
                   (bitmap_get(h->bitmap, i) & occupied) != occupied) i++;
            if ((bitmap_get(h->bitmap, i) & occupied) != occupied) i = h->limit;
            return i;
        }
        iterator& operator++() { i = step(i+1); return *this; }
        iterator operator++(int) { iterator r = *this; ++(*this); return r; }
        data_type& operator*() { i = step(i); return h->data[i]; }
        data_type* operator->() { i = step(i); return &h->data[i]; }
        bool operator==(const iterator &o) const { return h == o.h && i == o.i; }
        bool operator!=(const iterator &o) const { return h != o.h || i != o.i; }
    };
    struct const_iterator
    {
        const hashmap *h;
        size_t i;
        const_iterator(const hashmap* h, size_t i) noexcept {
            this->h = h;
            this->i = step(i);
        }


        size_t step(size_t i) {
            while (i < h->limit &&
                   (bitmap_get(h->bitmap, i) & occupied) != occupied) i++;
            if ((bitmap_get(h->bitmap, i) & occupied) != occupied) i = h->limit;
            return i;
        }
        const_iterator& operator++() { i = step(i+1); return *this; }
        const_iterator operator++(int) { const_iterator r = *this; ++(*this); return r; }
        const data_type& operator*() { i = step(i); return h->data[i]; }
        const data_type* operator->() { i = step(i); return &h->data[i]; }
        bool operator==(const const_iterator &o) const { return h == o.h && i == o.i; }
        bool operator!=(const const_iterator &o) const { return h != o.h || i != o.i; }
    };

    /*
     * constructors and destructor
     */

    inline hashmap() : hashmap(default_size) {}
    inline hashmap(size_t initial_size) :
        used(0), tombs(0), limit(initial_size)
    {
        size_t data_size = sizeof(data_type) * limit;
        size_t bitmap_size = limit >> 2;
        size_t total_size = data_size + bitmap_size;

        assert(is_pow2(limit));

        data = (data_type*)malloc(total_size);
        bitmap = (uint64_t*)((char*)data + data_size);
        memset(data, 0, total_size);
    }
    inline ~hashmap() { free(data); }

    /*
     * copy constructor and assignment operator
     */

    inline hashmap(const hashmap &o) :
        used(o.used), tombs(o.tombs), limit(o.limit)
    {
        size_t data_size = sizeof(data_type) * limit;
        size_t bitmap_size = limit >> 2;
        size_t total_size = data_size + bitmap_size;

        data = (data_type*)malloc(total_size);
        bitmap = (uint64_t*)((char*)data + data_size);
        memcpy(data, o.data, total_size);
    }

    inline hashmap(hashmap &&o) :
        used(o.used), tombs(o.tombs), limit(o.limit),
        data(o.data), bitmap(o.bitmap)
    {
        o.data = nullptr;
        o.bitmap = nullptr;
    }

    inline hashmap(std::initializer_list<data_type> init) : hashmap(default_size) {
        for (auto& d : init) emplace(std::move(d));
    }

    inline hashmap& operator=(const hashmap &o)
    {
        free(data);

        used = o.used;
        tombs = o.tombs;
        limit = o.limit;

        size_t data_size = sizeof(data_type) * limit;
        size_t bitmap_size = limit >> 2;
        size_t total_size = data_size + bitmap_size;

        data = (data_type*)malloc(total_size);
        bitmap = (uint64_t*)((char*)data + data_size);
        memcpy(data, o.data, total_size);

        return *this;
    }

    inline hashmap& operator=(hashmap &&o)
    {
        data = o.data;
        bitmap = o.bitmap;
        used = o.used;
        tombs = o.tombs;
        limit = o.limit;

        o.data = nullptr;
        o.bitmap = nullptr;

        return *this;
    }

    /*
     * member functions
     */

    inline bool empty() const { return used == 0; }
    inline size_t size() const { return used; }
    inline size_t capacity() const { return limit; }
    inline size_t load() { return (used + tombs) * load_multiplier / limit; }
    inline size_t index_mask() const { return limit - 1; }
    inline size_t hash_index(uint64_t h) const { return h & index_mask(); }
    inline size_t key_index(Key key) const { return hash_index(_hasher(key)); }
    inline hasher hash_function() const { return _hasher; }
    inline iterator begin() { return iterator( this, 0 ); }
    inline iterator end() { return iterator( this, limit ); }
    inline const_iterator begin() const { return const_iterator( this, 0 ); }
    inline const_iterator end() const { return const_iterator( this, limit ); }

    /*
     * bit manipulation helpers
     */

    enum bitmap_state {
        available = 0, occupied = 1, deleted = 2, recycled = 3
    };
    static inline size_t bitmap_idx(size_t i) { return i >> 5; }
    static inline size_t bitmap_shift(size_t i) { return ((i << 1) & 63); }
    static inline bitmap_state bitmap_get(uint64_t *bitmap, size_t i)
    {
        return (bitmap_state)((bitmap[bitmap_idx(i)] >> bitmap_shift(i)) & 3);
    }
    static inline bitmap_state bitmap_get(const uint64_t *bitmap, size_t i)
    {
        return (bitmap_state)((bitmap[bitmap_idx(i)] >> bitmap_shift(i)) & 3);
    }
    static inline void bitmap_set(uint64_t *bitmap, size_t i, uint64_t value)
    {
        bitmap[bitmap_idx(i)] |= (value << bitmap_shift(i));
    }
    static inline void bitmap_clear(uint64_t *bitmap, size_t i, uint64_t value)
    {
        bitmap[bitmap_idx(i)] &= ~(value << bitmap_shift(i));
    }
    static inline bool is_pow2(intptr_t n) { return  ((n & -n) == n); }

    /**
     * the implementation
     */

    void resize_internal(data_type *old_data, uint64_t *old_bitmap,
                         size_t old_size, size_t new_size)
    {
        size_t data_size = sizeof(data_type) * new_size;
        size_t bitmap_size = new_size >> 2;
        size_t total_size = data_size + bitmap_size;

        assert(is_pow2(new_size));

        data = (data_type*)malloc(total_size);
        bitmap = (uint64_t*)((char*)data + data_size);
        limit = new_size;
        memset(data, 0, total_size);

        size_t i = 0;
        for (data_type *v = old_data; v != old_data + old_size; v++, i++) {
            if ((bitmap_get(old_bitmap, i) & occupied) != occupied) continue;
            for (size_t j = key_index(v->first); ; j = (j+1) & index_mask()) {
                if ((bitmap_get(bitmap, j) & occupied) != occupied) {
                    bitmap_set(bitmap, j, occupied);
                    data[j] = std::move(*v);
                    break;
                }
            }
        }

        tombs = 0;
        free(old_data);
    }

    void clear()
    {
        size_t data_size = sizeof(data_type) * limit;
        size_t bitmap_size = limit >> 2;
        size_t total_size = data_size + bitmap_size;
        memset(data, 0, total_size);
        used = tombs = 0;
    }

    iterator insert(iterator i, const value_type& val) { return insert(val); }
    iterator insert(Key key, Value val) { return insert(value_type{key, val}); }
    iterator emplace(Key key, Value val) {
        auto v = value_type{std::move(key), std::move(val)};
        return insert(std::move(v));
    }

    iterator emplace(value_type v) {
        return insert(std::move(v));
    }
    iterator insert(const value_type& v)
    {
        for (size_t i = key_index(v.first); ; i = (i+1) & index_mask()) {
            bitmap_state state = bitmap_get(bitmap, i);
            if ((state & occupied) != occupied) {
                bitmap_set(bitmap, i, occupied);
                data[i] = v;
                used++;
                if ((state & deleted) == deleted) tombs--;
                if (load() > load_factor) {
                    resize_internal(data, bitmap, limit, limit << 1);
                    for (i = key_index(v.first); ; i = (i+1) & index_mask()) {
                        bitmap_state state = bitmap_get(bitmap, i);
                             if (state == available) abort();
                        else if (state == deleted); /* skip */
                        else if (_compare(data[i].first, v.first)) {
                            return iterator(this, i);
                        }
                    }
                } else {
                    return iterator(this, i);
                }
            } else if (_compare(data[i].first, v.first)) {
                data[i].second = v.second;
                return iterator(this, i);
            }
        }
    }
    iterator insert(value_type&& v)
    {
        for (size_t i = key_index(v.first); ; i = (i+1) & index_mask()) {
            bitmap_state state = bitmap_get(bitmap, i);
            if ((state & occupied) != occupied) {
                bitmap_set(bitmap, i, occupied);
                auto old_key = v.first;
                data[i] = std::move(v);
                used++;
                if ((state & deleted) == deleted) tombs--;
                if (load() > load_factor) {
                    resize_internal(data, bitmap, limit, limit << 1);
                    for (i = key_index(old_key); ; i = (i+1) & index_mask()) {
                        bitmap_state state = bitmap_get(bitmap, i);
                             if (state == available) abort();
                        else if (state == deleted); /* skip */
                        else if (_compare(data[i].first, old_key)) {
                            return iterator(this, i);
                        }
                    }
                } else {
                    return iterator(this, i);
                }
            } else if (_compare(data[i].first, v.first)) {
                data[i].second = std::move(v.second);
                return iterator(this, i);
            }
        }
    }

    Value& operator[](Key key)
    {
        for (size_t i = key_index(key); ; i = (i+1) & index_mask()) {
            bitmap_state state = bitmap_get(bitmap, i);
            if ((state & occupied) != occupied) {
                used++;
                if ((state & deleted) == deleted) tombs--;
                bitmap_set(bitmap, i, occupied);

                new (&data[i]) value_type;
                data[i].first = key;

                if (load() > load_factor) {
                    resize_internal(data, bitmap, limit, limit << 1);
                    for (i = key_index(key);; i = (i+1) & index_mask()) {
                        bitmap_state state = bitmap_get(bitmap, i);
                             if (state == available) abort();
                        else if (state == deleted); /* skip */
                        else if (_compare(data[i].first, key)) {
                            return data[i].second;
                        }
                    }
                }
                return data[i].second;
            } else if (_compare(data[i].first, key)) {
                return data[i].second;
            }
        }
    }

    iterator find(const Key &key)
    {
        for (size_t i = key_index(key); ; i = (i+1) & index_mask()) {
            bitmap_state state = bitmap_get(bitmap, i);
                 if (state == available)           /* notfound */ break;
            else if (state == deleted);            /* skip */
            else if (_compare(data[i].first, key)) return iterator(this, i);
        }
        return end();
    }
    iterator find(const Key &key) const
    {
        for (size_t i = key_index(key); ; i = (i+1) & index_mask()) {
            bitmap_state state = bitmap_get(bitmap, i);
                 if (state == available)           /* notfound */ break;
            else if (state == deleted);            /* skip */
            else if (_compare(data[i].first, key)) return iterator(this, i);
        }
        return end();
    }
    Value& at(const Key& key) {
        return find(key)->second;
    }
    const Value& at(const Key &key) const
    {
        for (size_t i = key_index(key); ; i = (i+1) & index_mask()) {
            bitmap_state state = bitmap_get(bitmap, i);
                 if (state == available)           /* notfound */ break;
            else if (state == deleted);            /* skip */
            else if (_compare(data[i].first, key)) return data[i].second;
        }
        assert(false);
        static Value v;
        return v;
    }
    bool contains(const Key &key) const
    {
        if (!bitmap) return false;
        for (size_t i = key_index(key); ; i = (i+1) & index_mask()) {
            bitmap_state state = bitmap_get(bitmap, i);
                 if (state == available)           /* notfound */ return false;
            else if (state == deleted);            /* skip */
            else if (_compare(data[i].first, key)) return true;
        }
        return false;
    }

    void erase(Key key)
    {
        for (size_t i = key_index(key); ; i = (i+1) & index_mask()) {
            bitmap_state state = bitmap_get(bitmap, i);
                 if (state == available)           /* notfound */ break;
            else if (state == deleted);            /* skip */
            else if (_compare(data[i].first, key)) {
                data[i].~value_type();
                bitmap_set(bitmap, i, deleted);
                bitmap_clear(bitmap, i, occupied);
                used--;
                tombs++;
                return;
            }
        }
    }
};

};

namespace xstd {
    template<typename K, typename V> using unordered_map = zedland::hashmap<K, V>;
}