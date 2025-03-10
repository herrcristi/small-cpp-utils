#pragma once

#include <list>
#include <unordered_map>

namespace small {

    //
    // a simple LRU cache
    //
    struct lru_cache_config
    {
        std::size_t capacity{};
    };

    template <typename Key, typename Value>
    class lru_cache
    {
    public:
        lru_cache(const lru_cache_config& config = {}) : m_config(config)
        {
        }

        ~lru_cache() = default;

        //
        // lru functions
        //
        inline std::size_t size() const
        {
            return m_cache.size();
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

        // operators
        inline Value& operator[](const Key& key)
        {
            return get(key);
        }

        inline Value& operator[](Key&& key)
        {
            return get(std::move(key));
        }

    private:
        //
        // members
        //
        lru_cache_config m_config{};

        // simulate a fast list
        std::list<std::pair<Key, Value>>                                             m_list;
        std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> m_cache;
    };
} // namespace small
