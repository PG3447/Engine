#include "core/component.h"
#include "systems/MenuSystem.h"

static GameObject* CreateMenuButton(
    Scene& scene,
    const std::string& name,
    const std::string& texNormal,
    const std::string& texHover,
    const std::string& texSelected,
    glm::vec2 screenPos,
    glm::vec2 size,
    MenuAction action,
    int navIndex,
    const std::string& panel,
    bool enabled = true)
{
    GameObject* go = scene.CreateGameObject(nullptr);
    go->name = name;

    SpriteComponent* sprite = go->AddComponent<SpriteComponent>();
    sprite->sprites = {
        ResourceManager::LoadTexture(texNormal,   "res/textures/").id,
        ResourceManager::LoadTexture(texHover,    "res/textures/").id,
        ResourceManager::LoadTexture(texSelected, "res/textures/").id,
    };
    sprite->currentSprite  = 0;
    sprite->screenPosition = screenPos;
    sprite->size            = size;
    sprite->layer           = 1;
    sprite->isVisible       = (panel == "main"); // domyslnie tylko panel main widoczny

    MenuButtonComponent* btn = go->AddComponent<MenuButtonComponent>();
    btn->action     = action;
    btn->navIndex   = navIndex;
    btn->menuPanel  = panel;
    btn->isEnabled  = enabled;

    return go;
}

// Pomocnicza funkcja tworzaca slider (tlo + wypelnienie + tekst etykiety)
static GameObject* CreateMenuSlider(
    Scene& scene,
    const std::string& name,
    const std::string& label,
    glm::vec2 screenPos,
    glm::vec2 bgSize,
    float minValue, float maxValue, float startValue,
    int navIndex,
    const std::string& panel,
    std::function<void(float)> onChanged)
{
    // Tlo paska
    GameObject* bgObj = scene.CreateGameObject(nullptr);
    bgObj->name = name + "_bg";
    SpriteComponent* bgSprite = bgObj->AddComponent<SpriteComponent>();
    bgSprite->sprites = { ResourceManager::LoadTexture("slider_bg.png", "res/sprites/menu/").id };
    bgSprite->screenPosition = screenPos;
    bgSprite->size = bgSize;
    bgSprite->layer = 1;
    bgSprite->isVisible = (panel == "settings");

    // Etykieta tekstowa nad/po lewej paska
    bgSprite->textEnabled        = true;
    bgSprite->text               = label;
    bgSprite->textCentered       = false;
    bgSprite->textOffset         = glm::vec2(0.0f, -32.0f); // nad paskiem
    bgSprite->textOutlineEnabled = true;

    // Wypelnienie paska (osobny GameObject zeby moc niezaleznie skalowac sprite->size)
    GameObject* fillObj = scene.CreateGameObject(nullptr);
    fillObj->name = name + "_fill";
    SpriteComponent* fillSprite = fillObj->AddComponent<SpriteComponent>();
    fillSprite->sprites = { ResourceManager::LoadTexture("slider_fill.png", "res/textures/menu/").id };
    fillSprite->screenPosition = screenPos;
    fillSprite->size = bgSize;
    fillSprite->layer = 2;
    fillSprite->isVisible = (panel == "settings");

    MenuSliderComponent* slider = bgObj->AddComponent<MenuSliderComponent>();
    slider->label         = label;
    slider->minValue       = minValue;
    slider->maxValue       = maxValue;
    slider->value          = startValue;
    slider->fillObject     = fillObj;
    slider->fillMaxWidth   = bgSize.x;
    slider->fillBaseHeight = bgSize.y;
    slider->navIndex       = navIndex;
    slider->menuPanel      = panel;
    slider->onValueChanged = onChanged;

    return bgObj;
}

inline void CreateMenuScene(Scene* menu, AudioSystem* audioSys)
{
    menu->GetECS().AddSystem<MenuSystem>(menu->GetECS());
    MenuSystem* menuSystem = menu->GetECS().GetSystem<MenuSystem>();

    GameObject* background = menu->CreateGameObject(nullptr);
    background->name = "MenuBackground";
    SpriteComponent* bgSprite = background->AddComponent<SpriteComponent>();
    bgSprite->sprites = { ResourceManager::LoadTexture("menu_background.png", "res/textures/menu/").id };
    bgSprite->screenPosition = glm::vec2(0.0f, 0.0f);
    bgSprite->size = glm::vec2(1920.0f, 1080.0f); // pelny ekran, dopasuj jesli zmienisz WINDOW_WIDTH/HEIGHT
    bgSprite->layer = 0;
    bgSprite->isVisible = true;

    const glm::vec2 btnSize(420.0f, 90.0f);
    const float startX = (1920.0f - btnSize.x) * 0.5f;
    const float startY = 380.0f;
    const float gapY   = 120.0f;

    CreateMenuButton(*menu, "Btn_Start",
        "menu/btn_start_normal.png", "menu/btn_start_hover.png", "menu/btn_start_selected.png",
        glm::vec2(startX, startY + gapY * 0), btnSize,
        MenuAction::StartGame, 0, "main");

    CreateMenuButton(*menu, "Btn_Settings",
        "menu/btn_settings_normal.png", "menu/btn_settings_hover.png", "menu/btn_settings_selected.png",
        glm::vec2(startX, startY + gapY * 1), btnSize,
        MenuAction::OpenSettings, 1, "main");

    CreateMenuButton(*menu, "Btn_AdditionalModes",
        "menu/btn_additional_modes_normal.png", "menu/btn_additional_modes_normal.png", "menu/btn_additional_modes_normal.png",
        glm::vec2(startX, startY + gapY * 2), btnSize,
        MenuAction::OpenAdditionalModes, 2, "main",
        /*enabled=*/false); // wyszarzone - uzywamy tej samej (wyszarzonej) tekstury we wszystkich stanach

    CreateMenuButton(*menu, "Btn_Exit",
        "menu/btn_exit_normal.png", "menu/btn_exit_hover.png", "menu/btn_exit_selected.png",
        glm::vec2(startX, startY + gapY * 3), btnSize,
        MenuAction::ExitGame, 3, "main");

    const float sliderX = (1920.0f - 400.0f) * 0.5f;
    const float sliderStartY = 360.0f;
    const float sliderGapY = 140.0f;
    const glm::vec2 sliderSize(400.0f, 24.0f);

    CreateMenuSlider(*menu, "Slider_Volume", "Glosnosc",
        glm::vec2(sliderX, sliderStartY + sliderGapY * 0), sliderSize,
        0.0f, 1.0f, 1.0f, 0, "settings",
        [audioSys](float v) {
            spdlog::info("Menu: glosnosc ustawiona na {:.2f}", v);
            (void)audioSys;
        });

    CreateMenuSlider(*menu, "Slider_Gamma", "Gamma",
        glm::vec2(sliderX, sliderStartY + sliderGapY * 1), sliderSize,
        0.1f, 3.0f, 1.0f, 1, "settings",
        [](float v) {
            spdlog::info("Menu: gamma ustawiona na {:.2f}", v);
        });

    CreateMenuSlider(*menu, "Slider_Contrast", "Kontrast",
        glm::vec2(sliderX, sliderStartY + sliderGapY * 2), sliderSize,
        0.0f, 2.0f, 1.0f, 2, "settings",
        [](float v) {
            spdlog::info("Menu: kontrast ustawiony na {:.2f}", v);
        });

    CreateMenuButton(*menu, "Btn_SettingsBack",
        "menu/btn_back_normal.png", "menu/btn_back_hover.png", "menu/btn_back_selected.png",
        glm::vec2(sliderX, sliderStartY + sliderGapY * 3 + 40.0f), glm::vec2(200.0f, 70.0f),
        MenuAction::BackToMainMenu, 3, "settings");

    CreateMenuButton(*menu, "Btn_AdditionalModesBack",
        "menu/btn_back_normal.png", "menu/btn_back_hover.png", "menu/btn_back_selected.png",
        glm::vec2((1920.0f - 200.0f) * 0.5f, 900.0f), glm::vec2(200.0f, 70.0f),
        MenuAction::BackToMainMenu, 0, "additional_modes");

    menuSystem->SetActivePanel("main");
}
