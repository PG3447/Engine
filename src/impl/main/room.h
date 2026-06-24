//
// Created by kubka on 25.06.2026.
//

#ifndef MIMICRY_EXPERIMENTS_ROOM_H
#define MIMICRY_EXPERIMENTS_ROOM_H

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <glm/glm.hpp>
#include "core/gameobject.h"
#include "core/component.h"

struct Room
{
    uint16_t id;
    std::string name;
    glm::vec3 position = glm::vec3(0.0);
    glm::vec3 halfSize = glm::vec3(1.0);
    std::vector<LightComponent*> lights;
    std::vector<bool> savedStates;
    std::set<GameObject*> occupants;
};

#endif //MIMICRY_EXPERIMENTS_ROOM_H
