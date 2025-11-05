#pragma once
#include <unordered_map>
#include <vector>
#include <utility>      // pair
#include <cstddef>      // size_t
#include <iterator>     // std::prev

//============================
// ordered_map (insertion-preserving associative container)
//============================
template<typename K, typename V>
class ordered_map {
public:
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;
    using size_type = size_t;
    using container_type = std::vector<std::pair<K, V>>;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;

    ordered_map() = default;
    ordered_map(const ordered_map&) = default;
    ordered_map& operator=(const ordered_map&) = default;
    ordered_map(ordered_map&&) noexcept = default;
    ordered_map& operator=(ordered_map&&) noexcept = default;

    inline V& operator[](const K& key) {
        auto it = m_index.find(key);
        if (it == m_index.end()) return push_back(key, V{})->second;
        return m_data[it->second].second;
    }

    inline V& operator[](K&& key) {
        auto it = m_index.find(key);
        if (it == m_index.end()) return push_back(std::move(key), V{})->second;
        return m_data[it->second].second;
    }

    inline V& at(const K& key) { return m_data.at(m_index.at(key)).second; }
    inline const V& at(const K& key) const { return m_data.at(m_index.at(key)).second; }

    inline std::pair<iterator, bool> insert(iterator pos, const value_type& value) {
        auto it = m_index.find(value.first);
        if (it != m_index.end()) return { m_data.begin() + it->second,false };
        auto p = m_data.insert(pos, { value.first,value.second });
        rebuild_index(); return { p,true };
    }

    inline std::pair<iterator, bool> insert(iterator pos, value_type&& value) {
        auto it = m_index.find(value.first);
        if (it != m_index.end()) return { m_data.begin() + it->second,false };
        auto p = m_data.insert(pos, std::move(value));
        rebuild_index(); return { p,true };
    }

    inline iterator push_back(const K& key, V&& value) {
        auto it = m_index.find(key);
        if (it != m_index.end()) return m_data.begin() + it->second;
        m_data.emplace_back(key, std::move(value));
        m_index[key] = m_data.size() - 1;
        return std::prev(m_data.end());
    }

    inline iterator push_back(K&& key, V&& value) {
        auto it = m_index.find(key);
        if (it != m_index.end()) return m_data.begin() + it->second;
        m_data.emplace_back(std::move(key), std::move(value));
        m_index[m_data.back().first] = m_data.size() - 1;
        return std::prev(m_data.end());
    }

    template<typename... Args>
    inline iterator emplace_back(const K& key, Args&&... args) {
        auto it = m_index.find(key);
        if (it != m_index.end()) return m_data.begin() + it->second;
        m_data.emplace_back(key, V(std::forward<Args>(args)...));
        m_index[key] = m_data.size() - 1;
        return std::prev(m_data.end());
    }

    template<typename... Args>
    inline std::pair<iterator, bool> emplace(const K& key, Args&&... args) {
        auto it = m_index.find(key);
        if (it != m_index.end()) return { m_data.begin() + it->second,false };
        m_data.emplace_back(key, V(std::forward<Args>(args)...));
        m_index[key] = m_data.size() - 1;
        return { std::prev(m_data.end()),true };
    }

    inline void erase(const K& key) {
        auto it = m_index.find(key);
        if (it == m_index.end()) return;
        size_t idx = it->second;
        m_data.erase(m_data.begin() + idx);
        m_index.erase(it);
        rebuild_index();
    }

    inline iterator erase(iterator pos) {
        if (pos == m_data.end()) return m_data.end();
        m_index.erase(pos->first);
        auto nxt = m_data.erase(pos);
        rebuild_index(); return nxt;
    }

    inline void clear() noexcept { m_data.clear(); m_index.clear(); }

    inline iterator find(const K& key) {
        auto it = m_index.find(key);
        return it == m_index.end() ? m_data.end() : m_data.begin() + it->second;
    }

    inline const_iterator find(const K& key) const {
        auto it = m_index.find(key);
        return it == m_index.end() ? m_data.end() : m_data.begin() + it->second;
    }

    inline bool contains(const K& key) const noexcept { return m_index.find(key) != m_index.end(); }
    inline size_type count(const K& key) const noexcept { return m_index.count(key); }

    inline iterator begin() noexcept { return m_data.begin(); }
    inline iterator end() noexcept { return m_data.end(); }
    inline const_iterator begin() const noexcept { return m_data.begin(); }
    inline const_iterator end() const noexcept { return m_data.end(); }
    inline const_iterator cbegin() const noexcept { return m_data.cbegin(); }
    inline const_iterator cend() const noexcept { return m_data.cend(); }

    inline bool empty() const noexcept { return m_data.empty(); }
    inline size_type size() const noexcept { return m_data.size(); }

private:
    inline void rebuild_index() {
        m_index.clear();
        for (size_t i = 0; i < m_data.size(); ++i) m_index[m_data[i].first] = i;
    }

    container_type m_data;
    std::unordered_map<K, size_t> m_index;
};