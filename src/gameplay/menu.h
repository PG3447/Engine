#ifndef MENU_H
#define MENU_H

class Menu
{
private:
	Scene* scenaMenu;
	std::unique_ptr<Prefab> modelTest;

public:

	Menu(Scene* menu) : scenaMenu(menu)
	{
		
	}

	void Init()
	{
		GameObject* menuobjekt = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* jakisprite = menuobjekt->AddComponent<SpriteComponent>();
		jakisprite->sprites = { ResourceManager::LoadTexture("diffuse_brick.png", "res/textures/").id };
		jakisprite->screenPosition = glm::vec2(480.0f - 16.0f, 540.0f - 16.0f); // centrum - half size
		jakisprite->size = glm::vec2(160.0f, 160.0f);
		jakisprite->layer = 2; // nad napisami
		jakisprite->isVisible = true;

		UIButtonComponent* jakisbutton = menuobjekt->AddComponent<UIButtonComponent>();


		jakisbutton->onHoverEnter = [](GameObject* go)
			{
				auto* sprite = go->GetComponent<SpriteComponent>();
				if (!sprite) return;

				sprite->size = glm::vec2(280.0f, 280.0f);

				spdlog::info("Hover enter");
			};

		jakisbutton->onHoverExit = [](GameObject* go)
			{
				auto* sprite = go->GetComponent<SpriteComponent>();
				if (!sprite) return;

				sprite->size = glm::vec2(160.0f, 160.0f);

				spdlog::info("Hover exit");
			};

		jakisbutton->onClick = [](GameObject* go)
			{
				spdlog::info("Przycisk klikniety!");
			};

		GameObject* cameraMenu = scenaMenu->CreateGameObject(nullptr);//groundModel->Instantiate(*scena1, nullptr, ourShader.get());
		cameraMenu->name = "Kamera";
		cameraMenu->AddComponent<CameraComponent>();

		modelTest = std::make_unique<Prefab>("res/models/podloze.glb");
		GameObject* menuPodloze = modelTest->Instantiate(*scenaMenu, nullptr, nullptr);

	}


};
#endif