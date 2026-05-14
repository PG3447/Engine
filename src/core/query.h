#ifndef QUERY_H
#define QUERY_H

#include <vector>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include "gameobject.h"

class QueryBase {
public:
    virtual ~QueryBase() {}
    virtual void OnGameObjectUpdated(GameObject* e) = 0;
};


template<typename... Components>
class Query : public QueryBase {
public:
    // Struktura SoA
    std::vector<GameObject*> gameobjects;
    std::tuple<std::vector<Components*>...> componentsVectors;
    std::unordered_map<size_t, size_t> indexMap;
    uint64_t requiredMask = 0;

    Query() : requiredMask((Components::ComponentBit | ...)) {}

    void OnGameObjectUpdated(GameObject* e) override {
        // Sprawdzenie, czy wszystkie wymagane komponenty s� obecne przez bitmask�
        bool match = (e->componentMask & requiredMask) == requiredMask;

        auto it = indexMap.find(e->id);
        size_t currentIndex = (it != indexMap.end()) ? it->second : size_t(-1);

        if (match) {
            if (currentIndex == size_t(-1)) {
                // Dodanie nowego GameObject
                size_t newIndex = gameobjects.size();
                gameobjects.push_back(e);
                indexMap[e->id] = newIndex;

                (..., (std::get<std::vector<Components*>>(componentsVectors).push_back(e->template GetComponent<Components>())));
            }
        }
        else if (currentIndex != size_t(-1)) {
            size_t last = gameobjects.size() - 1;
            GameObject* lastObj = gameobjects[last];

            gameobjects[currentIndex] = lastObj;
            gameobjects.pop_back();

            auto eraseAt = [currentIndex](auto& vec) {
                vec[currentIndex] = vec.back();
                vec.pop_back();
                };

            std::apply([&](auto&... vecs) { (eraseAt(vecs), ...); }, componentsVectors);

            indexMap[lastObj->id] = currentIndex;
            indexMap.erase(e->id);
        }
    }
};

#endif

