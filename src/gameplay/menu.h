#ifndef MENU_H
#define MENU_H

class Menu
{
private:
	SceneManager* sceneManager;
	Scene* scenaMenu;
	std::unique_ptr<Prefab> modelTest;
	GLFWwindow* window;
	std::vector<GameObject*> grp_main;
	std::vector<GameObject*> grp_pause;
	std::vector<GameObject*> grp_settings;
	std::vector<GameObject*> grp_credits;
	std::vector<GameObject*> grp_load;

public:

	Menu(SceneManager* manager, Scene* menu, GLFWwindow* windoww) : sceneManager(manager), scenaMenu(menu), window(windoww)
	{

	}

	void ShowOnly(std::vector<GameObject*>* group)
	{
		std::vector<std::vector<GameObject*>*> all = {
			&grp_main, &grp_settings, &grp_credits, &grp_load, &grp_pause
		};
		for (auto* g : all)
		{
			bool visible = (g == group);
			for (auto* go : *g)
			{
				if (auto* s = go->GetComponent<SpriteComponent>())
					s->isVisible = visible;
				if (auto* b = go->GetComponent<UIButtonComponent>())
					b->isEnabled = visible;
			}
		}
	}

	void Init() {
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


		//MAIN
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

		GameObject* Logo_Object = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Logo_SPRITE = Logo_Object->AddComponent<SpriteComponent>();
		Logo_SPRITE->sprites = {ResourceManager::LoadTexture("logo.png", "res/sprites/menu/").id };
		Logo_SPRITE->screenPosition = glm::vec2(123.0f, 45.9f);
		Logo_SPRITE->size = glm::vec2(458.0f, 316.0f);
		Logo_SPRITE->layer = 0;
		Logo_SPRITE->isVisible = true;
		UIButtonComponent* button2 = Logo_Object->AddComponent<UIButtonComponent>();


		GameObject* Button_Object_1 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Start_sprite = Button_Object_1->AddComponent<SpriteComponent>();
		Start_sprite->sprites = {ResourceManager::LoadTexture("new_game_z_sprite.png", "res/sprites/menu/").id, ResourceManager::LoadTexture("newgame_hover.png", "res/sprites/menu/").id};
		Start_sprite->screenPosition = glm::vec2(126.7f, 429.0f);
		Start_sprite->size = glm::vec2(473.0f, 108.0f);
		Start_sprite->layer = 1;
		Start_sprite->isVisible = true;
		UIButtonComponent* button3 = Button_Object_1->AddComponent<UIButtonComponent>();

		GameObject* Button_Object_2 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Load_sprite = Button_Object_2->AddComponent<SpriteComponent>();
		Load_sprite->sprites = {ResourceManager::LoadTexture("load_game_z_sprite.png", "res/sprites/menu/").id, ResourceManager::LoadTexture("loadgame_hover.png", "res/sprites/menu/").id };
		Load_sprite->screenPosition = glm::vec2(126.7f, 548.0f);
		Load_sprite->size = glm::vec2(473.0f, 108.0f);
		Load_sprite->layer = 1;
		Load_sprite->isVisible = true;
		UIButtonComponent* button4 = Button_Object_2->AddComponent<UIButtonComponent>();

		GameObject* Button_Object_3 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Settings_sprite = Button_Object_3->AddComponent<SpriteComponent>();
		Settings_sprite->sprites = {ResourceManager::LoadTexture("settings_z_sprite.png", "res/sprites/menu/").id, ResourceManager::LoadTexture("settings_hover.png", "res/sprites/menu/").id };
		Settings_sprite->screenPosition = glm::vec2(126.7f, 666.8f);
		Settings_sprite->size = glm::vec2(473.0f, 108.0f);
		Settings_sprite->layer = 1;
		Settings_sprite->isVisible = true;
		UIButtonComponent* button5 = Button_Object_3->AddComponent<UIButtonComponent>();

		GameObject* Button_Object_4 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Credits_sprite = Button_Object_4->AddComponent<SpriteComponent>();
		Credits_sprite->sprites = {ResourceManager::LoadTexture("credits_z_sprite.png", "res/sprites/menu/").id, ResourceManager::LoadTexture("credits_hover.png", "res/sprites/menu/").id };
		Credits_sprite->screenPosition = glm::vec2(126.7f, 788.5);
		Credits_sprite->size = glm::vec2(473.0f, 108.0f);
		Credits_sprite->layer = 1;
		Credits_sprite->isVisible = true;
		UIButtonComponent* button6 = Button_Object_4->AddComponent<UIButtonComponent>();

		GameObject* Button_Object_5 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Exit_sprite = Button_Object_5->AddComponent<SpriteComponent>();
		Exit_sprite->sprites = {ResourceManager::LoadTexture("exit_z_sprite.png", "res/sprites/menu/").id, ResourceManager::LoadTexture("exit_hover.png", "res/sprites/menu/").id };
		Exit_sprite->screenPosition = glm::vec2(126.7f, 907.7f);
		Exit_sprite->size = glm::vec2(473.0f, 108.0f);
		Exit_sprite->layer = 1;
		Exit_sprite->isVisible = true;
		UIButtonComponent* button7 = Button_Object_5->AddComponent<UIButtonComponent>();

		button3->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button3->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		button4->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button4->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		button5->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button5->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};


		button6->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button6->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		button7->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button7->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		//AMON GUS


		button3->onClick = [&](GameObject* go)
		{
			//sceneManager->ChangeScene("Scena 1"); //change
			ShowOnly(&grp_pause); //for testing only
		};

		button4->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_load); //change
		};

		button5->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_settings); //change
		};

		button6->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_credits); //change
		};

		button7->onClick = [&](GameObject* go)
		{
			glfwSetWindowShouldClose(window, true);
		};

		grp_main.push_back(BG_Object);
		grp_main.push_back(Logo_Object);
		grp_main.push_back(Button_Object_1);
		grp_main.push_back(Button_Object_2);
		grp_main.push_back(Button_Object_3);
		grp_main.push_back(Button_Object_4);
		grp_main.push_back(Button_Object_5);


		//CREDITS
		GameObject* BG_Object_2 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* BG_sprite_credits = BG_Object_2->AddComponent<SpriteComponent>();
		for (int i = 86400; i <= 86518; i++)
		BG_sprite_credits->sprites = {ResourceManager::LoadTexture("credits.png", "res/sprites/menu/credits").id };
		BG_sprite_credits->screenPosition = glm::vec2(0.0f, 0.0f);
		BG_sprite_credits->size = glm::vec2(1920.0f, 1080.0f);
		BG_sprite_credits->layer = 0;
		BG_sprite_credits->isVisible = true;
		UIButtonComponent* bg_button_2 = BG_Object_2->AddComponent<UIButtonComponent>();

		GameObject* credits_back = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* credits_back_sprite = credits_back->AddComponent<SpriteComponent>();
		credits_back_sprite->sprites = {ResourceManager::LoadTexture("back_sprite.png", "res/sprites/menu/credits").id, ResourceManager::LoadTexture("back_sprite_hover.png", "res/sprites/menu/credits").id};
		credits_back_sprite->screenPosition = glm::vec2(1375.7f, 871.8f);
		credits_back_sprite->size = glm::vec2(473.0f, 108.0f);
		credits_back_sprite->layer = 1;
		credits_back_sprite->isVisible = true;
		UIButtonComponent* button_credits = credits_back->AddComponent<UIButtonComponent>();


		button_credits->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button_credits->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		button_credits->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_main);
		};

		grp_credits.push_back(BG_Object_2);
		grp_credits.push_back(credits_back);

		//load




		//settings
		GameObject* settings_back = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* settings_back_sprite = settings_back->AddComponent<SpriteComponent>();
		settings_back_sprite->sprites = {ResourceManager::LoadTexture("settings.png", "res/sprites/menu/settings").id};
		settings_back_sprite->screenPosition = glm::vec2(1375.7f, 871.8f);
		settings_back_sprite->size = glm::vec2( 1920.0f, 1080.0f);
		settings_back_sprite->layer = 1;
		settings_back_sprite->isVisible = true;
		UIButtonComponent* settings_button = credits_back->AddComponent<UIButtonComponent>();

		settings_button->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_main);
		};

		grp_settings.push_back(settings_back);

		ShowOnly(&grp_main);
	}


};
#endif