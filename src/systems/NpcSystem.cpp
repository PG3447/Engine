#include "NpcSystem.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>

static std::mt19937& GetRng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

static float RandFloat(float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(GetRng());
}

NpcSystem::NpcSystem(ECS& ecs) {
    leaderQuery_   = ecs.CreateQuery<TransformComponent, NavPathComponent, CockroachLeaderComponent>();
    followerQuery_ = ecs.CreateQuery<TransformComponent, NavPathComponent, CockroachFollowerComponent>();
}

void NpcSystem::OnGameObjectUpdated(GameObject* go) {
    leaderQuery_->OnGameObjectUpdated(go);
    followerQuery_->OnGameObjectUpdated(go);
}

void NpcSystem::Update(ECS& ecs, float dt) {

    auto* navSys = ecs.GetSystem<NavMeshSystem>();
    if (!navSys || !navSys->IsBaked()) return;
    cachedNavMesh_ = navSys->GetNavMesh();
    if (!cachedNavMesh_) return;

    std::vector<glm::vec3> playerPositions;
    for (auto* go : ecs.GetAllGameObjects()) {
        if (go->GetComponent<CameraComponent>() && go->GetComponent<TransformComponent>()) {
            playerPositions.push_back(go->GetComponent<TransformComponent>()->position);
        }
    }

    {
        auto& gos    = leaderQuery_->gameobjects;
        auto& trs    = std::get<0>(leaderQuery_->componentsVectors);
        auto& navs   = std::get<1>(leaderQuery_->componentsVectors);
        auto& leaders = std::get<2>(leaderQuery_->componentsVectors);

        for (size_t i = 0; i < gos.size(); i++) {
            UpdateLeader(gos[i], *trs[i], *navs[i], *leaders[i], playerPositions, dt);
        }
    }

    {
        auto& gos       = followerQuery_->gameobjects;
        auto& trs       = std::get<0>(followerQuery_->componentsVectors);
        auto& navs      = std::get<1>(followerQuery_->componentsVectors);
        auto& followers = std::get<2>(followerQuery_->componentsVectors);

        for (size_t i = 0; i < gos.size(); i++) {
            UpdateFollower(gos[i], *trs[i], *navs[i], *followers[i], dt);
        }
    }
}

void NpcSystem::UpdateLeader(
    GameObject* go,
    TransformComponent& tr,
    NavPathComponent& nav,
    CockroachLeaderComponent& leader,
    const std::vector<glm::vec3>& playerPositions,
    float dt)
{
    const glm::vec3 pos = tr.position;


    if (leader.state != LeaderState::Escape) {
        if (PlayersInRange(pos, leader.detectionRadius, playerPositions)) {
            glm::vec3 escapeGoal = pos;
            float closestDist = std::numeric_limits<float>::max();
            glm::vec3 closestPlayer = pos;

            for (const auto& pp : playerPositions) {
                float d = glm::length(glm::vec3(pp.x - pos.x, 0, pp.z - pos.z));
                if (d < closestDist) { closestDist = d; closestPlayer = pp; }
            }

            glm::vec3 awayDir = pos - closestPlayer;
            awayDir.y = 0.0f;
            float len = glm::length(awayDir);
            if (len > 0.001f) awayDir /= len;
            else awayDir = glm::vec3(RandFloat(-1,1), 0, RandFloat(-1,1));

            float angle = std::atan2(awayDir.z, awayDir.x) + RandFloat(-0.8f, 0.8f);
            glm::vec3 candidate = pos + glm::vec3(
                std::cos(angle) * leader.escapeRadius,
                0.0f,
                std::sin(angle) * leader.escapeRadius
            );
            escapeGoal = ClampToNavMesh(candidate);

            leader.state = LeaderState::Escape;
            leader.escapeTimer = leader.escapeDuration;
            leader.hasActiveNavGoal = false;
            nav.path.clear();
            SendTo(nav, escapeGoal);
            spdlog::info("[Leader] player is here", escapeGoal.x, escapeGoal.z);
            return;
        }
    }

    switch (leader.state) {

    case LeaderState::Idle: {

        float distFromHome = glm::length(
            glm::vec3(pos.x - leader.homePosition.x, 0, pos.z - leader.homePosition.z));

        if (distFromHome <= leader.homeRadius) {
            leader.homeTimeAccum += dt;
            if (leader.homeTimeAccum >= leader.homeTimeRequired) {
                leader.homeTimeAccum = 0.0f;
                leader.exploreTimer = leader.exploreDuration;
                leader.state = LeaderState::Explore;
                nav.path.clear();
                leader.hasActiveNavGoal = false;
                spdlog::debug("[Leader] im goin explorin");
                break;
            }
        } else {
            leader.homeTimeAccum = 0.0f;
        }

        leader.idleWaitTimer -= dt;
        if (leader.idleWaitTimer <= 0.0f && HasArrived(nav)) {
            glm::vec3 wanderGoal = RandomPointAround(pos, leader.idleWanderRadius);
            SendTo(nav, wanderGoal);
            leader.idleWaitTimer = RandFloat(leader.idleWaitMin, leader.idleWaitMax);
            leader.hasActiveNavGoal = true;
        }
        break;
    }

    case LeaderState::Explore: {
        leader.exploreTimer -= dt;

        if (leader.exploreTimer <= 0.0f) {
            leader.state = LeaderState::WalkBackHome;
            nav.path.clear();
            leader.hasActiveNavGoal = false;
            SendTo(nav, leader.homePosition);
            spdlog::info("[Leader] im goin home");
            break;
        }

        if (HasArrived(nav) || !leader.hasActiveNavGoal) {
            glm::vec3 exploreGoal = RandomPointAround(leader.homePosition, leader.exploreRadius);
            SendTo(nav, exploreGoal);
            leader.hasActiveNavGoal = true;
            spdlog::info("[Leader] im explorin again)", exploreGoal.x, exploreGoal.z);
        }
        break;
    }

    case LeaderState::WalkBackHome: {
        float distFromHome = glm::length(
            glm::vec3(pos.x - leader.homePosition.x, 0, pos.z - leader.homePosition.z));

        if (distFromHome <= leader.homeRadius) {
            leader.homeTimeAccum = 0.0f;
            leader.state = LeaderState::Idle;
            nav.path.clear();
            leader.hasActiveNavGoal = false;
            spdlog::info("[Leader] im back home");
        } else if (!leader.hasActiveNavGoal || HasArrived(nav)) {
            SendTo(nav, leader.homePosition);
            leader.hasActiveNavGoal = true;
        }
        break;
    }

    case LeaderState::Escape: {
        leader.escapeTimer -= dt;

        bool stillSeeingPlayer = PlayersInRange(pos, leader.detectionRadius, playerPositions);
        if (!stillSeeingPlayer && leader.escapeTimer <= 0.0f) {
            leader.state = LeaderState::Idle;
            nav.path.clear();
            leader.hasActiveNavGoal = false;
            leader.idleWaitTimer = RandFloat(leader.idleWaitMin, leader.idleWaitMax);
            spdlog::info("[Leader] i escaped");
            break;
        }

        if (HasArrived(nav) && stillSeeingPlayer) {
            glm::vec3 furtherEscape = RandomPointAround(pos, leader.escapeRadius);
            SendTo(nav, furtherEscape);
            leader.hasActiveNavGoal = true;
        }
        break;
    }

    }
}

void NpcSystem::UpdateFollower(
    GameObject* go,
    TransformComponent& tr,
    NavPathComponent& nav,
    CockroachFollowerComponent& follower,
    float dt)
{
    if (!follower.leaderGameObject) return;

    auto* leaderGO = static_cast<GameObject*>(follower.leaderGameObject);
    auto* leaderTr = leaderGO->GetComponent<TransformComponent>();
    auto* leaderComp = leaderGO->GetComponent<CockroachLeaderComponent>();
    if (!leaderTr || !leaderComp) return;

    const glm::vec3 myPos     = tr.position;
    const glm::vec3 leaderPos = leaderTr->position;

    float distToLeader = glm::length(
        glm::vec3(leaderPos.x - myPos.x, 0, leaderPos.z - myPos.z));

    bool leaderIsIdle = (leaderComp->state == LeaderState::Idle);
    switch (follower.state) {

    case FollowerState::Follow: {
        if (leaderIsIdle && distToLeader <= follower.followDistance) {
            follower.state = FollowerState::Idle;
            follower.idleWaitTimer = RandFloat(follower.idleWaitMin, follower.idleWaitMax);
            nav.path.clear();
            follower.hasActiveNavGoal = false;
            break;
        }

        if (distToLeader > follower.followDistance) {
            if (!follower.hasActiveNavGoal || HasArrived(nav)) {
                glm::vec3 toLeader = leaderPos - myPos;
                toLeader.y = 0;
                float len = glm::length(toLeader);
                glm::vec3 goal = leaderPos;
                if (len > follower.followStopDistance) {
                    goal = leaderPos - (toLeader / len) * follower.followStopDistance;
                }
                SendTo(nav, goal);
                follower.hasActiveNavGoal = true;
            }
        } else if (HasArrived(nav)) {
            follower.hasActiveNavGoal = false;
        }
        break;
    }

    case FollowerState::Idle: {
        if (!leaderIsIdle || distToLeader > follower.followDistance * 2.0f) {
            follower.state = FollowerState::Follow;
            nav.path.clear();
            follower.hasActiveNavGoal = false;
            break;
        }
        follower.idleWaitTimer -= dt;
        if (follower.idleWaitTimer <= 0.0f && HasArrived(nav)) {
            glm::vec3 wanderGoal = RandomPointAround(leaderPos, follower.idleWanderRadius);
            SendTo(nav, wanderGoal);
            follower.idleWaitTimer = RandFloat(follower.idleWaitMin, follower.idleWaitMax);
            follower.hasActiveNavGoal = true;
        }
        break;
    }

    }
}

void NpcSystem::SendTo(NavPathComponent& nav, const glm::vec3& goal) {
    nav.goalPosition = goal;
    nav.state        = NavAgentState::RequestingPath;
    nav.path.clear();
    nav.currentWaypoint = 0;
}

bool NpcSystem::HasArrived(const NavPathComponent& nav) const {
    return nav.state == NavAgentState::Arrived || nav.state == NavAgentState::Idle;
}

bool NpcSystem::PlayersInRange(
    const glm::vec3& pos,
    float radius,
    const std::vector<glm::vec3>& playerPositions) const
{
    for (const auto& pp : playerPositions) {
        float dx = pp.x - pos.x;
        float dz = pp.z - pos.z;
        if (dx*dx + dz*dz <= radius*radius) return true;
    }
    return false;
}

glm::vec3 NpcSystem::RandomPointAround(const glm::vec3& center, float radius) const {
    float angle = RandFloat(0.0f, 2.0f * 3.14159265f);
    float dist  = RandFloat(0.0f, radius);
    glm::vec3 candidate = center + glm::vec3(
        std::cos(angle) * dist,
        0.0f,
        std::sin(angle) * dist
    );
    return ClampToNavMesh(candidate);
}

glm::vec3 NpcSystem::ClampToNavMesh(const glm::vec3& pos) const {
    if (!cachedNavMesh_) return pos;
    const NavMeshData& data = cachedNavMesh_->data;

    // Jeśli punkt jest na navmeshu – zwróć go
    if (cachedNavMesh_->FindTriangle(pos) >= 0) return pos;

    // Znajdź najbliższy centroid trójkąta
    float bestDist = std::numeric_limits<float>::max();
    glm::vec3 best = pos;
    for (const auto& tri : data.triangles) {
        if (!tri.walkable) continue;
        float dx = tri.centroid.x - pos.x;
        float dz = tri.centroid.z - pos.z;
        float d  = dx*dx + dz*dz;
        if (d < bestDist) { bestDist = d; best = tri.centroid; }
    }
    return best;
}

glm::vec3 NpcSystem::RandomPointOnNavMesh(const NavMeshData& data) const {
    std::vector<int> walkable;
    for (int i = 0; i < (int)data.triangles.size(); i++)
        if (data.triangles[i].walkable) walkable.push_back(i);
    if (walkable.empty()) return glm::vec3(0.0f);

    std::uniform_int_distribution<int> triDist(0, (int)walkable.size() - 1);
    int idx = walkable[triDist(GetRng())];
    const NavTriangle& tri = data.triangles[idx];

    float r1 = RandFloat(0,1), r2 = RandFloat(0,1);
    if (r1 + r2 > 1.0f) { r1 = 1-r1; r2 = 1-r2; }
    float r3 = 1.0f - r1 - r2;

    return r1 * data.vertices[tri.v[0]].position
         + r2 * data.vertices[tri.v[1]].position
         + r3 * data.vertices[tri.v[2]].position;
}
