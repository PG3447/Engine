#ifndef COMPONENT_POOL_H
#define COMPONENT_POOL_H

#include <memory>
#include <vector>
#include <deque>

template<typename T>
class ComponentPool {
private:
    std::deque<T> pool; // elementy w deque maj¹ stabilny adres
    std::vector<size_t> freeIndices;

public:
    T* Allocate() {
        if (!freeIndices.empty()) {
            size_t idx = freeIndices.back();
            freeIndices.pop_back();
            return &pool[idx];
        }
        pool.emplace_back();
        return &pool.back();
    }

    void Free(T* comp) {
        size_t idx = comp - &pool[0];
        *comp = T();
        freeIndices.push_back(idx);
    }

    void Clear() {
        pool.clear();
        freeIndices.clear();
    }

    size_t Size() const { return pool.size(); }
};

template<typename T>
static ComponentPool<T>& GetPool() {
    static ComponentPool<T> pool;
    return pool;
}

#endif