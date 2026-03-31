#ifndef QUERY_H
#define QUERY_H

#include <vector>
#include <tuple>
#include <algorithm>
#include "gameobject.h"

class QueryBase {
public:
    virtual ~QueryBase() {}
    virtual void OnGameObjectUpdated(GameObject* e) = 0;
};


template<typename... Components>
class Query : public QueryBase {
public:
    // Struktura SoA – oddzielne wektory komponentów i encji
    std::vector<GameObject*> gameobjects;
    std::tuple<std::vector<Components*>...> componentsVectors;

    void OnGameObjectUpdated(GameObject* e) override {
        // Sprawdzenie, czy encja ma wszystkie wymagane komponenty
        if ((e->GetComponent<Components>() && ...)) {
            // unikamy duplikatów
            auto it = std::find(gameobjects.begin(), gameobjects.end(), e);
            if (it == gameobjects.end()) {
                gameobjects.push_back(e);
                (std::get<std::vector<Components*>>(componentsVectors).push_back(e->GetComponent<Components>()), ...);
            }
        }
        else {
            // jeœli encja nie pasuje, usuñ j¹ (swap-and-pop)
            for (size_t i = 0; i < gameobjects.size(); ++i) {
                if (gameobjects[i] == e) {
                    gameobjects[i] = gameobjects.back();
                    gameobjects.pop_back();
                    ((std::get<std::vector<Components*>>(componentsVectors)[i] =
                        std::get<std::vector<Components*>>(componentsVectors).back(),
                        std::get<std::vector<Components*>>(componentsVectors).pop_back()), ...);
                    break;
                }
            }
        }
    }
};

#endif