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
    // Struktura SoA – oddzielne wektory komponentów i encji
    std::vector<GameObject*> gameobjects;
    std::tuple<std::vector<Components*>...> componentsVectors;

    void OnGameObjectUpdated(GameObject* e) override {
        // Sprawdzenie, czy encja ma wszystkie wymagane komponenty
        if ((e->GetComponent<Components>() && ...)) {
            // unikamy duplikatów
            if (std::find(gameobjects.begin(), gameobjects.end(), e) == gameobjects.end()) {
                gameobjects.push_back(e);
                (std::get<std::vector<Components*>>(componentsVectors).push_back(e->GetComponent<Components>()), ...);
            }
        }
        else {
            // jeœli encja nie pasuje, usuñ j¹ (swap-and-pop)
            auto it = std::find(gameobjects.begin(), gameobjects.end(), e);
            if (it != gameobjects.end()) {
                size_t index = it - gameobjects.begin();
                gameobjects.erase(it);
                ((std::get<std::vector<Components*>>(componentsVectors).erase(
                    std::get<std::vector<Components*>>(componentsVectors).begin() + index)), ...);
            }
        }
    }
};

#endif