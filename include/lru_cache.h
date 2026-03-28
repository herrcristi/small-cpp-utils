#pragma once

#include <list>
#include <mutex>
#include <unordered_map>

namespace small {

    //
    // a simple LRU cache configuration
    //
    struct lru_cache_config
    {
        std::size_t capacity{static_cast<std::size_t>(-1)};
    };

    //
    // NOT thread-safe LRU cache (use lru_cache for thread-safe version)
    //
    template <typename Key, typename Value>
    class lru_cache_unsafe
    {
    public:
        lru_cache_unsafe(const lru_cache_config& config = {}) : m_config(config)
        {
        }

        lru_cache_unsafe(const lru_cache_unsafe& o) { operator=(o); }
        lru_cache_unsafe(lru_cache_unsafe&& o) noexcept { operator=(std::move(o)); }

        lru_cache_unsafe& operator=(const lru_cache_unsafe& o)
        {
            m_config = o.m_config;
            m_list   = o.m_list;
            m_cache.clear();
            for (auto it = m_list.begin(); it != m_list.end(); ++it) {
                m_cache[it->first] = it;
            }
            return *this;
        }
        lru_cache_unsafe& operator=(lru_cache_unsafe&& o) noexcept
        {
            m_config = std::move(o.m_config);
            m_list   = std::move(o.m_list);
            m_cache  = std::move(o.m_cache);
            return *this;
        }

        ~lru_cache_unsafe() = default;

        //
        // lru functions
        //
        inline std::size_t size() const
        {
            return m_cache.size();
        }

        inline void clear()
        {
            m_cache.clear();
            m_list.clear();
        }

        inline void set(const Key& key, const Value& value)
        {
            if (m_config.capacity <= 0) {
                return;
            }

            auto it = m_cache.find(key);
            if (it == m_cache.end()) {
                // add the key to the front of the list
                m_list.push_front({key, value});
                m_cache[key] = m_list.begin();
            } else {
                // overwrite the value and move it to the front of the list
                it->second->second = value;
                m_list.splice(m_list.begin(), m_list, it->second);
            }

            // remove last element
            if (m_cache.size() > m_config.capacity) {
                m_cache.erase(m_list.back().first); // erase the key
                m_list.pop_back();
            }
        }

        inline Value* get(const Key& key)
        {
            auto it = m_cache.find(key);
            if (it == m_cache.end()) {
                return nullptr;
            }

            // due to access, move it to the front of the list
            m_list.splice(m_list.begin(), m_list, it->second);
            return &(it->second->second);
        }

        inline void erase(const Key& key)
        {
            auto it = m_cache.find(key);
            if (it == m_cache.end()) {
                return;
            }

            m_list.erase(it->second);
            m_cache.erase(it);
        }

        // operators
        inline Value* operator[](const Key& key)
        {
            return get(key);
        }

        inline Value* operator[](Key&& key)
        {
            return get(std::move(key));
        }

    private:
        //
        // members
        //
        lru_cache_config m_config{};

        // simulate an ordered map
        using Node = std::pair<Key, Value>;
        std::list<Node>                                             m_list;
        std::unordered_map<Key, typename std::list<Node>::iterator> m_cache;
    };

    //
    // Thread-safe LRU cache wrapper around lru_cache_unsafe
    //
    template <typename Key, typename Value>
    class lru_cache
    {
    public:
        lru_cache(const lru_cache_config& config = {}) : m_impl(config)
        {
        }

        lru_cache(const lru_cache& o)
        {
            std::unique_lock lock(o.m_mutex);
            m_impl = o.m_impl;
        }
        lru_cache(lru_cache&& o) noexcept
        {
            std::unique_lock lock(o.m_mutex);
            m_impl = std::move(o.m_impl);
        }

        lru_cache& operator=(const lru_cache& o)
        {
            if (this != &o) {
                std::unique_lock lock1(m_mutex, std::defer_lock);
                std::unique_lock lock2(o.m_mutex, std::defer_lock);
                std::lock(lock1, lock2);
                m_impl = o.m_impl;
            }
            return *this;
        }
        lru_cache& operator=(lru_cache&& o) noexcept
        {
            if (this != &o) {
                std::unique_lock lock1(m_mutex, std::defer_lock);
                std::unique_lock lock2(o.m_mutex, std::defer_lock);
                std::lock(lock1, lock2);
                m_impl = std::move(o.m_impl);
            }
            return *this;
        }

        ~lru_cache() = default;

        //
        // thread-safe lru functions
        //
        inline std::size_t size() const
        {
            std::unique_lock lock(m_mutex);
            return m_impl.size();
        }

        inline void clear()
        {
            std::unique_lock lock(m_mutex);
            m_impl.clear();
        }

        inline void set(const Key& key, const Value& value)
        {
            std::unique_lock lock(m_mutex);
            m_impl.set(key, value);
        }

        inline Value* get(const Key& key)
        {
            std::unique_lock lock(m_mutex);
            return m_impl.get(key);
        }

        inline void erase(const Key& key)
        {
            std::unique_lock lock(m_mutex);
            m_impl.erase(key);
        }

        // operators (thread-safe)
        inline Value* operator[](const Key& key)
        {
            std::unique_lock lock(m_mutex);
            return m_impl.get(key);
        }

        inline Value* operator[](Key&& key)
        {
            std::unique_lock lock(m_mutex);
            return m_impl.get(std::forward<Key>(key));
        }

    private:
        mutable std::mutex           m_mutex;
        lru_cache_unsafe<Key, Value> m_impl;
    };
} // namespace small
