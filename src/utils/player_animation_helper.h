#pragma once

#include "core/component.h"
#include "utils/animation_helper.h"
#include "prefab.h"
#include <spdlog/spdlog.h>

class PlayerAnimationHelper {
public:
    // Ustalone indeksy (grafik musi siê ich trzymaæ!)
    static constexpr int IDLE_ANIM_INDEX = 0;
    static constexpr int WALK_ANIM_INDEX = 1;
    static constexpr int PICKUP_ANIM_INDEX = 2; // Jednorazowa
    static constexpr int DROP_ANIM_INDEX = 3; // Jednorazowa
    static constexpr int IDLE_HOLD_INDEX = 4; // Z przedmiotem
    static constexpr int WALK_HOLD_INDEX = 5; // Z przedmiotem
    static constexpr int INTERACT_ANIM_INDEX = 6; // NOWA: Wciskanie/Otwieranie (Jednorazowa)

    // Wywo³anie jednorazowej akcji (Pickup/Drop)
    static void TriggerAction(AnimatorComponent* animator, Prefab* playerPrefab, int actionIndex) {
        if (!animator || !playerPrefab || !playerPrefab->rootModel) return;
        auto& animations = playerPrefab->rootModel->animations;

        if (animations.size() <= actionIndex) {
            spdlog::warn("Brak animacji o indeksie {} w modelu gracza!", actionIndex);
            return;
        }

        AnimationClip* targetClip = &animations[actionIndex];

        // Odpalamy akcjê raz (loop = false). Dajemy jej pe³ny priorytet.
        AnimationHelper::Play(animator, targetClip, false, 1.0f);
    }

    // Aktualizacja co klatkê (zabezpieczona przed przerwaniem Pickup/Drop)
    static void UpdateAnimation(AnimatorComponent* animator, Prefab* playerPrefab, bool isMoving, bool isHoldingObject) {
        if (!animator || !playerPrefab || !playerPrefab->rootModel) return;
        auto& animations = playerPrefab->rootModel->animations;

        if (animations.size() <= WALK_ANIM_INDEX) return;

        // --- SYSTEM PRIORYTETÓW ---
        // Jeœli aktualna animacja to jednorazowa akcja (loop == false) i jeszcze trwa (!isFinished),
        // to przerywamy funkcjê. Dziêki temu kod ruchu nie nadpisze animacji podnoszenia!
        if (animator->currentAnimation != nullptr && !animator->looping && !animator->isFinished) {
            return;
        }

        // Kiedy akcja siê skoñczy, wracamy do p³ynnego dobierania stanu bazowego
        AnimationClip* targetClip = nullptr;

        if (isHoldingObject) {
            // Jeœli ma za ma³o animacji na hold, dla bezpieczeñstwa wracamy do zwyk³ych
            if (animations.size() > WALK_HOLD_INDEX) {
                targetClip = isMoving ? &animations[WALK_HOLD_INDEX] : &animations[IDLE_HOLD_INDEX];
            }
            else {
                targetClip = isMoving ? &animations[WALK_ANIM_INDEX] : &animations[IDLE_ANIM_INDEX];
            }
        }
        else {
            targetClip = isMoving ? &animations[WALK_ANIM_INDEX] : &animations[IDLE_ANIM_INDEX];
        }

        // Odpalamy zapêtlon¹ animacjê bazow¹ (loop = true). 
        AnimationHelper::Play(animator, targetClip, true, 1.0f);
    }
};