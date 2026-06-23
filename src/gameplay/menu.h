#ifndef MENU_H
#define MENU_H

class Menu
{
private:
	SceneManager* sceneManager;
	Scene* scenaMenu;
	std::unique_ptr<Prefab> modelTest;

public:

	Menu(SceneManager* manager, Scene* menu) : sceneManager(manager), scenaMenu(menu)
	{
		
	}

	void Init()
	{
	/*	GameObject* menuobjekt = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* jakisprite = menuobjekt->AddComponent<SpriteComponent>();
		jakisprite->sprites = { ResourceManager::LoadTexture("diffuse_brick.png", "res/textures/").id };
		jakisprite->screenPosition = glm::vec2(480.0f - 16.0f, 540.0f - 16.0f); // centrum - half size
		jakisprite->size = glm::vec2(160.0f, 160.0f);
		jakisprite->layer = 2; // nad napisami
		jakisprite->isVisible = true;

		UIButtonComponent* button = menuobjekt->AddComponent<UIButtonComponent>();


		button->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->size = glm::vec2(280.0f, 280.0f);

			spdlog::info("Hover enter");
		};

		button->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->size = glm::vec2(160.0f, 160.0f);

			spdlog::info("Hover exit");
		};

		button->onClick = [&](GameObject* go)
		{
			sceneManager->ChangeScene("Scena 1");
			spdlog::info("Przycisk klikniety!");
		};*/

		GameObject* cameraMenu = scenaMenu->CreateGameObject(nullptr);//groundModel->Instantiate(*scena1, nullptr, ourShader.get());
		cameraMenu->name = "Kamera";
		cameraMenu->AddComponent<CameraComponent>();

		modelTest = std::make_unique<Prefab>("res/models/podloze.glb");
		GameObject* menuPodloze = modelTest->Instantiate(*scenaMenu, nullptr, nullptr);


		//tlo

		GameObject* BG_Object = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* BG_SPRITE = BG_Object->AddComponent<SpriteComponent>();
		for (int i = 86400; i <= 86518; i++)
		{
			char filename[64];
			sprintf(filename, "menu_final%08d.png", i);
			auto tex = ResourceManager::LoadTexture(filename, "res/sprites/menu/BG/");
			BG_SPRITE->sprites.push_back(tex.id);
		}
		BG_SPRITE->screenPosition = glm::vec2(0.0f, 0.0f);
		BG_SPRITE->size = glm::vec2(1920.0f, 1080.0f);
		BG_SPRITE->layer = 0;
		BG_SPRITE->isVisible = true;
		BG_SPRITE->isAnimating = true;
		BG_SPRITE->loop = true;
		BG_SPRITE->frameDuration = 1.0f / 24.0f;
		UIButtonComponent* button = BG_Object->AddComponent<UIButtonComponent>();


	}


};
#endif