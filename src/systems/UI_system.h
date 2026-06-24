#ifndef UI_SYSTEM_H
#define UI_SYSTEM_H


class UISystem : public System {
private:
    ECS& uiECS;
    HID& hidSystem;

    Query<TransformComponent, SpriteComponent, UIButtonComponent>* buttonsQuery;
    Query<TransformComponent, SpriteComponent, UISliderComponent>* slidersQuery;

public:
    UISystem(ECS& ecs, HID& hid) : uiECS(ecs), hidSystem(hid)
    {
        buttonsQuery = ecs.CreateQuery<TransformComponent, SpriteComponent, UIButtonComponent>();
        slidersQuery = ecs.CreateQuery<TransformComponent, SpriteComponent, UISliderComponent>();


        Init();
    }

    void Init()
    {
    }

    void OnGameObjectUpdated(GameObject* e) override
    {
        buttonsQuery->OnGameObjectUpdated(e);
        slidersQuery->OnGameObjectUpdated(e);
    }

    void Update(ECS& ecs, float dt) override
    {

        float mouseX = (float)hidSystem.get_mouse_x();
        float mouseY = (float)hidSystem.get_mouse_y();

        auto& gameObjectButtons = buttonsQuery->gameobjects;

        auto& sprites = std::get<1>(buttonsQuery->componentsVectors);
        auto& buttons = std::get<2>(buttonsQuery->componentsVectors);

        for (size_t i = 0; i < gameObjectButtons.size(); i++)
        {
            auto* sprite = sprites[i];
            auto* button = buttons[i];

            glm::vec2 pos = { sprite->screenPosition.x, sprite->screenPosition.y };

            glm::vec2 size = { sprite->size.x, sprite->size.y };

            bool inside = mouseX >= pos.x &&
                          mouseX <= pos.x + size.x &&
                          mouseY >= pos.y &&
                          mouseY <= pos.y + size.y;

            if (inside && !button->isHovered)
            {
                button->isHovered = true;

                if (button->onHoverEnter)
                    button->onHoverEnter(gameObjectButtons[i]);
            }

            if (!inside && button->isHovered)
            {
                button->isHovered = false;

                if (button->onHoverExit)
                    button->onHoverExit(gameObjectButtons[i]);
            }

            if (inside && hidSystem.is_action_just_pressed("ui_click"))
            {
                button->isPressed = true;
            }

            if (!button->isEnabled) {
                continue;
            }

            if (button->isPressed && hidSystem.is_action_just_released("ui_click"))
            {
                button->isPressed = false;

                if (inside && button->onClick)
                    button->onClick(gameObjectButtons[i]);
            }
        }


        auto& gameObjectSliders = slidersQuery->gameobjects;

        auto& sliderSprites = std::get<1>(slidersQuery->componentsVectors);
        auto& sliders = std::get<2>(slidersQuery->componentsVectors);

        for (size_t i = 0; i < gameObjectSliders.size(); i++)
        {
            auto* sprite = sliderSprites[i];
            auto* slider = sliders[i];

            glm::vec2 pos = { sprite->screenPosition.x, sprite->screenPosition.y };
            glm::vec2 size = { sprite->size.x, sprite->size.y };

            bool inside = mouseX >= pos.x &&
                          mouseX <= pos.x + size.x &&
                          mouseY >= pos.y &&
                          mouseY <= pos.y + size.y;

            if (inside && hidSystem.is_action_just_pressed("ui_click"))
                slider->isDragging = true;

            if (hidSystem.is_action_just_released("ui_click"))
                slider->isDragging = false;


            if (slider->isDragging)
            {
                float t = (mouseX - pos.x) / size.x;
                t = glm::clamp(t, 0.0f, 1.0f);

                float newValue = slider->minValue + t * (slider->maxValue - slider->minValue);

                if (newValue != slider->value)
                {
                    slider->value = newValue;
                    if (slider->onValueChanged)
                        slider->onValueChanged(slider->value);
                }
            }
        }
    };
};

#endif