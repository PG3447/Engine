#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include "core/ecs.h"
#include "core/component.h"
#include "systems/HID.h"
#include <algorithm>
#include <functional>
#include <spdlog/spdlog.h>

class MenuSystem : public System {
private:
    Query<SpriteComponent, MenuButtonComponent>* buttonQuery;
    Query<SpriteComponent, MenuSliderComponent>* sliderQuery;

    std::string activePanel = "main";

    int selectedNavIndex = 0;

    bool lastInputWasMouse = true;

    float navRepeatTimer = 0.0f;
    static constexpr float NAV_REPEAT_DELAY = 0.18f;

public:
    std::function<void(MenuAction)> onAction = nullptr;

    MenuSystem(ECS& ecs)
    {
        buttonQuery = ecs.CreateQuery<SpriteComponent, MenuButtonComponent>();
        sliderQuery = ecs.CreateQuery<SpriteComponent, MenuSliderComponent>();
    }

    void OnGameObjectUpdated(GameObject* e) override
    {
        buttonQuery->OnGameObjectUpdated(e);
        sliderQuery->OnGameObjectUpdated(e);
    }

    void SetActivePanel(const std::string& panel)
    {
        activePanel = panel;
        selectedNavIndex = 0;
        UpdatePanelVisibility();
    }

    const std::string& GetActivePanel() const { return activePanel; }

    void Update(ECS& ecs, float dt) override
    {
        HID* hid = ecs.GetSystem<HID>();
        if (!hid) return;

        UpdateButtons(ecs, hid, dt);
        UpdateSliders(ecs, hid, dt);
    }

private:
    void UpdatePanelVisibility()
    {
        auto& sprites = std::get<0>(buttonQuery->componentsVectors);
        auto& buttons = std::get<1>(buttonQuery->componentsVectors);

        for (size_t i = 0; i < buttonQuery->gameobjects.size(); i++)
        {
            if (!sprites[i] || !buttons[i]) continue;
            sprites[i]->isVisible = (buttons[i]->menuPanel == activePanel);
        }

        auto& sSprites = std::get<0>(sliderQuery->componentsVectors);
        auto& sliders  = std::get<1>(sliderQuery->componentsVectors);

        for (size_t i = 0; i < sliderQuery->gameobjects.size(); i++)
        {
            if (!sSprites[i] || !sliders[i]) continue;
            bool visible = (sliders[i]->menuPanel == activePanel);
            sSprites[i]->isVisible = visible;
            if (sliders[i]->fillObject)
            {
                auto* fillSprite = sliders[i]->fillObject->GetComponent<SpriteComponent>();
                if (fillSprite) fillSprite->isVisible = visible;
            }
        }
    }

    static bool PointInSprite(double mx, double my, const SpriteComponent* sprite)
    {
        // screenPosition to top-left (zgodnie z SpriteSystem::RenderSprite -> translate potem scale od (0,0))
        float left   = sprite->screenPosition.x;
        float top    = sprite->screenPosition.y;
        float right  = left + sprite->size.x;
        float bottom = top  + sprite->size.y;
        return mx >= left && mx <= right && my >= top && my <= bottom;
    }

    void UpdateButtons(ECS& ecs, HID* hid, float dt)
    {
        auto& sprites = std::get<0>(buttonQuery->componentsVectors);
        auto& buttons = std::get<1>(buttonQuery->componentsVectors);

        // Zbierz aktywne (widoczne w tym panelu) przyciski posortowane wg navIndex
        std::vector<std::pair<SpriteComponent*, MenuButtonComponent*>> active;
        for (size_t i = 0; i < buttonQuery->gameobjects.size(); i++)
        {
            if (!sprites[i] || !buttons[i]) continue;
            if (buttons[i]->menuPanel != activePanel) continue;
            active.emplace_back(sprites[i], buttons[i]);
        }
        std::sort(active.begin(), active.end(), [](auto& a, auto& b) {
            return a.second->navIndex < b.second->navIndex;
        });

        if (active.empty()) return;

        double mx = hid->get_mouse_x();
        double my = hid->get_mouse_y();
        bool mouseMoved = (hid->get_mouse_dx() != 0.0 || hid->get_mouse_dy() != 0.0);

        // --- Mysz: hover ---
        int hoveredIdx = -1;
        for (size_t i = 0; i < active.size(); i++)
        {
            if (!active[i].second->isEnabled) continue;
            if (PointInSprite(mx, my, active[i].first))
            {
                hoveredIdx = (int)i;
                break;
            }
        }

        if (mouseMoved && hoveredIdx >= 0)
        {
            lastInputWasMouse = true;
            selectedNavIndex = hoveredIdx;
        }

        // --- Pad / klawiatura: nawigacja gora/dol ---
        bool navDown = hid->is_action_just_pressed("ui_down");
        bool navUp   = hid->is_action_just_pressed("ui_up");

        if (navDown || navUp)
        {
            lastInputWasMouse = false;
            int dir = navDown ? 1 : -1;
            int count = (int)active.size();

            // przeskocz wyszarzone (isEnabled == false) przyciski
            for (int step = 0; step < count; step++)
            {
                selectedNavIndex = (selectedNavIndex + dir + count) % count;
                if (active[selectedNavIndex].second->isEnabled) break;
            }
        }

        if (selectedNavIndex < 0 || selectedNavIndex >= (int)active.size())
            selectedNavIndex = 0;

        // --- Ustaw wizualne stany ---
        for (size_t i = 0; i < active.size(); i++)
        {
            MenuButtonComponent* btn = active[i].second;
            SpriteComponent* spr = active[i].first;

            bool isCurrent = ((int)i == selectedNavIndex) &&
                              (lastInputWasMouse ? (hoveredIdx == (int)i) : true);

            btn->isSelected = isCurrent && btn->isEnabled;

            int state = MENU_BTN_NORMAL;
            if (!btn->isEnabled)
                state = MENU_BTN_NORMAL; // wyszarzony = normal tekstura (oczekujemy ze tekstura "normal" dla wylaczonych jest juz wyszarzona, patrz res/textures)
            else if (btn->isSelected)
                state = lastInputWasMouse ? MENU_BTN_HOVER : MENU_BTN_SELECTED;

            if (!spr->sprites.empty())
                spr->currentSprite = std::min(state, (int)spr->sprites.size() - 1);
        }

        // --- Potwierdzenie (klik myszki LMB lub ui_confirm na padzie/klawiaturze) ---
        bool confirmPressed = hid->is_action_just_pressed("ui_confirm");
        bool mouseClicked    = hid->is_action_just_pressed("menu_click");

        bool triggerConfirm = false;
        int  triggerIdx = -1;

        if (mouseClicked && hoveredIdx >= 0 && active[hoveredIdx].second->isEnabled)
        {
            triggerConfirm = true;
            triggerIdx = hoveredIdx;
        }
        else if (confirmPressed && !lastInputWasMouse && selectedNavIndex >= 0 &&
                 active[selectedNavIndex].second->isEnabled)
        {
            triggerConfirm = true;
            triggerIdx = selectedNavIndex;
        }

        if (triggerConfirm && triggerIdx >= 0)
        {
            MenuAction action = active[triggerIdx].second->action;
            spdlog::info("MenuSystem: wybrano akcje [{}] w panelu '{}'", (int)action, activePanel);
            HandleBuiltInNavigation(action);
            if (onAction) onAction(action);
        }

        // --- Back (Escape / ui_back) ---
        if (hid->is_action_just_pressed("ui_back") && activePanel != "main")
        {
            SetActivePanel("main");
        }
    }

    void HandleBuiltInNavigation(MenuAction action)
    {
        switch (action)
        {
            case MenuAction::OpenSettings:
                SetActivePanel("settings");
                break;
            case MenuAction::OpenAdditionalModes:
                SetActivePanel("additional_modes");
                break;
            case MenuAction::BackToMainMenu:
                SetActivePanel("main");
                break;
            default:
                break; // StartGame / ExitGame obslugiwane przez uzytkownika (onAction) w main.cpp
        }
    }

    void UpdateSliders(ECS& ecs, HID* hid, float dt)
    {
        auto& sprites = std::get<0>(sliderQuery->componentsVectors);
        auto& sliders = std::get<1>(sliderQuery->componentsVectors);

        double mx = hid->get_mouse_x();
        double my = hid->get_mouse_y();
        bool mouseHeld = hid->is_action_pressed("menu_click");
        bool mouseJustPressed = hid->is_action_just_pressed("menu_click");

        for (size_t i = 0; i < sliderQuery->gameobjects.size(); i++)
        {
            SpriteComponent* spr = sprites[i];
            MenuSliderComponent* slider = sliders[i];
            if (!spr || !slider) continue;
            if (slider->menuPanel != activePanel) continue;

            bool hovered = PointInSprite(mx, my, spr);

            // --- Mysz: klik + przeciaganie po pasku ---
            if (mouseJustPressed && hovered)
            {
                slider->isDragging = true;
                lastInputWasMouse = true;
            }
            if (!mouseHeld)
                slider->isDragging = false;

            if (slider->isDragging)
            {
                float t = (float)((mx - spr->screenPosition.x) / std::max(1.0f, spr->size.x));
                t = std::clamp(t, 0.0f, 1.0f);
                SetSliderValue(*slider, slider->minValue + t * (slider->maxValue - slider->minValue));
            }

            slider->isSelected = hovered || (slider->isSelected && !lastInputWasMouse);

            // --- Pad / klawiatura: lewo/prawo zmienia wartosc gdy slider ma "focus" ---
            // (focus sliderow w nawigacji padem obslugiwany jest razem z przyciskami poprzez
            //  wspolny navIndex - tutaj uproszczone: lewo/prawo dziala gdy kursor nad sliderem
            //  lub gdy slider jest jedynym elementem zaznaczonym w danym panelu)
            if (!lastInputWasMouse || hovered)
            {
                if (hid->is_action_just_pressed("ui_left"))
                    SetSliderValue(*slider, slider->value - slider->step);
                if (hid->is_action_just_pressed("ui_right"))
                    SetSliderValue(*slider, slider->value + slider->step);
            }

            // --- Aktualizacja wizualna paska wypelnienia ---
            if (slider->fillObject)
            {
                auto* fillSprite = slider->fillObject->GetComponent<SpriteComponent>();
                if (fillSprite)
                {
                    float range = (slider->maxValue - slider->minValue);
                    float t = range > 0.0001f ? (slider->value - slider->minValue) / range : 0.0f;
                    t = std::clamp(t, 0.0f, 1.0f);

                    fillSprite->size.x = slider->fillMaxWidth * t;
                    fillSprite->size.y = slider->fillBaseHeight;
                    fillSprite->screenPosition = spr->screenPosition; // wypelnienie rysowane od lewej krawedzi tla
                }
            }
        }
    }

    void SetSliderValue(MenuSliderComponent& slider, float newValue)
    {
        newValue = std::clamp(newValue, slider.minValue, slider.maxValue);
        if (newValue == slider.value) return;
        slider.value = newValue;
        if (slider.onValueChanged)
            slider.onValueChanged(slider.value);
    }
};

#endif //MENU_SYSTEM_H