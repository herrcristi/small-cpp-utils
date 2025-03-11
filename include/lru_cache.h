#pragma once

#include <list>
#include <unordered_map>

namespace small {

    //
    // a simple LRU cache
    //
    struct lru_cache_config
    {
        std::size_t capacity{static_cast<std::size_t>(-1)};
    };

    template <typename Key, typename Value>
    class lru_cache
    {
    public:
        lru_cache(const lru_cache_config& config = {}) : m_config(config)
        {
        }

        lru_cache(const lru_cache& o) { operator=(o); }
        lru_cache(lru_cache&& o) noexcept { operator=(std::move(o)); }

        lru_cache& operator=(const lru_cache& o)
        {
            m_config = o.m_config;
            m_list   = o.m_list;
            m_cache.clear();
            for (auto it = m_list.begin(); it != m_list.end(); ++it) {
                m_cache[it->first] = it;
            }
            return *this;
        }
        lru_cache& operator=(lru_cache&& o) noexcept
        {
            m_config = std::move(o.m_config);
            m_list   = std::move(o.m_list);
            m_cache  = std::move(o.m_cache);
            return *this;
        }

        ~lru_cache() = default;

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
} // namespace small
