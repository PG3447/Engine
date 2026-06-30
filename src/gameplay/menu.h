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
	std::vector<GameObject*> grp_cutscene_1;
	std::vector<GameObject*> grp_cutscene_2;

public:
	SpriteComponent* Cutscene_2_sprite = nullptr;
	bool reset = false;


	Menu(SceneManager* manager, Scene* menu, GLFWwindow* windoww) : sceneManager(manager), scenaMenu(menu), window(windoww)
	{

	}

	void ShowOnly(std::vector<GameObject*>* group)
	{
		//spdlog::critical("ShowOnly wywolane, grupa ma {} obiektow", group->size());
		std::vector<std::vector<GameObject*>*> all = {
			&grp_main, &grp_settings, &grp_credits, &grp_load, &grp_pause, &grp_cutscene_1, &grp_cutscene_2
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

	void ShowOutro() {
		sceneManager->ChangeScene("menu");
		Cutscene_2_sprite->currentSprite = 0;
		ShowOnly(&grp_cutscene_2);
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


		//cutscene
		GameObject* Cutscene_1_Object = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Cutscene_1_sprite = Cutscene_1_Object->AddComponent<SpriteComponent>();
		for (int i = 1; i <= 17; i++)
		{
			char filename[64];
			sprintf(filename, "%02d.jpg", i);
			auto tex = ResourceManager::LoadTexture(filename, "res/sprites/cutscenes/intro");
			Cutscene_1_sprite->sprites.push_back(tex.id);
		}
		Cutscene_1_sprite->screenPosition = glm::vec2(0.0f, 0.0f);
		Cutscene_1_sprite->size = glm::vec2(1940.0f, 1100.0f);;
		Cutscene_1_sprite->layer = 0;
		Cutscene_1_sprite->isVisible = true;
		Cutscene_1_sprite->isAnimating = true;
		Cutscene_1_sprite->loop = false;
		Cutscene_1_sprite->frameDuration = 2.0f;


		GameObject* cutscene_1_object_procceed = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* cutscene_procced_sprite = cutscene_1_object_procceed->AddComponent<SpriteComponent>();
		cutscene_procced_sprite->sprites = {ResourceManager::LoadTexture("nothing.png", "res/sprites/cutscenes/intro").id, ResourceManager::LoadTexture("press_t_c.png", "res/sprites/cutscenes/intro").id};
		cutscene_procced_sprite->screenPosition = glm::vec2(1375.7f, 850.0f);
		cutscene_procced_sprite->size = glm::vec2(471.0f, 106.0f);
		cutscene_procced_sprite->layer = 1;
		cutscene_procced_sprite->isVisible = true;
		cutscene_procced_sprite->currentSprite = 0;
		cutscene_procced_sprite->isAnimating = true;
		cutscene_procced_sprite->loop = false;
		cutscene_procced_sprite->frameDuration = 34.0f;
		UIButtonComponent* Cutscene_1_button = cutscene_1_object_procceed->AddComponent<UIButtonComponent>();

		Cutscene_1_button->onClick = [&](GameObject* go)
		{
				ShowOnly(&grp_pause);
				reset = true;
				sceneManager->ChangeScene("Scena 1");
		};

		grp_cutscene_1.push_back(Cutscene_1_Object);
		grp_cutscene_1.push_back(cutscene_1_object_procceed);

		//Outro
		GameObject* Cutscene_2_Object = scenaMenu->CreateGameObject(nullptr);
		Cutscene_2_sprite = Cutscene_2_Object->AddComponent<SpriteComponent>();
		for (int i = 1; i <= 7; i++)
		{
			char filename[64];
			sprintf(filename, "%01d.png", i);
			auto tex = ResourceManager::LoadTexture(filename, "res/sprites/cutscenes/outro");
			Cutscene_2_sprite->sprites.push_back(tex.id);
		}
		Cutscene_2_sprite->screenPosition = glm::vec2(-10.0f, -10.0f);
		Cutscene_2_sprite->size = glm::vec2( 1940.0f, 1100.0f);
		Cutscene_2_sprite->layer = 0;
		Cutscene_2_sprite->isVisible = true;
		Cutscene_2_sprite->isAnimating = true;
		Cutscene_2_sprite->loop = false;
		Cutscene_2_sprite->frameDuration = 2.0f;


		GameObject* cutscene_2_object_procceed = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* cutscene_2_procced_sprite = cutscene_2_object_procceed->AddComponent<SpriteComponent>();
		cutscene_2_procced_sprite->sprites = {ResourceManager::LoadTexture("nothing.png", "res/sprites/cutscenes/intro").id};
		cutscene_2_procced_sprite->screenPosition = glm::vec2(960.0f, 0.0f);
		cutscene_2_procced_sprite->size = glm::vec2( 1940.0f, 1100.0f);
		cutscene_2_procced_sprite->layer = 1;
		cutscene_2_procced_sprite->isVisible = true;
		UIButtonComponent* Cutscene_2_button = cutscene_2_object_procceed->AddComponent<UIButtonComponent>();

		Cutscene_2_button->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_main);
		};


		grp_cutscene_2.push_back(Cutscene_2_Object);
		grp_cutscene_2.push_back(cutscene_2_object_procceed);



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
		BG_SPRITE->screenPosition = glm::vec2(-10.0f, -10.0f);
		BG_SPRITE->size = glm::vec2(1940.0f, 1100.0f);
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


		button3->onClick = [this, Cutscene_1_sprite, cutscene_procced_sprite](GameObject* go)
		{
			//sceneManager->ChangeScene("Scena 1"); //change
			cutscene_procced_sprite->currentSprite = 0;
			Cutscene_1_sprite->currentSprite = 0;
			ShowOnly(&grp_cutscene_1); //for testing only

		};

		button4->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_load); //change
		};

		button5->onClick = [&](GameObject* go)
		{
			//ShowOnly(&grp_settings); //jebac
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
		BG_sprite_credits->sprites = {ResourceManager::LoadTexture("credits.png", "res/sprites/menu/credits").id };
		BG_sprite_credits->screenPosition = glm::vec2(-10.0f, -10.0f);
		BG_sprite_credits->size = glm::vec2(1940.0f, 1100.0f);
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

		GameObject* Load_background_object = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Load_background_sprite = Load_background_object->AddComponent<SpriteComponent>();
		Load_background_sprite->sprites = {ResourceManager::LoadTexture("tlo.png", "res/sprites/menu/load").id};
		Load_background_sprite->screenPosition = glm::vec2(-10.0f, -10.0f);
		Load_background_sprite->size = glm::vec2( 1940.0f, 1100.0f);
		Load_background_sprite->layer = 0;
		Load_background_sprite->isVisible = true;

		GameObject* Load_Slot_Object_1 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Load_Slot_Sprite_1 = Load_Slot_Object_1->AddComponent<SpriteComponent>();
		Load_Slot_Sprite_1->sprites = {ResourceManager::LoadTexture("slot.png", "res/sprites/menu/load").id, ResourceManager::LoadTexture("slot_hover.png", "res/sprites/menu/load").id};
		Load_Slot_Sprite_1->screenPosition = glm::vec2(325.0f, 156.0f);
		Load_Slot_Sprite_1->size = glm::vec2(1266.0f, 75.0f);
		Load_Slot_Sprite_1->layer = 1;
		Load_Slot_Sprite_1->isVisible = true;
		UIButtonComponent* slot_button_1 = Load_Slot_Object_1->AddComponent<UIButtonComponent>();

		GameObject* Load_Slot_Object_2 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Load_Slot_Sprite_2 = Load_Slot_Object_2->AddComponent<SpriteComponent>();
		Load_Slot_Sprite_2->sprites = {ResourceManager::LoadTexture("slot.png", "res/sprites/menu/load").id, ResourceManager::LoadTexture("slot_hover.png", "res/sprites/menu/load").id};
		Load_Slot_Sprite_2->screenPosition = glm::vec2(325.0f, 292.3f);
		Load_Slot_Sprite_2->size = glm::vec2(1266.0f, 75.0f);
		Load_Slot_Sprite_2->layer = 1;
		Load_Slot_Sprite_2->isVisible = true;
		UIButtonComponent* slot_button_2 = Load_Slot_Object_2->AddComponent<UIButtonComponent>();

		GameObject* Load_Slot_Object_3 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Load_Slot_Sprite_3 = Load_Slot_Object_3->AddComponent<SpriteComponent>();
		Load_Slot_Sprite_3->sprites = {ResourceManager::LoadTexture("slot.png", "res/sprites/menu/load").id, ResourceManager::LoadTexture("slot_hover.png", "res/sprites/menu/load").id};
		Load_Slot_Sprite_3->screenPosition = glm::vec2(325.0f, 428.9f);
		Load_Slot_Sprite_3->size = glm::vec2(1266.0f, 75.0f);
		Load_Slot_Sprite_3->layer = 1;
		Load_Slot_Sprite_3->isVisible = true;
		UIButtonComponent* slot_button_3 = Load_Slot_Object_3->AddComponent<UIButtonComponent>();

		GameObject* Load_Slot_Object_4 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Load_Slot_Sprite_4 = Load_Slot_Object_4->AddComponent<SpriteComponent>();
		Load_Slot_Sprite_4->sprites = {ResourceManager::LoadTexture("slot.png", "res/sprites/menu/load").id, ResourceManager::LoadTexture("slot_hover.png", "res/sprites/menu/load").id};
		Load_Slot_Sprite_4->screenPosition = glm::vec2(325.0f, 569.8f);
		Load_Slot_Sprite_4->size = glm::vec2(1266.0f, 75.0f);
		Load_Slot_Sprite_4->layer = 1;
		Load_Slot_Sprite_4->isVisible = true;
		UIButtonComponent* slot_button_4 = Load_Slot_Object_4->AddComponent<UIButtonComponent>();

		GameObject* Load_Slot_Object_5 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Load_Slot_Sprite_5 = Load_Slot_Object_5->AddComponent<SpriteComponent>();
		Load_Slot_Sprite_5->sprites = {ResourceManager::LoadTexture("slot.png", "res/sprites/menu/load").id, ResourceManager::LoadTexture("slot_hover.png", "res/sprites/menu/load").id};
		Load_Slot_Sprite_5->screenPosition = glm::vec2(325.0f, 711.0f);
		Load_Slot_Sprite_5->size = glm::vec2(1266.0f, 75.0f);
		Load_Slot_Sprite_5->layer = 1;
		Load_Slot_Sprite_5->isVisible = true;
		UIButtonComponent* slot_button_5 = Load_Slot_Object_5->AddComponent<UIButtonComponent>();

		GameObject* Load_Back_Object = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Load_Back_Button = Load_Back_Object->AddComponent<SpriteComponent>();
		Load_Back_Button->sprites = {ResourceManager::LoadTexture("back_sprite.png", "res/sprites/menu/credits").id, ResourceManager::LoadTexture("back_sprite_hover.png", "res/sprites/menu/credits").id };
		Load_Back_Button->screenPosition = glm::vec2(1375.0f, 922.6f);
		Load_Back_Button->size = glm::vec2(471.0f, 106.0f);
		Load_Back_Button->layer = 1;
		Load_Back_Button->isVisible = true;
		UIButtonComponent* Load_Back_Object_button = Load_Back_Object->AddComponent<UIButtonComponent>();

		slot_button_1->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		slot_button_1->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		slot_button_2->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		slot_button_2->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		slot_button_3->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		slot_button_3->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		slot_button_4->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		slot_button_4->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		slot_button_5->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		slot_button_5->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};


		Load_Back_Object_button->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		Load_Back_Object_button->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};


		Load_Back_Object_button->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_main);
		};



		grp_load.push_back(Load_background_object);
		grp_load.push_back(Load_Slot_Object_1);
		grp_load.push_back(Load_Slot_Object_2);
		grp_load.push_back(Load_Slot_Object_3);
		grp_load.push_back(Load_Slot_Object_4);
		grp_load.push_back(Load_Slot_Object_5);
		grp_load.push_back(Load_Back_Object);


		//settings
		GameObject* settings_back = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* settings_back_sprite = settings_back->AddComponent<SpriteComponent>();
		settings_back_sprite->sprites = {ResourceManager::LoadTexture("settings.png", "res/sprites/menu/settings").id};
		settings_back_sprite->screenPosition = glm::vec2(-10.0f, -10.0f);
		settings_back_sprite->size = glm::vec2( 1940.0f, 1100.0f);
		settings_back_sprite->layer = 0;
		settings_back_sprite->isVisible = true;
		
		GameObject* placeholder_exit_object = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* niewiemkurwa = placeholder_exit_object->AddComponent<SpriteComponent>();
		niewiemkurwa->sprites = {ResourceManager::LoadTexture("back_sprite.png", "res/sprites/menu/credits").id, ResourceManager::LoadTexture("back_sprite_hover.png", "res/sprites/menu/credits").id };
		niewiemkurwa->screenPosition = glm::vec2(1375.0f, 922.6f);
		niewiemkurwa->size = glm::vec2(471.0f, 106.0f);
		niewiemkurwa->layer = 1;
		niewiemkurwa->isVisible = true;
		UIButtonComponent* placeholder_exit_object_button = placeholder_exit_object->AddComponent<UIButtonComponent>();

		placeholder_exit_object_button->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_main);
		};

		grp_settings.push_back(settings_back);
		grp_settings.push_back(placeholder_exit_object);

		//pause

		GameObject* pause_back = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* pause_background_sprite = pause_back->AddComponent<SpriteComponent>();
		pause_background_sprite->sprites = {ResourceManager::LoadTexture("menu_poboczne_tlo.png", "res/sprites/menu/pause").id};
		pause_background_sprite->screenPosition = glm::vec2(-10.0f, -10.0f);
		pause_background_sprite->size = glm::vec2( 1940.0f, 1100.0f);
		pause_background_sprite->layer = 0;
		pause_background_sprite->isVisible = true;

		GameObject* Pause_Logo_Object_1 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Pause_Logo_Sprite_1 = Pause_Logo_Object_1->AddComponent<SpriteComponent>();
		Pause_Logo_Sprite_1->sprites = {ResourceManager::LoadTexture("logo_menu poboczne.png", "res/sprites/menu/pause").id};
		Pause_Logo_Sprite_1->screenPosition = glm::vec2(730.5f, 151.6f);
		Pause_Logo_Sprite_1->size = glm::vec2(460.0f, 315.0f);
		Pause_Logo_Sprite_1->layer = 1;
		Pause_Logo_Sprite_1->isVisible = true;

		GameObject* Pause_Button_Object_1 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Pause_Button_Sprite_1 = Pause_Button_Object_1->AddComponent<SpriteComponent>();
		Pause_Button_Sprite_1->sprites = {ResourceManager::LoadTexture("resume_sprite.png", "res/sprites/menu/pause").id, ResourceManager::LoadTexture("resume_sprite_hover.png", "res/sprites/menu/pause").id};
		Pause_Button_Sprite_1->screenPosition = glm::vec2(725.0f, 522.1f);
		Pause_Button_Sprite_1->size = glm::vec2(470.0f, 106.0f);
		Pause_Button_Sprite_1->layer = 1;
		Pause_Button_Sprite_1->isVisible = true;
		UIButtonComponent* button_p_1 = Pause_Button_Object_1->AddComponent<UIButtonComponent>();

		GameObject* Pause_Button_Object_2 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Pause_Button_Sprite_2 = Pause_Button_Object_2->AddComponent<SpriteComponent>();
		Pause_Button_Sprite_2->sprites = {ResourceManager::LoadTexture("settings_sprite.png", "res/sprites/menu/pause").id, ResourceManager::LoadTexture("settings_sprite_hover.png", "res/sprites/menu/pause").id };
		Pause_Button_Sprite_2->screenPosition = glm::vec2(725.0f, 649.9f);
		Pause_Button_Sprite_2->size = glm::vec2(470.0f, 106.0f);
		Pause_Button_Sprite_2->layer = 1;
		Pause_Button_Sprite_2->isVisible = true;
		UIButtonComponent* button_p_2 = Pause_Button_Object_2->AddComponent<UIButtonComponent>();

		GameObject* Pause_Button_Object_3 = scenaMenu->CreateGameObject(nullptr);
		SpriteComponent* Pause_Button_Sprite_3 = Pause_Button_Object_3->AddComponent<SpriteComponent>();
		Pause_Button_Sprite_3->sprites = {ResourceManager::LoadTexture("exit_sprite.png", "res/sprites/menu/pause").id, ResourceManager::LoadTexture("exit_sprite_hover.png", "res/sprites/menu/pause").id };
		Pause_Button_Sprite_3->screenPosition = glm::vec2(725.0f, 786.7f);
		Pause_Button_Sprite_3->size = glm::vec2(470.0f, 106.0f);
		Pause_Button_Sprite_3->layer = 1;
		Pause_Button_Sprite_3->isVisible = true;
		UIButtonComponent* button_p_3 = Pause_Button_Object_3->AddComponent<UIButtonComponent>();

		button_p_1->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button_p_1->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		button_p_2->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button_p_2->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		button_p_3->onHoverEnter = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=1;
		};

		button_p_3->onHoverExit = [&](GameObject* go)
		{
			auto* sprite = go->GetComponent<SpriteComponent>();
			if (!sprite) return;

			sprite->currentSprite=0;
		};

		button_p_1->onClick = [&](GameObject* go)
		{
			sceneManager->ChangeScene("Scena 1");
			//ShowOnly(&grp_main); //change
		};

		button_p_2->onClick = [&](GameObject* go)
		{
			//ShowOnly(&grp_settings); jebac
		};

		button_p_3->onClick = [&](GameObject* go)
		{
			ShowOnly(&grp_main);
		};


		grp_pause.push_back(pause_back);
		grp_pause.push_back(Pause_Logo_Object_1);
		grp_pause.push_back(Pause_Button_Object_1);
		grp_pause.push_back(Pause_Button_Object_2);
		grp_pause.push_back(Pause_Button_Object_3);


		ShowOnly(&grp_main);
	}



};
#endif