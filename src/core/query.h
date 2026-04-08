#ifndef QUERY_H
#define QUERY_H

#include <vector>
#include <tuple>
#include <algorithm>


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
    std::unordered_map<size_t, size_t> indexMap; // id -> index w gameobjects
    uint64_t requiredMask = 0;

    Query() : requiredMask((Components::ComponentBit | ...)) {}

    void OnGameObjectUpdated(GameObject* e) override {
        // Sprawdzenie, czy wszystkie wymagane komponenty s¹ obecne przez bitmaskê
        bool match = (e->componentMask & requiredMask) == requiredMask;

        auto it = indexMap.find(e->id);
        size_t currentIndex = (it != indexMap.end()) ? it->second : size_t(-1);

        if (match) {
            if (currentIndex == size_t(-1)) {
                // Dodanie nowego GameObject
                size_t newIndex = gameobjects.size();
                gameobjects.push_back(e);
                indexMap[e->id] = newIndex;

                (..., (std::get<std::vector<Components*>>(componentsVectors).push_back(e->GetComponent<Components>())));
            }
        }
        else if (currentIndex != size_t(-1)) {
            // Swap-and-pop w SoA
            size_t last = gameobjects.size() - 1;
            GameObject* lastObj = gameobjects[last];

            gameobjects[currentIndex] = lastObj;
            gameobjects.pop_back();

            size_t i = 0;
            (..., (
                std::swap(std::get<std::vector<Components*>>(componentsVectors)[i][currentIndex],
                    std::get<std::vector<Components*>>(componentsVectors)[i].back()),
                std::get<std::vector<Components*>>(componentsVectors)[i].pop_back(),
                ++i
                ));

            indexMap[lastObj->id] = currentIndex;
            indexMap.erase(e->id);
        }
    }
};

#endif


/*
void OnGameObjectUpdated(GameObject* e) override {
        size_t idx = e->id;
        if (idx > maxID) return;

        // Pobranie komponentów raz z poola
        auto comps = std::make_tuple(GetPool<Components>().pool.data() + e->GetComponentIndex<Components>()...);

        // Sprawdzenie, czy wszystkie komponenty istniej¹
        bool match = (... && (std::get<Components*>(comps) != nullptr));

        size_t currentIndex = indexMap[idx];

        if (match) {
            if (currentIndex == size_t(-1)) {
                // Dodanie nowego GameObject
                size_t newIndex = gameobjects.size();
                gameobjects.push_back(e);
                indexMap[idx] = newIndex;

                size_t i = 0;
                (..., (std::get<std::vector<Components*>>(componentsVectors)[i++].push_back(std::get<Components*>(comps))));
            }
        }
        else {
            if (currentIndex != size_t(-1)) {
                // Swap-and-pop w SoA
                size_t last = gameobjects.size() - 1;
                GameObject* lastObj = gameobjects[last];

                gameobjects[currentIndex] = lastObj;
                gameobjects.pop_back();

                size_t i = 0;
                (..., (
                    std::get<std::vector<Components*>>(componentsVectors)[i][currentIndex] =
                    std::get<std::vector<Components*>>(componentsVectors)[i][last],
                    std::get<std::vector<Components*>>(componentsVectors)[i].pop_back(),
                    ++i
                    ));

                indexMap[lastObj->id] = currentIndex;
                indexMap[idx] = size_t(-1);
            }
        }
    }

*/



/*
    void OnGameObjectUpdated(GameObject* e) {
        bool match = ((e->GetComponent<Components>() != nullptr) && ...);
        size_t idx = e->id;

        if (idx > maxID) return; // jeœli id poza zakresem

        size_t currentIndex = indexMap[idx];

        if (match) {
            if (currentIndex == size_t(-1)) {
                // dodanie nowego
                size_t newIndex = gameobjects.size();
                gameobjects.push_back(e);
                indexMap[idx] = newIndex;

                size_t i = 0;
                ((componentsVectors[i++].push_back(e->GetComponent<Components>())), ...);
            }
        }
        else {
            if (currentIndex != size_t(-1)) {
                // swap-and-pop w SoA
                size_t last = gameobjects.size() - 1;
                GameObject* lastObj = gameobjects[last];

                gameobjects[currentIndex] = lastObj;
                gameobjects.pop_back();

                size_t i = 0;
                ((componentsVectors[i][currentIndex] = componentsVectors[i][last].back(), componentsVectors[i].pop_back(), ++i), ...);

                indexMap[lastObj->id] = currentIndex;
                indexMap[idx] = size_t(-1);
            }
        }
    }
    */