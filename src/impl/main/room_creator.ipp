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

std::vector<Room> roomsLights;

void createFirstRoom(Scene* scena1) {
    floorModel = std::make_unique<Prefab>("res/models/number_floor.glb");
    wallModel  = std::make_unique<Prefab>("res/models/wall.glb");
    wallModel2 = std::make_unique<Prefab>("res/models/wall2.glb");
    wallModel3 = std::make_unique<Prefab>("res/models/wall3.glb");


    roomsLights.resize(2);

    Room lazienka;
    lazienka.id = 0;
    lazienka.name = "Lazienka";
    lazienka.position = glm::vec3(0.0, 0.0, -54.0);
    lazienka.halfSize = glm::vec3(40.0, 30.0, 46.5);
    roomsLights[lazienka.id] = lazienka;
    // Podloga i sufit
    GameObject* floor = CreateStaticObject(scena1, floorModelB.get(), nullptr,"PodlogawLazience", glm::vec3(12.440, 1.170, -53.770), glm::vec3(2.270, 1, 4.430));
    floor->GetComponent<ColliderComponent>()->isWalkable = true;
    CreateStaticObject(scena1, ceilingModelB.get(), nullptr, "SufitWKiblu",       glm::vec3(11.200, 20.120, -55.330),  glm::vec3(2.400, 1, 4.540)); //glm::vec3(100, 1, 100)

    // Sciany
    GameObject * ScianaTylnaKibel = CreateStaticObject(scena1, wallBathroomModel.get(),  nullptr, "ScianaTylnaKibel",           glm::vec3(11.950, 12.680, -10),    glm::vec3(2.330, 2.280, 3.000)); // glm::vec3(50, 50, 1)
    ScianaTylnaKibel->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaTylnaKibel->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKiblowa = CreateStaticObject(scena1, wallBathroomModel.get(), nullptr, "ScianaKiblowa",              glm::vec3(35.000, 12.680, -53.510),     glm::vec3(4.920, 2.280, 3.000), glm::vec3(0,90,0)); // glm::vec3(1, 50, 100)
    ScianaKiblowa->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaKiblowa->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaSinkowa = CreateStaticObject(scena1, wallBathroomModel.get(), nullptr, "ScianaSinkowa",              glm::vec3(-10, 11.080, -51.790),    glm::vec3(4.920, 2.280, 3.000), glm::vec3(0,90,0)); // glm::vec3(1, 50, 100)
    ScianaSinkowa->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaSinkowa->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaDoMainRoomPrawa = CreateStaticObject(scena1, wallBathroomModel.get(),  nullptr, "ScianaDrzwiDoMainRoomPrawa", glm::vec3(23.770, 11, -100), glm::vec3(1.38, 2.28, 2.82));
    ScianaDoMainRoomPrawa->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaDoMainRoomPrawa->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaDoMainRoomLewa = CreateStaticObject(scena1, wallBathroomModel.get(),  nullptr, "ScianaDrzwiDoMainRoomLewa",  glm::vec3(-3.985, 11.080, -100),glm::vec3(0.560, 2.280, 1));
    ScianaDoMainRoomLewa->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaDoMainRoomLewa->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    CreateStaticObject(scena1, wallBathroomModel.get(),  nullptr, "GoraPrzejscieDoMainRoom",    glm::vec3(5.090, 30.390, -99.960),  glm::vec3(0.550, 2.630, 0.500));

    // Kibel
    GameObject* kible = scena1->CreateGameObject(nullptr);
    kible->name = "Kible";
    GameObject* tablicaKibli[8];
    for (int i = 0; i < 8; i++) {
        if (i !=2 && i != 3) {
            tablicaKibli[i] = toiletModel->Instantiate(*scena1, kible, nullptr);
            tablicaKibli[i]->name = "Kibel" + std::to_string(i);
            tablicaKibli[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
            tablicaKibli[i]->AddComponent<ColliderComponent>();
            tablicaKibli[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 2.5, 4, 2.5 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->offset       = glm::vec3{ 0, 4, 0 };
            tablicaKibli[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 30, 0.5f, -25 + (-10 * i) };
            tablicaKibli[i]->GetComponent<TransformComponent>()->rotation    = glm::vec3{ 0, 90, 0 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
            tablicaKibli[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
        }

        if (i == 2 || i == 3) {
            tablicaKibli[i] = urinModel->Instantiate(*scena1, kible, nullptr);
            tablicaKibli[i]->name = "Kibel" + std::to_string(i);
            tablicaKibli[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 12, 12, 12 };
            tablicaKibli[i]->AddComponent<ColliderComponent>();
            tablicaKibli[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 2.5, 4, 2.5 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->offset       = glm::vec3{ 0, 4, 0 };
            tablicaKibli[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 32.6, 2.0f, -25 + (-10 * i) };
            tablicaKibli[i]->GetComponent<TransformComponent>()->rotation    = glm::vec3{ 0, 270, 0 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
            tablicaKibli[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
        }
    }

    // Zaslony
    GameObject* zaslony = scena1->CreateGameObject(nullptr);
    zaslony->name = "Zasolony";
    GameObject* tablicaZaslon[9];
    for (int i = 0; i < 9; i++) {

        tablicaZaslon[i] = wallModel3->Instantiate(*scena1, zaslony, nullptr);
        tablicaZaslon[i]->GetComponent<TransformComponent>()->scale = glm::vec3{ 0.3, 20, 20 };
        tablicaZaslon[i]->name = "Zaslona" + std::to_string(i);
        tablicaZaslon[i]->AddComponent<ColliderComponent>();
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 20, 15, 0.3 };
        tablicaZaslon[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 35, 0, -20 + (-10 * i) };
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;

        if (i==3) {
            tablicaZaslon[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 5000, 0, -20 + (-10 * i) };
        }
    }

    // Drzwi do kibla
    for (int i = 0; i < 8; i++) {
        if (i != 2 && i != 3) {
            glm::vec3 doorPos      = glm::vec3{ 14.830, 9, -20 + (-10.0f * i) };
            glm::vec3 doorScale    = glm::vec3{ 3.040f, 2.80f, 1.8 };
            glm::vec3 pivotOffset  = glm::vec3(0.0f, 0.0f, 0.1);
            glm::vec3 colliderSize = glm::vec3{ 1, 10, 5 };
            GameObject* hinge = CreateInteractableDoor(
                scena1, doorsToiletModel.get(), nullptr,
                "ToiletDoor_" + std::to_string(i),
                doorPos, doorScale, pivotOffset, colliderSize, 90.0f, 90, glm::vec3{ 5, 10, 1 }, glm::vec3{-4.5,0, 0}, glm::vec3{0,0, -4.5}
            );
            unlockedDoors.insert(hinge);
        }
    }

    // Papier toaletowy
    GameObject* tablicaPokrywek[6];
    for (int i = 0; i < 6; i++) {
        if (i == 0 || i == 3)
            tablicaPapierowKibel[i] = toiletPaperGreenModel->Instantiate(*scena1, nullptr, nullptr);
        if (i == 1 || i == 5)
            tablicaPapierowKibel[i] = toiletPaperRedModel->Instantiate(*scena1, nullptr, nullptr);
        if (i == 2 || i == 4)
            tablicaPapierowKibel[i] = toiletPaperBlueModel->Instantiate(*scena1, nullptr, nullptr);

        tablicaPokrywek[i] = pokrywkaRolkiModel->Instantiate(*scena1, nullptr, nullptr);
        tablicaPokrywek[i]->name = "pokrywkaKibel"+std::to_string(i);
        tablicaPokrywek[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
        tablicaPokrywek[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaPokrywek[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 21, 5.0, -40.7 + (-10 * i) };

        tablicaPapierowKibel[i]->name = "PapierKibel" + std::to_string(i);
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaPapierowKibel[i]->AddComponent<ColliderComponent>();
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 21, 5.0, -40.7 + (-10 * i) };
        rotatableObjects.insert(tablicaPapierowKibel[i]);
        if (i < 2) {
            tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 21, 5.0, -40.7 + (-10 * i) + 20 };
            tablicaPokrywek[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 21, 5.0, -40.7 + (-10 * i) + 20 };
        }
    }

    // Zlewy - pozycja X z MainRoomIPoprawkiModeli (-20.5)
    GameObject* zlewy = scena1->CreateGameObject(nullptr);
    zlewy->name = "Zlewy";
    GameObject* tablicaSink[8];
    for (int i = 0; i < 8; i++) {
        tablicaSink[i] = sinkModel->Instantiate(*scena1, zlewy, nullptr);
        tablicaSink[i]->name = "Sink" + std::to_string(i);
        tablicaSink[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
        tablicaSink[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaSink[i]->GetComponent<TransformComponent>()->position = glm::vec3{ -5.5, 6.0, -20 + (-10 * i) };
        tablicaSink[i]->AddComponent<ColliderComponent>();
    }

    GameObject* lustra = scena1->CreateGameObject(nullptr);
    lustra->name = "Lustra";
    // Lustra 1-3
    GameObject* lustro1 = mirrorModel1->Instantiate(*scena1, lustra, nullptr);
    lustro1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 0, 0 };
    lustro1->AddComponent<ColliderComponent>();
    lustro1->GetComponent<TransformComponent>()->position   = glm::vec3{ -8.5, 12.0, -25 + (-20 * 0) };

    GameObject* lustro2 = mirrorModel2->Instantiate(*scena1, lustra, nullptr);
    lustro2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro2->AddComponent<ColliderComponent>();
    lustro2->GetComponent<TransformComponent>()->position   = glm::vec3{ -8.5, 12.0, -25 + (-20 * 1) };

    GameObject* lustro3 = mirrorModel3->Instantiate(*scena1, lustra, nullptr);
    lustro3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro3->AddComponent<ColliderComponent>();
    lustro3->GetComponent<TransformComponent>()->position   = glm::vec3{ -8.5, 12.0, -25 + (-20 * 2) };

    // Lustro 4 - dodane z mirrorModel4 (lustro_puste.glb)
    GameObject* lustro4 = mirrorModel4->Instantiate(*scena1, lustra, nullptr);
    lustro4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro4->AddComponent<ColliderComponent>();
    lustro4->GetComponent<TransformComponent>()->position   = glm::vec3{ -8.5, 12.0, -25 + (-20 * 3) };

    // Drzwi wyjsciowe z lazienki (washroomExit)
    GameObject* hingeWashroomExit = CreateInteractableDoor(
    scena1, washroomExit.get(), nullptr, "WashroomExit",
    glm::vec3(10.0f, 9.840, -100.0f),
    glm::vec3{ 2, 2, 2 },
    glm::vec3(0.1f, 0.0f, 0.0f),
    glm::vec3{ 5, 22, 0.5 },
    -90.0f,
    180.0f,
    glm::vec3{ 1, 22, 4 },
    glm::vec3(0.0f, 0.0f, -4.0f),glm::vec3(-4,0,0)
);
    //majorDoors.insert(hingeWashroomExit);
    toiletDoorsMap[hingeWashroomExit].requiresUnlock = true;
}

void createMainRooom(Scene* scena) {

    Room mainRoom;
    mainRoom.id = 1;
    mainRoom.name = "MainRoom";
    mainRoom.position = glm::vec3(26.0, 0.0, -147.0);
    mainRoom.halfSize = glm::vec3(38.0, 30.0, 46.5);
    roomsLights[mainRoom.id] = mainRoom;
    // Podloga i sufit
    GameObject * podlogaMainRomm = CreateStaticObject(scena, floorModelB.get(), nullptr, "PodlogaMainRoom", glm::vec3(24.870, 1.170, -143.820),  glm::vec3(3.560, 1, 4.570));
    podlogaMainRomm->GetComponent<ColliderComponent>()->isWalkable = true;
    CreateStaticObject(scena, ceilingModelB.get(), nullptr, "SufitMainRoom",   glm::vec3(24.980, 20.100, -144.870), glm::vec3(3.450, 1, 4.460));

    // Sciany
    GameObject * ScianaDoRentgenaPrawa = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaDoRentgenaPrawa",        glm::vec3(36.120, 11.000, -188.720),   glm::vec3(2.540, 2.190, 3.000));
    ScianaDoRentgenaPrawa->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaDoRentgenaPrawa->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * Zauek = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "Zauek",        glm::vec3(37.230, 11.000, -102.700),   glm::vec3(2.240, 2.190, 9.940));
    Zauek->GetComponent<ColliderComponent>()->isWalkable = false;
    Zauek->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaDoRentgenaLewa = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaDoRentgenaLewa",         glm::vec3(-3.185, 11.000, -188.720),  glm::vec3(0.600, 2.190, 3.000));
    ScianaDoRentgenaLewa->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaDoRentgenaLewa->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaPrawaDoKrematorium = CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "ScianaPrawaDoKrematorium",     glm::vec3(60, 11, -104.890),   glm::vec3(0.400, 2.190, 3.000), glm::vec3(0,90,0));
    ScianaPrawaDoKrematorium->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaPrawaDoKrematorium->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaLewaDoKrematorium = CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "ScianaLewaDoKrematorium",      glm::vec3(60, 11.000, -152.875),   glm::vec3(3.670, 2.190, 3.000),glm::vec3(0,90,0));
    ScianaLewaDoKrematorium->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaLewaDoKrematorium->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaPrawaDoATOMU = CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "ScianaPrawaDoATOMU",           glm::vec3(-10.000, 11.000, -120.360),  glm::vec3(1.980, 2.190, 3.000), glm::vec3(0,90,0));
    ScianaPrawaDoATOMU->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaPrawaDoATOMU->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaLewaDoATOMU = CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "ScianaLewaDoATOMU",            glm::vec3(-10.000, 11,  -168.342),  glm::vec3(2.110, 2.190, 3.000), glm::vec3(0,90,0));
    ScianaLewaDoATOMU->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaLewaDoATOMU->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaDoLazienkiPrawa = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaDoLazienkiPrawa",        glm::vec3(35.067, 11.000, -100.720),   glm::vec3(2.540, 2.190, 3.000));
    ScianaDoLazienkiPrawa->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaDoLazienkiPrawa->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaDoLazienkiLewa = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaDoLazienkiLewa",         glm::vec3(-4.681, 11.000, -100.720),  glm::vec3(0.630, 2.190, 3.000));
    ScianaDoLazienkiLewa->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaDoLazienkiLewa->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    // Gora przejscia-169.010
    //CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoRentgena",              glm::vec3(0, 66, -189.000),   glm::vec3(10.000, 50, 1));
    CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "GoraPrzejscieDoRentgena",              glm::vec3(6.634, 20.890, -188.710),   glm::vec3(0.450, 1, 3.000));
    //CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoKrematorium",           glm::vec3(60, 66, -209.590),  glm::vec3(100, 50, 1), glm::vec3(0, 90, 0));
    CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "GoraPrzejscieDoKrematorium",           glm::vec3(59.970, 20.990, -114.300),  glm::vec3(0.600, 1, 3.000), glm::vec3(0, 90, 0));
    //CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoREAKTORAATOMOWEGO",     glm::vec3(-17.000+7, 66, -209.590), glm::vec3(100, 50, 1), glm::vec3(0, 90, 0));
    CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "GoraPrzejscieDoREAKTORAATOMOWEGO",     glm::vec3(-9.970, 21.060, -145.270), glm::vec3(0.540, 1, 3.000), glm::vec3(0, 90, 0));

    glm::vec3 scaleDoors = glm::vec3(2.25, 2.2, 1);

    GameObject* hingeKrematorium = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoKrematorium",
        glm::vec3(59.850f, 7.300f, -115.090+3-2.65+4), glm::vec3(2.500, 3.400, 1),
        glm::vec3(0.0f, 0.0f, -6.0f), glm::vec3(1, 20.0f, 7), -90.0f, 90.0f
    );
    //129.950 115.090
    toiletDoorsMap[hingeKrematorium].canBeClicked = false;
    mainRoomDoors.push_back(hingeKrematorium);

    GameObject* hingeRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoRentgen",
        glm::vec3(5.440f-3+2.65, 7.300f, -217.800f+15+14), glm::vec3(/*3.600*/2.500, 3.400, 1),
        glm::vec3(6.0f, 0.0f, 0.0f), glm::vec3(5.7f, 20.0f, 1.0f), 90.0f, 0.0f,glm::vec3(1.0f, 20.0f, 4.670), glm::vec3(0.0f, 0.0f, -5.070), glm::vec3(2, 0.0f, 0.0f)
    );
    /*GameObject* hingeDrzwiDoRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoKrematorium",
        glm::vec3(131.390-5-2-0.3, 7.300f, -121.670), glm::vec3(3.600, 3.400, 1),
        glm::vec3(8.0f, 0.0f, 0.0f), glm::vec3(5.7f, 20.0f, 1.0f), -90.0f, 0.0f, glm::vec3(1.0f, 20.0f, 4.670), glm::vec3(0.0f, 0.0f, -5.070), glm::vec3(2, 0.0f, 0.0f)
    );*/
    toiletDoorsMap[hingeRentgen].canBeClicked = false;
    mainRoomDoors.push_back(hingeRentgen);

    GameObject* hingeATOM = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoATOMU",
        glm::vec3(-17.000+7, 7.300f, -146.010+3-2.65+4), glm::vec3(2.500, 3.400, 1),
        glm::vec3(0.0f, 0.0f, -6.0f), glm::vec3(0.8f, 20.0f, 5.7f), 90.0f, 90.0f,glm::vec3(4.670, 20.0f, 1.0), glm::vec3(-0.070f, 0.0f, 0), glm::vec3(0, 0.0f, -2)
    );
    toiletDoorsMap[hingeATOM].canBeClicked = false;
    mainRoomDoors.push_back(hingeATOM);

    GameObject* szafkaObj = szafkaModel->Instantiate(*scena, nullptr, nullptr);
    szafkaObj->name = "Szafka";

    TransformComponent* szafkaTr = szafkaObj->GetComponent<TransformComponent>();
    szafkaTr->position = glm::vec3{ 27.0f, 6.530, -216.490f +15+14};
    szafkaTr->scale    = glm::vec3{ 8.0f, 8.0f, 8.0f };
    szafkaTr->rotation = glm::vec3{ 0.0f, -90.0f, 0.0f };

    // nie dotykac tego collidera kurna
    ColliderComponent* szafkaCol = szafkaObj->AddComponent<ColliderComponent>();
    szafkaCol->affectsNavMesh = true;
    szafkaCol->halfSize     = glm::vec3{ 10.0f, 1.0f, 3.0f };
    szafkaCol->offset       = glm::vec3{ 2.0f, 0.0f, 0.0f };

    CabinetState cabState;
    szafkaObj->TraverseChildren([&](GameObject* go) {
        if (go->name == "Left_Door")  cabState.leftDoor  = go;
        if (go->name == "Right_Door") cabState.rightDoor = go;
        if (go->name == "Guzik")      cabState.button    = go;
        if (go->name == "Gear_Fixed_1") fixedGear1 = go;
        if (go->name == "Gear_Fixed_2") fixedGear2 = go;

        if (go->name == "start_button") {
            machineStartButton = go;
            ColliderComponent* col = go->AddComponent<ColliderComponent>();
            col->halfSize = glm::vec3(0.0f);
        }

        if (go->name.find("lights_") != std::string::npos) {
            machineLights[go->name] = go;

            LightComponent* lc = go->AddComponent<LightComponent>();
            lc->type = Point;
            lc->ambient = glm::vec3(0.1f, 0.05f, 0.0f);
            lc->diffuse = glm::vec3(1.0f, 0.4f, 0.0f);
            lc->specular = glm::vec3(1.0f, 0.4f, 0.0f);
            lc->constant = 1.0f;
            lc->linear = 0.22f;
            lc->quadratic = 0.20f;
            roomsLights[mainRoom.id].lights.push_back(lc);
           

            if (go->name == "lights_2" || go->name == "lights_5") {
                lc->isOn = true;
                roomsLights[mainRoom.id].savedStates.push_back(true);
            }
            else {
                lc->isOn = false;
                roomsLights[mainRoom.id].savedStates.push_back(false);
            }
        }

        if (go->name == "MachineSlot_1" || go->name == "MachineSlot_2" || go->name == "MachineSlot_3") {
            ColliderComponent* col = go->AddComponent<ColliderComponent>();
            col->halfSize = glm::vec3(0.0f);

            PuzzleSlot slot;
            slot.slotObject = go;
            slot.expectedObject = nullptr;
            slot.targetRotation = glm::vec3(90.0f, 0.0f, 0.0f);

            machineSlotsMap[go] = slot;
            spdlog::info("Skonfigurowano slot maszyny: {}", go->name);
        }
    });

    GameObject * bossCapsule = bossCapsuleModel->Instantiate(*scena, nullptr, nullptr);
    bossCapsule->name = "BossCapsule";
    bossCapsule->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    bossCapsule->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    bossCapsule->GetComponent<TransformComponent>()->position = glm::vec3{51.58 ,5.229, -180.960}; //194
    bossCapsule->AddComponent<ColliderComponent>();
    bossCapsule->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    bossCapsule->GetComponent<ColliderComponent>()->isWalkable = false;
    bossCapsule->GetComponent<ColliderComponent>()->halfSize    = glm::vec3{ 5.34, 10.0f, 5.34f };

    GameObject * kredens1 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens1",     glm::vec3(56, 4.8, -176+15), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0)); // glm::vec3(2, 5, 13)
    kredens1->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    kredens1->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * kredens2 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens2",     glm::vec3(56, 4.8, -152+15), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0)); // glm::vec3(2, 5, 13)
    kredens2->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    kredens2->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * kredens3 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens3",     glm::vec3(20+7, 4.8, -176+15), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0)); // glm::vec3(2, 5, 13)
    kredens3->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    kredens3->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * kredens4 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens4",     glm::vec3(20+7, 4.8, -152+15), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0)); // glm::vec3(2, 5, 13)
    kredens4->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    kredens4->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * kredens5 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens5",     glm::vec3(27.7+7, 4.8, -177+15), glm::vec3(8, 8, 8), glm::vec3(0, -270, 0)); //glm::vec3(4, 5, 13)
    kredens5->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    kredens5->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * kredens6 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens6",     glm::vec3(27.7+7, 4.8, -153+15), glm::vec3(8, 8, 8), glm::vec3(0, -270, 0)); //glm::vec3(4, 5, 13)
    kredens6->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    kredens6->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * kredens7 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens7",     glm::vec3(-12.230+7, 4.8, -122.540), glm::vec3(8, 8, 8), glm::vec3(0, -270, 0)); //glm::vec3(4, 5, 13)
    kredens7->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    kredens7->GetComponent<ColliderComponent>()->isWalkable = false;
    //kredens5->GetComponent<ColliderComponent>()->offset = glm::vec3{ -2.0f, 0.0f, 0.0f };
    //kredens6->GetComponent<ColliderComponent>()->offset = glm::vec3{ -2.0f, 0.0f, 0.0f };
    //kredens7->GetComponent<ColliderComponent>()->offset = glm::vec3{ -2.0f, 0.0f, 0.0f };

    GameObject * eksp1 = eksp1Model->Instantiate(*scena, nullptr, nullptr);
    eksp1->name = "eksp1";
    eksp1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    eksp1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -180.0f, 0.0f };
    eksp1->GetComponent<TransformComponent>()->position = glm::vec3{ 57.170 ,9.510, -165.340 + 15 };

    GameObject * eksp2 = eksp2Model->Instantiate(*scena, nullptr, nullptr);
    eksp2->name = "eksp2";
    eksp2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    eksp2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -111.000f, 0.0f };
    eksp2->GetComponent<TransformComponent>()->position = glm::vec3{ 57.250 ,9.000, -178.730f + 15  };

    GameObject * fiolka1 = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1->name = "fiolka1";
    fiolka1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka1->GetComponent<TransformComponent>()->position = glm::vec3{ 20+7 ,9, -144.64f + 15  };
    GameObject * fiolka1b = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1b->name = "fiolka1b";
    fiolka1b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -300.0f, 0.0f };
    fiolka1b->GetComponent<TransformComponent>()->position = glm::vec3{ 22+7 ,9, -146.64f + 15  };
    GameObject * fiolka1c = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1c->name = "fiolka1c";
    fiolka1c->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1c->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -200.0f, 0.0f };
    fiolka1c->GetComponent<TransformComponent>()->position = glm::vec3{ 22+7 ,9, -142.64f + 15  };

    GameObject * fiolka2 = fiolka2Model->Instantiate(*scena, nullptr, nullptr);
    fiolka2->name = "fiolka2";
    fiolka2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10.000, 10.000, 10.000 };
    fiolka2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka2->GetComponent<TransformComponent>()->position = glm::vec3{ 58.270 ,9.000, -174.380f + 15  };
    GameObject * fiolka2b = fiolka2Model->Instantiate(*scena, nullptr, nullptr);
    fiolka2b->name = "fiolka2b";
    fiolka2b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10.000, 10.000, 10.000 };
    fiolka2b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka2b->GetComponent<TransformComponent>()->position = glm::vec3{ 57.630 ,9.000, -171.140f  + 15 };

    //GameObject * ksiazka = ksiazkaModel->Instantiate(*scena, nullptr, nullptr);
    //ksiazka->name = "ksiazka";
    //ksiazka->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    //ksiazka->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    //ksiazka->GetComponent<TransformComponent>()->position = glm::vec3{ 56.920 ,8.560, -150.740  + 15 };

    SpawnLoreNote(scena, ksiazkaModel.get(), glm::vec3{ 56.920 ,8.560, -150.740 + 15 }, "res/lore/Kalfu_ch4.txt", glm::vec3{ 0.0f, -45.0f, 0.0f }, glm::vec3(10), nullptr);

    GameObject* probowki = scena->CreateGameObject(nullptr);
    probowki->name = "probowki";

    GameObject* probowka7 = probowka7Model->Instantiate(*scena, probowki, nullptr);
    probowka7->name = "probowka7";
    probowka7->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka7->GetComponent<TransformComponent>()->position = glm::vec3{ 57.570 ,9, -174.640f  + 15 };
    GameObject * probowka7b = probowka7Model->Instantiate(*scena, probowki, nullptr);
    probowka7b->name = "probowka7b";
    probowka7b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -30.0f, 0.0f };
    probowka7b->GetComponent<TransformComponent>()->position = glm::vec3{ 25.960+7 ,9, -174.640f + 15  };
    GameObject * probowka7c = probowka7Model->Instantiate(*scena, nullptr, nullptr);
    probowka7c->name = "probowka7c";
    probowka7c->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7c->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 45.0f, 0.0f };
    probowka7c->GetComponent<TransformComponent>()->position = glm::vec3{ 21.360+7 ,9, -182.860  + 15 };

    GameObject * probowka6 = probowka6Model->Instantiate(*scena, probowki, nullptr);
    probowka6->name = "probowka6";
    probowka6->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka6->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -66.900, 0.0f };
    probowka6->GetComponent<TransformComponent>()->position = glm::vec3{ 56.930 ,8.580, -172.450 + 15  };
    GameObject * probowka6b = probowka6Model->Instantiate(*scena, probowki, nullptr);
    probowka6b->name = "probowka6b";
    probowka6b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka6b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -115.900, 0.0f };
    probowka6b->GetComponent<TransformComponent>()->position = glm::vec3{ 56.930 ,8.580, -174.460  + 15 };

    GameObject * probowka5 = probowka5Model->Instantiate(*scena, probowki, nullptr);
    probowka5->name = "probowka5";
    probowka5->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka5->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -45.0f, 0.0f };
    probowka5->GetComponent<TransformComponent>()->position = glm::vec3{ 25.690+7 ,8.570, -144.030  + 15 };
    GameObject * probowka5b = probowka5Model->Instantiate(*scena, nullptr, nullptr);
    probowka5b->name = "probowka5b";
    probowka5b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka5b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -112.0f, 0.0f };
    probowka5b->GetComponent<TransformComponent>()->position = glm::vec3{ 25.880+7 ,8.570, -147.370  + 15 };

    GameObject * probowkaArka_1 = probowkaArka_1_Model->Instantiate(*scena, probowki, nullptr);
    probowkaArka_1->name = "probowkaArka_1";
    probowkaArka_1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowkaArka_1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -11.300, 0.0f };
    probowkaArka_1->GetComponent<TransformComponent>()->position = glm::vec3{ 57.550 ,9.260, -184.430 + 15  };

    GameObject * probowka3 = probowka3Model->Instantiate(*scena, probowki, nullptr);
    probowka3->name = "probowka3";
    probowka3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.500, 1.500, 1.500 };
    probowka3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -151.800, 0.0f };
    probowka3->GetComponent<TransformComponent>()->position = glm::vec3{ 55.760 ,9.640, -185.700 + 15  };

    GameObject * probowka4 = probowka4Model->Instantiate(*scena, probowki, nullptr);
    probowka4->name = "probowka4";
    probowka4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowka4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka4->GetComponent<TransformComponent>()->position = glm::vec3{ 56.080 ,10.500, -162.990  + 15 };
    GameObject * probowka4b = probowka4Model->Instantiate(*scena, probowki, nullptr);
    probowka4b->name = "probowka4b";
    probowka4b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowka4b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka4b->GetComponent<TransformComponent>()->position = glm::vec3{ 57.500 ,10.500, -161.540  + 15 };

    GameObject * labOla1 = labOla1Model->Instantiate(*scena, nullptr, nullptr);
    labOla1->name = "labOla1";
    labOla1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 1, 1 };
    labOla1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    labOla1->GetComponent<TransformComponent>()->position = glm::vec3{ 56.840 ,9.540, -157.110 + 15  };

    GameObject * probowka2 = probowka2Model->Instantiate(*scena, nullptr, nullptr);
    probowka2->name = "probowka2";
    probowka2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -176.200, 0.0f };
    probowka2->GetComponent<TransformComponent>()->position = glm::vec3{ 24.200+7 ,9.400, -145.450 + 15  };

    //GameObject * folder = folderModel->Instantiate(*scena, nullptr, nullptr);
    //folder->name = "folder";
    //folder->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    //folder->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -93.300, 0.0f };
    //folder->GetComponent<TransformComponent>()->position = glm::vec3{ 26.550+7 ,8.390, -150.040  + 15 };

    SpawnLoreNote(scena, ksiazkaModel.get(), glm::vec3{ 26.550 + 7 , 8.730, -150.040 + 15 }, "res/lore/Kalfu_ch6.txt", glm::vec3{ 0.0f, -93.300, 0.0f }, glm::vec3(10), nullptr);

    //GameObject * papers = papersModel->Instantiate(*scena, nullptr, nullptr);
    //papers->name = "papers";
    //papers->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    //papers->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    //papers->GetComponent<TransformComponent>()->position = glm::vec3{ 22.080+7 ,8.270, -149.310 + 15  };

    //SpawnLoreNote(scena, ksiazkaModel.get(), glm::vec3{ 22.080 + 7 ,8.670, -149.310 + 15 }, "res/lore/Kalfu_ch7.txt", glm::vec3{ 0.0f, -45.0f, 0.0f }, glm::vec3(10), nullptr);

    GameObject * cup = cupModel->Instantiate(*scena, nullptr, nullptr);
    cup->name = "cup";
    cup->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    cup->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    cup->GetComponent<TransformComponent>()->position = glm::vec3{ 21.060+7 ,8.420, -186.810 + 15  };
    GameObject * cup2 = cupModel->Instantiate(*scena, nullptr, nullptr);
    cup2->name = "cup2";
    cup2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    cup2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -169.600, 0.0f };
    cup2->GetComponent<TransformComponent>()->position = glm::vec3{ 24.210+7 ,8.420, -185.480  + 15 };

    GameObject * corkBoard = corkBoardModel->Instantiate(*scena, nullptr, nullptr);
    corkBoard->name = "corkBoard";
    corkBoard->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    corkBoard->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -90.000, 0.0f };
    corkBoard->GetComponent<TransformComponent>()->position = glm::vec3{ 58.740 ,13.320, -177.210  + 15 };

    GameObject * clock = clockModel->Instantiate(*scena, nullptr, nullptr);
    clock->name = "clock";
    clock->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    clock->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 180.000, 0.0f };
    clock->GetComponent<TransformComponent>()->position = glm::vec3{ 58.840 ,17.080, -147.150  + 15 };

    GameObject * computer_pbr = computer_pbrModel->Instantiate(*scena, nullptr, nullptr);
    computer_pbr->name = "computer_pbr";
    computer_pbr->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    computer_pbr->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    computer_pbr->GetComponent<TransformComponent>()->position = glm::vec3{ 57.280 ,8.170, -142.870 + 15 };
    GameObject * computer_pbr2 = computer_pbrModel->Instantiate(*scena, nullptr, nullptr);
    computer_pbr2->name = "computer_pbr2";
    computer_pbr2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    computer_pbr2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    computer_pbr2->GetComponent<TransformComponent>()->position = glm::vec3{ 57.280 ,8.170, -147.460 + 15 };

    GameObject * laboratoryStuff1 = laboratoryStuff1Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff1->name = "laboratoryStuff1";
    laboratoryStuff1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    laboratoryStuff1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -123.800, 0.0f };
    laboratoryStuff1->GetComponent<TransformComponent>()->position = glm::vec3{ 27.760 ,11.070, -157.870 + 15  };

    GameObject * laboratoryStuff2 = laboratoryStuff2Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff2->name = "laboratoryStuff2";
    laboratoryStuff2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2.5, 2.5, 2.5 };
    laboratoryStuff2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -72.300, 0.0f };
    laboratoryStuff2->GetComponent<TransformComponent>()->position = glm::vec3{ 23.630+7 ,10.250, -165.330 + 15  };

    GameObject * laboratoryStuff3 = laboratoryStuff3Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff3->name = "laboratoryStuff3";
    laboratoryStuff3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    laboratoryStuff3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 7.500, 0.0f };
    laboratoryStuff3->GetComponent<TransformComponent>()->position = glm::vec3{ 23.540+7 ,8.540, -178.320  + 15 };

    GameObject * krzeslo1 = krzesloModel->Instantiate(*scena, nullptr, nullptr);
    krzeslo1->name = "krzeslo1";
    krzeslo1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    krzeslo1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 159.200, 0.0f };
    krzeslo1->GetComponent<TransformComponent>()->position = glm::vec3{ 48.160 ,4.590, -126.720 };
    krzeslo1->AddComponent<ColliderComponent>();
    GameObject * krzeslo2 = krzesloModel->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff3->name = "krzeslo2";
    krzeslo2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    krzeslo2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 106.100, 0.0f };
    krzeslo2->GetComponent<TransformComponent>()->position = glm::vec3{ 48.160 ,4.590, -139.860};
    krzeslo2->AddComponent<ColliderComponent>();
    GameObject * Szafka_lab1 = szafka_labModel->Instantiate(*scena, nullptr, nullptr);
    Szafka_lab1->name = "Szafka_lab1";
    Szafka_lab1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10 };
    Szafka_lab1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 91.000, 0.0f };
    Szafka_lab1->GetComponent<TransformComponent>()->position = glm::vec3{ -13.160+7 ,6.680, -190.470+14};
    Szafka_lab1->AddComponent<ColliderComponent>();
    Szafka_lab1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 5.0f, 5, 10.0f };
    Szafka_lab1->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    Szafka_lab1->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * Szafka_lab2 = szafka_labModel->Instantiate(*scena, nullptr, nullptr);
    Szafka_lab2->name = "Szafka_lab2";
    Szafka_lab2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10 };
    Szafka_lab2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 88, 0.0f };
    Szafka_lab2->GetComponent<TransformComponent>()->position = glm::vec3{ -13.160+7 ,6.680, -176.930+14};
    Szafka_lab2->AddComponent<ColliderComponent>();
    Szafka_lab2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 5.0f, 5, 10.0f };
    Szafka_lab2->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    Szafka_lab2->GetComponent<ColliderComponent>()->isWalkable = false;
    if (cabState.button) {
        ColliderComponent* btnCol = cabState.button->AddComponent<ColliderComponent>();
        btnCol->halfSize = glm::vec3{ 1.0f, 1.0f, 1.0f };

        TransformComponent* btnTr = cabState.button->GetComponent<TransformComponent>();
        cabState.buttonStartPos  = btnTr->position;
        cabState.buttonTargetPos = cabState.buttonStartPos + glm::vec3{ 0.0f, 0.0f, -0.15f };

        cabinetsMap[cabState.button] = cabState;
    }

    GameObject* bossObj = bossModel->Instantiate(*scena, nullptr, nullptr);
    bossObj->name = "DemonBoss";

    TransformComponent* bossTr = bossObj->GetComponent<TransformComponent>();
    bossTr->position = glm::vec3(52.590, 9.170, -196.120+14);
    bossTr->scale = glm::vec3(1.5f, 2.000, 1.5f);
    bossTr->rotation = glm::vec3(0.0f, 130.000, 0.0f);
    bossTr->isDirty = true;

    AnimatorComponent* bossAnimator = bossObj->AddComponent<AnimatorComponent>();

    if (bossModel->rootModel && !bossModel->rootModel->animations.empty()) {
        AnimationClip* defaultBossClip = &bossModel->rootModel->animations[0];

        AnimationHelper::Play(bossAnimator, defaultBossClip, true, 1.0f);
    }

    GameObject * szafka_inna1b = szafka_inna1Model->Instantiate(*scena, nullptr, nullptr);
    szafka_inna1b->name = "szafka_inna1b";
    szafka_inna1b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    szafka_inna1b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 180, 0.0f };
    szafka_inna1b->GetComponent<TransformComponent>()->position = glm::vec3{ -9.520+7 ,4.660, -107.320};
    szafka_inna1b->AddComponent<ColliderComponent>();
    szafka_inna1b->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 4.520, 5, 2.420f };
    szafka_inna1b->GetComponent<ColliderComponent>()->offset = glm::vec3{ -3.430, 0, 0 };
    szafka_inna1b->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    szafka_inna1b->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * szafka_inna2 = szafka_inna2Model->Instantiate(*scena, nullptr, nullptr);
    szafka_inna2->name = "szafka_inna2";
    szafka_inna2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    szafka_inna2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90, 0.0f };
    szafka_inna2->GetComponent<TransformComponent>()->position = glm::vec3{ -8.820+7 ,4.550, -102.990};
    szafka_inna2->AddComponent<ColliderComponent>();
    szafka_inna2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 5.910, 4.100, 2.760 };
    szafka_inna2->GetComponent<ColliderComponent>()->offset = glm::vec3{ -3.470, 0, 0.0f };
    szafka_inna2->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    szafka_inna2->GetComponent<ColliderComponent>()->isWalkable = false;
    GameObject * wozek = wozekModel->Instantiate(*scena, nullptr, nullptr);
    wozek->name = "wozek";
    wozek->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    wozek->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 122.800, 0.0f };
    wozek->GetComponent<TransformComponent>()->position = glm::vec3{ 6.630+7 ,4.800, -145.800};
    wozek->AddComponent<ColliderComponent>();
    wozek->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 4.0f, 5, 5.130 };

    GameObject * eksperyment3 = eksp3Model->Instantiate(*scena, nullptr, nullptr);
    eksperyment3->name = "eksperyment3";
    eksperyment3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 5 };
    eksperyment3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90.100, 0.0f };
    eksperyment3->GetComponent<TransformComponent>()->position = glm::vec3{ -6.030 ,9.180, -112.190};
    GameObject * eksperyment3b = eksp3Model->Instantiate(*scena, nullptr, nullptr);
    eksperyment3b->name = "eksperyment3b";
    eksperyment3b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 5 };
    eksperyment3b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 102.700, 0.0f };
    eksperyment3b->GetComponent<TransformComponent>()->position = glm::vec3{ -11.400+7 ,9.860, -107.600};
    /*for (int i = 0; i < 6; i++) {
        GameObject * ekperyment4 = eksp4Model->Instantiate(*scena, nullptr, nullptr);
        ekperyment4->name = "ekperyment4_a" + std::to_string(i);
        ekperyment4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7};
        ekperyment4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0, 0.0f };
        ekperyment4->GetComponent<TransformComponent>()->position = glm::vec3{ -7.910 ,7.760, -157.800 + (-i * 4.5)};
    }
    for (int i = 0; i < 6; i++) {
        GameObject * ekperyment4 = eksp4Model->Instantiate(*scena, nullptr, nullptr);
        ekperyment4->name = "ekperyment4_b" + std::to_string(i);
        ekperyment4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7};
        ekperyment4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0, 0.0f };
        ekperyment4->GetComponent<TransformComponent>()->position = glm::vec3{ -5.000 ,7.760, -157.800 + (-i * 4.5)};
    }*/
    GameObject * fiolka_nast = fiolka_nastModel->Instantiate(*scena, nullptr, nullptr);
    fiolka_nast->name = "fiolka_nast";
    fiolka_nast->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10 };
    fiolka_nast->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90, 0.0f };
    fiolka_nast->GetComponent<TransformComponent>()->position = glm::vec3{ 6.910+7 ,7.650, -145.210};

    GameObject * probowka7d = probowka7Model->Instantiate(*scena, wozek, nullptr);
    probowka7d->name = "probowka7d";
    probowka7d->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25};
    probowka7d->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90.000f, 90.0f };
    probowka7d->GetComponent<TransformComponent>()->position = glm::vec3{ 0.150,0.370,0};
    GameObject * probowka7e = probowka7Model->Instantiate(*scena, wozek, nullptr);
    probowka7e->name = "probowka7e";
    probowka7e->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25};
    probowka7e->GetComponent<TransformComponent>()->rotation = glm::vec3{ 9.300f, 22.400, 90.0f };
    probowka7e->GetComponent<TransformComponent>()->position = glm::vec3{ -0.080,0.370,0.120};

    GameObject * papers2 = papersModel->Instantiate(*scena, wozek, nullptr);
    papers2->name = "papers2";
    papers2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 30.0f, 0.0f };
    papers2->GetComponent<TransformComponent>()->position = glm::vec3{ 0, 0, 0  };

    GameObject * fiolka_nastb = fiolka_nastModel->Instantiate(*scena, kredens7 , nullptr);
    fiolka_nastb->name = "fiolka_nastb";
    fiolka_nastb->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1 };
    fiolka_nastb->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90, 0.0f };
    fiolka_nastb->GetComponent<TransformComponent>()->position = glm::vec3{ 0 ,0.540, 0};

    GameObject * papers3 = papersModel->Instantiate(*scena, kredens7, nullptr);
    papers3->name = "papers3";
    papers3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 30.0f, 0.0f };
    papers3->GetComponent<TransformComponent>()->position = glm::vec3{ -0.640, 0.450, 0  };
    GameObject * papers4 = papersModel->Instantiate(*scena, kredens7, nullptr);
    papers4->name = "papers4";
    papers4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 245.0f, 0.0f };
    papers4->GetComponent<TransformComponent>()->position = glm::vec3{ -0.220, 0.450, 0  };
    GameObject * papers5 = papersModel->Instantiate(*scena, kredens7, nullptr);
    papers5->name = "papers5";
    papers5->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers5->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    papers5->GetComponent<TransformComponent>()->position = glm::vec3{ 0.270, 0.450, -0.1  };
    GameObject * telephone = telephoneModel->Instantiate(*scena, szafka_inna2, nullptr);
    telephone->name = "telephone";
    telephone->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    telephone->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -151.800, 0.0f };
    telephone->GetComponent<TransformComponent>()->position = glm::vec3{ 0.120, 0.630, -0.350  };
    GameObject * papers6 = papersModel->Instantiate(*scena, szafka_inna2, nullptr);
    papers6->name = "papers6";
    papers6->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers6->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    papers6->GetComponent<TransformComponent>()->position = glm::vec3{ 0.030, 0.650, -0.770  };
    GameObject * papers7 = papersModel->Instantiate(*scena, szafka_inna2, nullptr);
    papers7->name = "papers7";
    papers7->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers7->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    papers7->GetComponent<TransformComponent>()->position = glm::vec3{ 0.320, 0.640, -0.680  };
    GameObject * folder2 = folderModel->Instantiate(*scena, szafka_inna2, nullptr);
    folder2->name = "folder2";
    folder2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    folder2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    folder2->GetComponent<TransformComponent>()->position = glm::vec3{ 0.320, 0.710, -0.720  };
    SpawnLoreNote(scena, folderModel.get(), glm::vec3(1.060, 0.550, -0.430), "res/lore/Kalfu_ch7.txt", glm::vec3(0.0f, -55.500, 0.0f), glm::vec3(0.25), szafka_inna2);
    /*
    GameObject * folder3 = folderModel->Instantiate(*scena, szafka_inna2, nullptr);
    folder3->name = "folder3";
    folder3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    folder3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    folder3->GetComponent<TransformComponent>()->position = glm::vec3{ 1.060, 0.550, -0.430  };
    */
    GameObject * cup3 = cupModel->Instantiate(*scena, szafka_inna2, nullptr);
    cup3->name = "cup3";
    cup3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    cup3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    cup3->GetComponent<TransformComponent>()->position = glm::vec3{ 0.460, 0.630, -0.210  };
}

void createNuclearRooom(Scene* scena) {
    //CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaNucearRoom", glm::vec3(-120, 0, -180),  glm::vec3(60, 1, 80));
    //CreateStaticObject(scena, floorModel.get(), nullptr, "SufitATOM",         glm::vec3(-120, 20, -180), glm::vec3(60, 1, 80));
    //CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaKoncowaAtom", glm::vec3(-180, 0, -180),  glm::vec3(80, 50, 1), std::nullopt, glm::vec3(1, 50, 80));
    //CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaATOMPrawa",   glm::vec3(-120.180, 0, -259.680), glm::vec3(60, 50, 1), std::nullopt, glm::vec3(60, 100, 1));
}

void createCrematorium(Scene* scena) {


    GameObject * PodlogaKrematorium = CreateStaticObject(scena, floorModelB.get(), nullptr, "PodlogaKrematorium",   glm::vec3(125, 1.170, -150.000),  glm::vec3(3.490, 1, 2.870));
    PodlogaKrematorium->GetComponent< ColliderComponent>()->isWalkable = true;
    CreateStaticObject(scena, ceilingModelB.get(), nullptr, "SufitCrematorium",     glm::vec3(125.000, 19.160, -150.000), glm::vec3(3.490, 1, 2.870));

    GameObject * ScianaKoncowaKrematorium = CreateStaticObject(scena, wallCrematoriumModel.get(), nullptr, "ScianaKoncowaKrematorium", glm::vec3(160.280, 11.000, -149.030), glm::vec3(2.880, 2, 3), glm::vec3(0,-90,0));
    ScianaKoncowaKrematorium->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaKoncowaKrematorium->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKremPrawa = CreateStaticObject(scena, wallCrematoriumModel.get(),  nullptr, "ScianaKremPrawa",       glm::vec3(124.600, 11.000, -178.010), glm::vec3(3.540, 2.000, 3.000));
    ScianaKremPrawa->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaKremPrawa->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKremLewa = CreateStaticObject(scena, wallCrematoriumModel.get(),  nullptr, "ScianaKremLewa",       glm::vec3(91.190, 11.000, -150.390), glm::vec3(2.870, 2.000, 3.000), glm::vec3(0,90,0));
    ScianaKremLewa->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaKremLewa->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKremLewa2 = CreateStaticObject(scena, wallCrematoriumModel.get(),  nullptr, "ScianaKremLewa2",       glm::vec3(106.000, 10.890, -121.890), glm::vec3(1.500, 2, 2));
    ScianaKremLewa2->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaKremLewa2->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKremLewa3 = CreateStaticObject(scena, wallCrematoriumModel.get(),  nullptr, "ScianaKremLewa3",       glm::vec3(145.015, 10.890, -121.890), glm::vec3(1.610, 2, 2));
    ScianaKremLewa3->GetComponent< ColliderComponent>()->isWalkable = false;
    ScianaKremLewa3->GetComponent< ColliderComponent>()->affectsNavMesh = true;
    CreateStaticObject(scena, wallCrematoriumModel.get(),  nullptr, "ScianaKremLewaGora",       glm::vec3(127.290, 26.332, -122.070), glm::vec3(0.650, 2.170, 1));


    crematoriumPuzzle.spacingHorizontal = 7.0f;
    crematoriumPuzzle.spacingVertical   = 4.0f;

    crematoriumPuzzle.minExtensionDistance = 10.0f;
    crematoriumPuzzle.maxExtensionDistance = 30.0f;

    crematoriumPuzzle.coffinDimensions = glm::vec3(5.0f, 3.0f, 50.0f);

    crematoriumPuzzle.wallOffset = 0.0f;

    crematoriumPuzzle.w1_buildDirX  = -1.0f;
    crematoriumPuzzle.w1_extendDirZ =  1.0f;
    crematoriumPuzzle.w2_buildDirZ  =  1.0f;
    crematoriumPuzzle.w2_extendDirX = -1.0f;

    glm::vec3 cornerPosition(159.0f, 4.1f, -176.65f);

    if (coffinRedEmptyModel && panelModel) {
        crematoriumPuzzle.Init(scena, coffinRedEmptyModel.get(), coffinRedCorpseModel.get(), coffinGreenEmptyModel.get(), coffinGreenCorpseModel.get(), coffinBaseEmptyModel.get(), coffinBaseCorpseModel.get(), panelModel.get(), nullptr, cornerPosition);
    }
    else {
        spdlog::error("Modele kostnicy lub panelu niepoprawnie zaladowane");
    }
}

void createRentgenRoom(Scene* scena) {
    GameObject * podlogaRentgenRoom = CreateStaticObject(scena, floorModelB.get(), nullptr, "PodlogaRentgenRoom",    glm::vec3(-70.610, 1.170, -178.790),  glm::vec3(2.520, 1, 2.530));
    podlogaRentgenRoom->GetComponent<ColliderComponent>()->isWalkable = true;
    GameObject * sufitRentgen = CreateStaticObject(scena, ceilingModelB.get(), nullptr, "SufitRentgen",          glm::vec3(-70.610, 17.000, -178.790), glm::vec3(2.520, 1, 2.600));
    GameObject * KoncowaScianaRentgen = CreateStaticObject(scena, wallRentgenModel.get(),  nullptr, "KoncowaScianaRentgen",  glm::vec3(-70.220, 9.020, -204.180),          glm::vec3(2.490, 1.780, 3));
    KoncowaScianaRentgen->GetComponent<ColliderComponent>()->isWalkable = false;
    KoncowaScianaRentgen->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject* ScianaRentgen2 = CreateStaticObject(scena, wallRentgenModel.get(),  nullptr, "ScianaRentgen2",         glm::vec3(-88.874, 9.020, -152.920),  glm::vec3(1, 1.780, 1));
    ScianaRentgen2->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaRentgen2->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject* ScianaRentgen3 = CreateStaticObject(scena, wallRentgenModel.get(),  nullptr, "ScianaRentgen3",         glm::vec3(-56.970, 9.020, -152.920),  glm::vec3(1.470, 1.780, 1));
    ScianaRentgen3->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaRentgen3->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * LewaScianaRentgen = CreateStaticObject(scena, wallRentgenModel.get(),  nullptr, "LewaScianaRentgen",    glm::vec3(-95.340, 9.020, -178.790),  glm::vec3(2.630, 1.780, 3), glm::vec3(180, 90, 180));
    LewaScianaRentgen->GetComponent<ColliderComponent>()->isWalkable = false;
    LewaScianaRentgen->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * PrawaScianaRentgen = CreateStaticObject(scena, wallRentgenModel.get(),  nullptr, "PrawaScianaRentgen",    glm::vec3(-54.970+10, 9.020, -178.790),  glm::vec3(2.630, 1.780, 3.000), glm::vec3(180, 90, 180));
    PrawaScianaRentgen->GetComponent<ColliderComponent>()->isWalkable = false;
    PrawaScianaRentgen->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaRentgenGora = CreateStaticObject(scena, wallRentgenModel.get(),  nullptr, "GoraPrzejscieRentgen",    glm::vec3(-76.660, 16.504, -152.920),  glm::vec3(0.530, 0.090, 0.960));
    ScianaRentgenGora->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaRentgenGora->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * objPuzel1 = puzel1->Instantiate(*scena, nullptr, nullptr);
    objPuzel1->name = "puzel1";
    objPuzel1->GetComponent<TransformComponent>()->position = glm::vec3(-55.527, 3.340,-166.800);
    objPuzel1->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel1->AddComponent<RigidbodyComponent>();
    objPuzel1->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel1->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel1->AddComponent<ColliderComponent>();
    objPuzel1->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalColliderSizes[objPuzel1] = objPuzel1->GetComponent<ColliderComponent>()->halfSize;
    objectOriginalRotations[objPuzel1] = objPuzel1->GetComponent<TransformComponent>()->rotation;
    objectOriginalPositions[objPuzel1] = objPuzel1->GetComponent<TransformComponent>()->position;
    pickupObjects.insert(objPuzel1);

    GameObject * objPuzel2 = puzel2->Instantiate(*scena, nullptr, nullptr);
    objPuzel2->name = "puzel2";
    objPuzel2->GetComponent<TransformComponent>()->position = glm::vec3(57.112, 8.537, -167.965);
    objPuzel2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel2->AddComponent<RigidbodyComponent>();
    objPuzel2->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel2->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel2->AddComponent<ColliderComponent>();
    objPuzel2->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalColliderSizes[objPuzel2] = objPuzel2->GetComponent<ColliderComponent>()->halfSize;
    objectOriginalRotations[objPuzel2] = objPuzel2->GetComponent<TransformComponent>()->rotation;
    objectOriginalPositions[objPuzel2] = objPuzel2->GetComponent<TransformComponent>()->position;
    pickupObjects.insert(objPuzel2);

    GameObject * objPuzel3 = puzel3->Instantiate(*scena, nullptr, nullptr);
    objPuzel3->name = "puzel3";
    objPuzel3->GetComponent<TransformComponent>()->position = glm::vec3(-88.680, 6.523, -168.894);
    objPuzel3->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel3->AddComponent<RigidbodyComponent>();
    objPuzel3->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel3->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel3->AddComponent<ColliderComponent>();
    objPuzel3->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalColliderSizes[objPuzel3] = objPuzel3->GetComponent<ColliderComponent>()->halfSize;
    objectOriginalRotations[objPuzel3] = objPuzel3->GetComponent<TransformComponent>()->rotation;
    objectOriginalPositions[objPuzel3] = objPuzel3->GetComponent<TransformComponent>()->position;
    pickupObjects.insert(objPuzel3);

    GameObject * objPuzel4 = puzel4->Instantiate(*scena, nullptr, nullptr);
    objPuzel4->name = "puzel4";
    objPuzel4->GetComponent<TransformComponent>()->position = glm::vec3(-63.418, 4.528, -155.781);
    objPuzel4->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel4->AddComponent<RigidbodyComponent>();
    objPuzel4->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel4->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel4->AddComponent<ColliderComponent>();
    objPuzel4->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalColliderSizes[objPuzel4] = objPuzel4->GetComponent<ColliderComponent>()->halfSize;
    objectOriginalRotations[objPuzel4] = objPuzel4->GetComponent<TransformComponent>()->rotation;
    objectOriginalPositions[objPuzel4] = objPuzel4->GetComponent<TransformComponent>()->position;
    pickupObjects.insert(objPuzel4);

    GameObject * objPuzel5 = puzel5->Instantiate(*scena, nullptr, nullptr);
    objPuzel5->name = "puzel5";
    objPuzel5->GetComponent<TransformComponent>()->position = glm::vec3(-69.744, 6.271, -200.524);
    objPuzel5->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel5->AddComponent<RigidbodyComponent>();
    objPuzel5->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel5->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel5->AddComponent<ColliderComponent>();
    objPuzel5->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalColliderSizes[objPuzel5] = objPuzel5->GetComponent<ColliderComponent>()->halfSize;
    objectOriginalRotations[objPuzel5] = objPuzel5->GetComponent<TransformComponent>()->rotation;
    objectOriginalPositions[objPuzel5] = objPuzel5->GetComponent<TransformComponent>()->position;
    pickupObjects.insert(objPuzel5);

    GameObject * objPuzel6 = puzel6->Instantiate(*scena, nullptr, nullptr);
    objPuzel6->name = "puzel6";
    objPuzel6->GetComponent<TransformComponent>()->position = glm::vec3(-7.130, 9.537, -130.855);
    objPuzel6->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel6->AddComponent<RigidbodyComponent>();
    objPuzel6->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel6->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel6->AddComponent<ColliderComponent>();
    objPuzel6->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalColliderSizes[objPuzel6] = objPuzel6->GetComponent<ColliderComponent>()->halfSize;
    objectOriginalRotations[objPuzel6] = objPuzel6->GetComponent<TransformComponent>()->rotation;
    objectOriginalPositions[objPuzel6] = objPuzel6->GetComponent<TransformComponent>()->position;
    pickupObjects.insert(objPuzel6);

    GameObject * rentgen = Rentgen->Instantiate(*scena, nullptr, nullptr);
    rentgen->name = "RentgenTablica";
    rentgen->GetComponent<TransformComponent>()->position =glm::vec3(-83.319+10, 10.160, -213.257+10);
    rentgen->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    rentgen->GetComponent<TransformComponent>()->scale = glm::vec3(1.5);

    rentgen->TraverseChildren([&](GameObject* go) {
        if (go->name.find("lights_") != std::string::npos) {
            machineLights[go->name] = go;

            LightComponent* lc = go->AddComponent<LightComponent>();
            lc->type = Point;
            lc->ambient = glm::vec3(0.1f, 0.05f, 0.0f);
            lc->diffuse = glm::vec3(0.4f, 0.4f, 1.0f);
            lc->specular = glm::vec3(1.0f, 0.4f, 0.0f);
            lc->constant = 1.0f;
            lc->linear = 0.22f;
            lc->quadratic = 0.20f;
            lc->intensity = 25.0f;
            lc->isOn = true;
        }


    });



    int puzzleLightIndex = 4;
    auto createPuzzleSlot = [&](const glm::vec3& pos, const glm::vec3& targetRot, GameObject* expected) {
        GameObject* slotGO = scena->CreateGameObject(nullptr);
        slotGO->name = "PuzzleSlot_" + expected->name;

        TransformComponent* tr = slotGO->AddComponent<TransformComponent>();
        tr->position = pos;

        ColliderComponent* col = slotGO->AddComponent<ColliderComponent>();
        col->halfSize  = glm::vec3(0.840, 1.390, 0.330);

        RigidbodyComponent* rb = slotGO->AddComponent<RigidbodyComponent>();
        rb->useGravity = false;
        rb->isStatic   = true;

        GameObject* lightGO = scena->CreateGameObject(nullptr);
        lightGO->name = "PuzzleLight_" + expected->name;
        TransformComponent* lightTr = lightGO->AddComponent<TransformComponent>();
        lightTr->position = pos + glm::vec3(0.0f, 2.0f, 1.0);

        LightComponent* light = lightGO->AddComponent<LightComponent>();
        light->type      = Point;
        light->index = puzzleLightIndex++;
        light->isOn      = false;
        light->ambient   = glm::vec3(0.0f);
        light->diffuse   = glm::vec3(0.0f);
        light->specular  = glm::vec3(0.0f);
        light->constant  = 15.0f;
        light->linear    = 0.3f;
        light->quadratic = 0.05f;
        light->range     = 15.0f;
        light->intensity = 20;

        PuzzleSlot slot;
        slot.targetRotation = targetRot;
        slot.slotObject     = slotGO;
        slot.expectedObject = expected;
        slot.lightObject    = lightGO;
        if (auto col = expected->GetComponent<ColliderComponent>()) {
            objectOriginalColliderSizes[expected] = col->halfSize;
        }
        puzzleSlotsMap[slotGO] = slot;
    };

    createPuzzleSlot(glm::vec3(-85.052+10, 11.430, -212.70+10), glm::vec3(0, -90,  -0), objPuzel2);
    createPuzzleSlot(glm::vec3(-83.382+10, 11.430, -212.70+10), glm::vec3(0, -90, 0), objPuzel6);
    createPuzzleSlot(glm::vec3(-81.820+10, 11.430, -212.70+10), glm::vec3(0, -90, 0), objPuzel4);
    createPuzzleSlot(glm::vec3(-85.052+10, 9, -212.70+10), glm::vec3(0, -90, 0), objPuzel1);
    createPuzzleSlot(glm::vec3(-83.382+10, 9, -212.70+10), glm::vec3(0, -90, 0), objPuzel3);
    createPuzzleSlot(glm::vec3(-81.820+10, 9, -212.70+10), glm::vec3(0, -90, 0), objPuzel5);
    
    GameObject* triggerRentgen = scena->CreateGameObject(rentgen);
    triggerRentgen->name = "triggerRentgen";
    triggerRentgen->GetComponent<TransformComponent>()->position = glm::vec3(5.0f, 0.0f, 0.0f);

    ColliderComponent* colliderRentgen = triggerRentgen->AddComponent<ColliderComponent>();
    colliderRentgen->halfSize = glm::vec3(15.0f);
    colliderRentgen->isTrigger = true;
    colliderRentgen->onTriggerEnter = [](GameObject* other)
    {
        if (other->name == "Gracz1" || other->name == "Gracz2")
        {
            vector<LightComponent*> lights = other->GetComponentsInChildren<LightComponent>();

            for (auto& light : lights)
            {
                light->intensity = 50.0f;
            }
        }
    };

    colliderRentgen->onTriggerExit = [](GameObject* other)
    {
        if (other->name == "Gracz1" || other->name == "Gracz2")
        {
            vector<LightComponent*> lights = other->GetComponentsInChildren<LightComponent>();

            for (auto& light : lights)
            {
                light->intensity = 650.0f;
            }
        }

    };

    //-85.197, 6.250, -182.170
    GameObject * lampaOperacyjna = lampaOperacyjnaModel->Instantiate(*scena, nullptr, nullptr);
    lampaOperacyjna->name = "lampaOperacyjna";
    lampaOperacyjna->GetComponent<TransformComponent>()->position =glm::vec3(-51.070, 9.800, -164.000);
    lampaOperacyjna->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 361.400, 0);
    lampaOperacyjna->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    lampaOperacyjna->AddComponent<ColliderComponent>();
    lampaOperacyjna->GetComponent<ColliderComponent>()->isWalkable = false;
    lampaOperacyjna->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * stolOperacyjny = stolOperacyjnyModel->Instantiate(*scena, nullptr, nullptr);
    stolOperacyjny->name = "stolOperacyjny";
    stolOperacyjny->GetComponent<TransformComponent>()->position =glm::vec3(-55.527, 3.340, -166.800);
    stolOperacyjny->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -209.800, 0);
    stolOperacyjny->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    stolOperacyjny->AddComponent<ColliderComponent>();
    stolOperacyjny->GetComponent<ColliderComponent>()->halfSize = glm::vec3(4.234, 2.535, 5.295);
    stolOperacyjny->GetComponent<ColliderComponent>()->offset = glm::vec3(0.385, -0.212, 0.256);
    stolOperacyjny->GetComponent<ColliderComponent>()->isWalkable = false;
    stolOperacyjny->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * szafka1Rentgen = SzafkaRentgen1Model->Instantiate(*scena, nullptr, nullptr);
    szafka1Rentgen->name = "szafka1Rentgen";
    szafka1Rentgen->GetComponent<TransformComponent>()->position =glm::vec3(-49.367, 8.240, -210.290+10);
    szafka1Rentgen->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -43.900, 0);
    szafka1Rentgen->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    szafka1Rentgen->AddComponent<ColliderComponent>();
    szafka1Rentgen->GetComponent<ColliderComponent>()->isWalkable = false;
    szafka1Rentgen->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * szafka1Rentgenb = SzafkaRentgen1Model->Instantiate(*scena, nullptr, nullptr);
    szafka1Rentgenb->name = "szafka1Rentgenb";
    szafka1Rentgenb->GetComponent<TransformComponent>()->position =glm::vec3(-47.327, 8.040, -197.000+10);
    szafka1Rentgenb->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -89.000, 0);
    szafka1Rentgenb->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    szafka1Rentgenb->AddComponent<ColliderComponent>();
    szafka1Rentgenb->GetComponent<ColliderComponent>()->isWalkable = false;
    szafka1Rentgenb->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * szafka2Rentgen = SzafkaRentgen2Model->Instantiate(*scena, nullptr, nullptr);
    szafka2Rentgen->name = "szafka2Rentgen";
    szafka2Rentgen->GetComponent<TransformComponent>()->position =glm::vec3(-53.037, 7.900, -202.100+10);
    szafka2Rentgen->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90.000, 0);
    szafka2Rentgen->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    szafka2Rentgen->AddComponent<ColliderComponent>();
    szafka2Rentgen->GetComponent<ColliderComponent>()->isWalkable = false;
    szafka2Rentgen->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * zaslona = zaslonaModel->Instantiate(*scena, nullptr, nullptr);
    zaslona->name = "zaslona";
    zaslona->GetComponent<TransformComponent>()->position =glm::vec3(-62.397, 7.860, -172.550);
    zaslona->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 236.500, 0);
    zaslona->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    zaslona->AddComponent<ColliderComponent>();
    zaslona->GetComponent<ColliderComponent>()->isWalkable = false;
    zaslona->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * zaslonab = zaslonaModel->Instantiate(*scena, nullptr, nullptr);
    zaslonab->name = "zaslonab";
    zaslonab->GetComponent<TransformComponent>()->position =glm::vec3(-58.437, 7.860, -175.840);
    zaslonab->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 207.700, 0);
    zaslonab->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    zaslonab->AddComponent<ColliderComponent>();
    zaslonab->GetComponent<ColliderComponent>()->isWalkable = false;
    zaslonab->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * zaslonac = zaslonaModel->Instantiate(*scena, nullptr, nullptr);
    zaslonac->name = "zaslonac";
    zaslonac->GetComponent<TransformComponent>()->position =glm::vec3(-64.907, 7.860, -167.890);
    zaslonac->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 255.500, 0);
    zaslonac->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    zaslonac->AddComponent<ColliderComponent>();
    zaslonac->GetComponent<ColliderComponent>()->isWalkable = false;
    zaslonac->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    //-73.385, 6.250 -175.377
    GameObject * wozek = wozekModel->Instantiate(*scena, nullptr, nullptr);
    wozek->name = "wozek";
    wozek->GetComponent<TransformComponent>()->position =glm::vec3(-61.165, 4.310, -185.587);
    wozek->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -31.900, 0);
    wozek->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    wozek->AddComponent<ColliderComponent>();
    GameObject * wozek2 = wozekModel->Instantiate(*scena, nullptr, nullptr);
    wozek2->name = "wozek2";
    wozek2->GetComponent<TransformComponent>()->position =glm::vec3(-63.885, 4.310, -156.157);
    wozek2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 0, 0);
    wozek2->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    wozek2->AddComponent<ColliderComponent>();
	wozek2->GetComponent<ColliderComponent>()->offset = glm::vec3(0, -3.050, 0);

    GameObject * collWozek2 = scena->CreateGameObject(nullptr);
    collWozek2->name = "collWozek2";
    TransformComponent * transform = collWozek2->GetComponent<TransformComponent>();
    transform->position = glm::vec3(-63.885, 6.245, -156.073);
    wozek2->GetComponent<TransformComponent>()->position =glm::vec3(-63.541, 4.228, -156.157);
    ColliderComponent* colliderComnponent = collWozek2->AddComponent<ColliderComponent>();
    colliderComnponent->halfSize = glm::vec3(2.560, 0.270, 1.620);
    colliderComnponent->offset = glm::vec3(0);


    GameObject * desk = deskModel->Instantiate(*scena, nullptr, nullptr);
    desk->name = "desk";
    desk->GetComponent<TransformComponent>()->position =glm::vec3(-73.025, 0.860, -200.417);
    desk->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90.000, 0);
    desk->GetComponent<TransformComponent>()->scale = glm::vec3(3);
    desk->AddComponent<ColliderComponent>();

    GameObject * needle = needleModel->Instantiate(*scena, desk, nullptr);
    needle->name = "needle";
    needle->GetComponent<TransformComponent>()->position =glm::vec3(0.040, 1.770, 2.030);
    needle->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 38.600, 0);
    needle->GetComponent<TransformComponent>()->scale = glm::vec3(1);
    GameObject * needle2 = needleModel->Instantiate(*scena, desk, nullptr);
    needle2->name = "needle2";
    needle2->GetComponent<TransformComponent>()->position =glm::vec3(0.310, 1.770, 1.870);
    needle2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 64.600, 0);
    needle2->GetComponent<TransformComponent>()->scale = glm::vec3(1);
    GameObject * needle3 = needleModel->Instantiate(*scena, desk, nullptr);
    needle3->name = "needle3";
    needle3->GetComponent<TransformComponent>()->position =glm::vec3(0.420, 1.770, 2.170);
    needle3->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 44.400, 0);
    needle3->GetComponent<TransformComponent>()->scale = glm::vec3(1);

    GameObject * needle4 = needleModel->Instantiate(*scena, wozek, nullptr);
    needle4->name = "needle4";
    needle4->GetComponent<TransformComponent>()->position =glm::vec3(0.170, 0.360, 0);
    needle4->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 44.400, 0);
    needle4->GetComponent<TransformComponent>()->scale = glm::vec3(0.5);
    GameObject * needle5 = needleModel->Instantiate(*scena, wozek, nullptr);
    needle5->name = "needle5";
    needle5->GetComponent<TransformComponent>()->position =glm::vec3(0, 0.330, 0);
    needle5->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -10.900, 0);
    needle5->GetComponent<TransformComponent>()->scale = glm::vec3(0.5);

    GameObject * sink = sinkModel->Instantiate(*scena, nullptr, nullptr);
    sink->name = "sink";
    sink->GetComponent<TransformComponent>()->position =glm::vec3(-87.185, 6.250, -201.537);
    sink->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 0, 0);
    sink->GetComponent<TransformComponent>()->scale = glm::vec3(2);
    sink->AddComponent<ColliderComponent>();

    GameObject * drawer1 = drawer1Model->Instantiate(*scena, nullptr, nullptr);
    drawer1->name = "drawer1";
    drawer1->GetComponent<TransformComponent>()->position =glm::vec3(-92.145, 12.860, -199.387);
    drawer1->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    drawer1->GetComponent<TransformComponent>()->scale = glm::vec3(3);
    drawer1->AddComponent<ColliderComponent>();

    GameObject * drawer2 = drawer2Model->Instantiate(*scena, nullptr, nullptr);
    drawer2->name = "drawer2";
    drawer2->GetComponent<TransformComponent>()->position =glm::vec3(-87.355, 12.860, -199.637);
    drawer2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    drawer2->GetComponent<TransformComponent>()->scale = glm::vec3(3);
    drawer2->AddComponent<ColliderComponent>();

    GameObject * stol = tableModel->Instantiate(*scena, nullptr, nullptr);
    stol->name = "stol";
    stol->GetComponent<TransformComponent>()->position =glm::vec3(-89.965, 3.100, -162.607);
    stol->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 0, 0);
    stol->GetComponent<TransformComponent>()->scale = glm::vec3(8.000);
    stol->AddComponent<ColliderComponent>();

    GameObject * kredens = kredensModel->Instantiate(*scena, nullptr, nullptr);
    kredens->name = "kredens";
    kredens->GetComponent<TransformComponent>()->position =glm::vec3(-91.795, 3.840, -182.037);
    kredens->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 90, 0);
    kredens->GetComponent<TransformComponent>()->scale = glm::vec3(6);
    kredens->AddComponent<ColliderComponent>();
    kredens->GetComponent<ColliderComponent>()->isWalkable = false;
    kredens->GetComponent<ColliderComponent>()->affectsNavMesh = true;

    GameObject * eksp1a = eksp1Model->Instantiate(*scena, kredens, nullptr);
    eksp1a->name = "eksp1a";
    eksp1a->GetComponent<TransformComponent>()->position =glm::vec3(-1.240, 0.590, -0.160);
    eksp1a->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    eksp1a->GetComponent<TransformComponent>()->scale = glm::vec3(1);
    GameObject * eksp1b = eksp1Model->Instantiate(*scena, kredens, nullptr);
    eksp1b->name = "eksp1b";
    eksp1b->GetComponent<TransformComponent>()->position =glm::vec3(0.080, 0.590, -0.160);
    eksp1b->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    eksp1b->GetComponent<TransformComponent>()->scale = glm::vec3(1);
    GameObject * eksp1c = eksp1Model->Instantiate(*scena, kredens, nullptr);
    eksp1c->name = "eksp1c";
    eksp1c->GetComponent<TransformComponent>()->position =glm::vec3(1.220, 0.590, -0.160);
    eksp1c->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    eksp1c->GetComponent<TransformComponent>()->scale = glm::vec3(1);
    GameObject * laboStuff1 = laboratoryStuff1Model->Instantiate(*scena, kredens, nullptr);
    laboStuff1->name = "laboStuff1";
    laboStuff1->GetComponent<TransformComponent>()->position =glm::vec3(0.600, 0.920, -0.060);
    laboStuff1->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -173.700, 0);
    laboStuff1->GetComponent<TransformComponent>()->scale = glm::vec3(0.5);
    GameObject * laboStuff2 = laboratoryStuff2Model->Instantiate(*scena, kredens, nullptr);
    laboStuff2->name = "laboStuff2";
    laboStuff2->GetComponent<TransformComponent>()->position =glm::vec3(-0.630, 0.830, -0.170);
    laboStuff2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    laboStuff2->GetComponent<TransformComponent>()->scale = glm::vec3(1);

    GameObject * laboStuff3a = laboratoryStuff3Model->Instantiate(*scena, stol, nullptr);
    laboStuff3a->name = "laboStuff3a";
    laboStuff3a->GetComponent<TransformComponent>()->position =glm::vec3(-0.180, 0.420, -0.380);
    laboStuff3a->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -105.600, 0);
    laboStuff3a->GetComponent<TransformComponent>()->scale = glm::vec3(1);
    GameObject * laboStuff3b = laboratoryStuff3Model->Instantiate(*scena, stol, nullptr);
    laboStuff3b->name = "laboStuff3b";
    laboStuff3b->GetComponent<TransformComponent>()->position =glm::vec3(0.210, 0.420, 0.460);
    laboStuff3b->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -101.300, 0);
    laboStuff3b->GetComponent<TransformComponent>()->scale = glm::vec3(1);

    GameObject * probowka_4a = probowka4Model->Instantiate(*scena, wozek2, nullptr);
    probowka_4a->name = "probowka_4a";
    probowka_4a->GetComponent<TransformComponent>()->position =glm::vec3(0.000, 0.500, 0.090);
    probowka_4a->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -187.900, 0);
    probowka_4a->GetComponent<TransformComponent>()->scale = glm::vec3(0.025);
    GameObject * probowka_4b = probowka4Model->Instantiate(*scena, wozek2, nullptr);
    probowka_4b->name = "probowka_4b";
    probowka_4b->GetComponent<TransformComponent>()->position =glm::vec3(0.270, 0.500, -0.030);
    probowka_4b->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -101.300, 0);
    probowka_4b->GetComponent<TransformComponent>()->scale = glm::vec3(0.025);

    //GameObject * ksiazka = ksiazkaModel->Instantiate(*scena, wozek2, nullptr);
    //ksiazka->name = "ksiazka";
    //ksiazka->GetComponent<TransformComponent>()->position =glm::vec3(-0.160, 0.350, -0.030);
    //ksiazka->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -101.300, 0);
    //ksiazka->GetComponent<TransformComponent>()->scale = glm::vec3(1);

    SpawnLoreNote(scena, ksiazkaModel.get(), glm::vec3(-0.160, 0.350, -0.030), "res/lore/Expedition.txt", glm::vec3(0, -101.300, 0), glm::vec3(1), wozek2);

    //glm::vec3 basePos = glm::vec3(-73.025f, 1.0f, -190.417f);

    //SpawnLoreNote(scena, ksiazkaModel.get(), basePos + glm::vec3(1.5f, 0.0f, 0.0f), "res/lore/Kalfu_ch4.txt", glm::vec3(1.0), glm::vec3(10.0));

    //SpawnLoreNote(scena, ksiazkaModel.get(), basePos + glm::vec3(3.0f, 0.0f, 0.0f), "res/lore/Kalfu_ch6.txt", glm::vec3(1.0), glm::vec3(10.0));

    //SpawnLoreNote(scena, ksiazkaModel.get(), basePos + glm::vec3(4.5f, 0.0f, 0.0f), "res/lore/Kalfu_ch7.txt", glm::vec3(1.0), glm::vec3(10.0));
}
void createRentgenCorridor(Scene * scena){
    GameObject * ScianaLewaKorytarzRentgen = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaLewaKorytarzRentgen",         glm::vec3(-52.900, 10.780, -139.190),  glm::vec3(4.230, 1.930, 3.000));
    ScianaLewaKorytarzRentgen->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaLewaKorytarzRentgen->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaPrawaKorytarzRentgen1 = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaPrawaKorytarzRentgen1",         glm::vec3(-85.215, 10.780, -152.410),  glm::vec3(0.630, 1.930, 2.000));
    ScianaPrawaKorytarzRentgen1->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaPrawaKorytarzRentgen1->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaPrawaKorytarzRentgen2 = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaPrawaKorytarzRentgen2",         glm::vec3(-41.200, 10.780, -152.410),  glm::vec3(3.040, 1.930, 2));
    ScianaPrawaKorytarzRentgen2->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaPrawaKorytarzRentgen2->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKoncowaKorytarzRentgen = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaKoncowaKorytarzRentgen",         glm::vec3(-91.890, 10.780, -144.840),  glm::vec3(0.820, 1.930, 3),glm::vec3(0,90,0));
    ScianaKoncowaKorytarzRentgen->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaKoncowaKorytarzRentgen->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKoncowaKorytarzRentgen1 = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaKoncowaKorytarzRentgen1",         glm::vec3(-10.830, 10.780, -140.320),  glm::vec3(0.060, 1.930, 1),glm::vec3(0,90,0));
    ScianaKoncowaKorytarzRentgen1->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaKoncowaKorytarzRentgen1->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKoncowaKorytarzRentgen2 = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaKoncowaKorytarzRentgen2",         glm::vec3(-10.512, 10.780, -149.959),  glm::vec3(0.270, 1.930, 3.000),glm::vec3(0,90,0));
    ScianaKoncowaKorytarzRentgen2->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaKoncowaKorytarzRentgen2->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKoncowaKorytarzRentgen3 = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaKoncowaKorytarzRentgen3",         glm::vec3(-10.830, 18.030, -145.760),  glm::vec3(0.510, 0.410, 1),glm::vec3(0,90,0));
    ScianaKoncowaKorytarzRentgen3->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaKoncowaKorytarzRentgen3->GetComponent<ColliderComponent>()->affectsNavMesh = true;

    CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "GoraPrzejscieDoRentgenaZKorytarza",              glm::vec3(-76.660, 18.040, -152.220),   glm::vec3(1, 0.360, 1));

    GameObject * PodlogaKorytarzRentgen = CreateStaticObject(scena, floorModelB.get(), nullptr, "PodlogaKorytarzRentgen",    glm::vec3(-52.750, 1.170, -145.280),  glm::vec3(4.210, 1, 0.820));
    PodlogaKorytarzRentgen->GetComponent<ColliderComponent>()->isWalkable = true;

    CreateStaticObject(scena, ceilingModelB.get(), nullptr, "SufitKorytarzRentgen",    glm::vec3(-52.750, 20, -145.150),  glm::vec3(4.210, 1, 0.790));
    GameObject* hingeDrzwiDoRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoRentgenZKorytarza",
        glm::vec3(-77, 7.300f, -152.310), glm::vec3(2.500, 3.400, 1),
        glm::vec3(6.0f, 0.0f, 0.0f), glm::vec3(5.7f, 20.0f, 1.0f), -90.0f, 0.0f, glm::vec3(1.0f, 20.0f, 4.670), glm::vec3(0.0f, 0.0f, -5.070), glm::vec3(2, 0.0f, 0.0f)
    );
    unlockedDoors.insert(hingeDrzwiDoRentgen);

}

void createCrematoriumCorridor(Scene * scena){
    GameObject * PodlogaKorytarzKrematorium = CreateStaticObject(scena, floorModelB.get(), nullptr, "PodlogaKorytarzKrematorium",    glm::vec3(101.060, 1.170, -113.820),  glm::vec3(4.060, 1, 0.750));
    PodlogaKorytarzKrematorium->GetComponent<ColliderComponent>()->isWalkable = true;
    CreateStaticObject(scena, ceilingModelB.get(), nullptr, "SufitKorytarzKrematorium",    glm::vec3(101.180, 20, -114.550),  glm::vec3(4.060, 1, 0.750));

    GameObject * ScianaLewaKorytarzKrematorium = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaLewaKorytarzKrematorium",         glm::vec3(100.920, 10.780, -107.230),  glm::vec3(4.080, 1.930, 3.000));
    ScianaLewaKorytarzKrematorium->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaLewaKorytarzKrematorium->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaPrawaKorytarzKrematorium1 = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaPrawaKorytarzKrematorium1",         glm::vec3(135.202, 10.780, -121.430),  glm::vec3(0.630, 1.930, 2.000));
    ScianaPrawaKorytarzKrematorium1->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaPrawaKorytarzKrematorium1->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaPrawaKorytarzKrematorium2 = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaPrawaKorytarzKrematorium2",         glm::vec3(90.490, 10.780, -121.660),  glm::vec3(3.050, 1.930, 3));
    ScianaPrawaKorytarzKrematorium2->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaPrawaKorytarzKrematorium2->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    GameObject * ScianaKoncowaKorytarzKrematorium = CreateStaticObject(scena, wallMainRoomModel.get(),  nullptr, "ScianaKoncowaKorytarzKrematorium",         glm::vec3(141.810, 10.780, -113.790),  glm::vec3(0.760, 1.930, 3), glm::vec3(0,90,0));
    ScianaKoncowaKorytarzKrematorium->GetComponent<ColliderComponent>()->isWalkable = false;
    ScianaKoncowaKorytarzKrematorium->GetComponent<ColliderComponent>()->affectsNavMesh = true;

    CreateStaticObject(scena, wallMainRoomModel.get(), nullptr, "GoraPrzejscieDoKrematoriumZKorytarza",              glm::vec3(127.090, 18.060, -121.260),   glm::vec3(1.000, 0.480, 1));
    GameObject* hingeDrzwiDoRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoKrematoriumZKorytarza",
        glm::vec3(123.122, 7.300f, -121.618), glm::vec3(2.500, 3.400, 1),
        glm::vec3(6.0f, 0.0f, 0.0f), glm::vec3(5.7f, 20.0f, 1.0f), -90.0f, 0.0f, glm::vec3(1.0f, 20.0f, 4.670), glm::vec3(0.0f, 0.0f, -5.070), glm::vec3(2, 0.0f, 0.0f)
    );
    unlockedDoors.insert(hingeDrzwiDoRentgen);
}

void RoomPlayerEntered(int roomIndex, GameObject* player)
{
    auto& room = roomsLights[roomIndex];
    bool wasEmpty = room.occupants.empty();

    room.occupants.insert(player);

    if (wasEmpty)
    {
        if (room.savedStates.size() == room.lights.size())
        {
            for (size_t j = 0; j < room.lights.size(); j++)
                room.lights[j]->isOn = room.savedStates[j];
        }
    }
}

void RoomPlayerExited(int roomIndex, GameObject* player)
{
    auto& room = roomsLights[roomIndex];
    room.occupants.erase(player);

    if (room.occupants.empty())
    {
        room.savedStates.resize(room.lights.size());
        for (size_t j = 0; j < room.lights.size(); j++)
        {
            room.savedStates[j] = room.lights[j]->isOn;
            room.lights[j]->isOn = false;
        }
    }
}

void createTriggerRoom(Scene* scena, int roomId, std::string name, glm::vec3 position, glm::vec3 halfSize)
{
    GameObject* triggerRoom = scena->CreateGameObject(nullptr);
    triggerRoom->name = name + "trigger";

    TransformComponent* transform = triggerRoom->GetComponent<TransformComponent>();
    transform->position = position;

    ColliderComponent* colliderComnponent = triggerRoom->AddComponent<ColliderComponent>();
    colliderComnponent->halfSize = halfSize;
    colliderComnponent->isTrigger = true;
    colliderComnponent->onTriggerEnter = [roomId](GameObject* other)
    {
        if (other->name == "Gracz1" || other->name == "Gracz2")
        {
            spdlog::info("{} wszed� do pomieszczenia {}", other->name, roomId);
            RoomPlayerEntered(roomId, other);
        }

    };

    colliderComnponent->onTriggerExit = [roomId](GameObject* other)
    {
        if (other->name == "Gracz1" || other->name == "Gracz2")
        {
            spdlog::info("{} wyszed� z pomieszczenia {}", other->name, roomId);
            RoomPlayerExited(roomId, other);
        }

    };
}


void InitializeRoomLights(int startingRoomId)
{
    for (size_t i = 0; i < roomsLights.size(); i++)
    {
        auto& room = roomsLights[i];

        if ((int)i == startingRoomId)
            continue;

        room.savedStates.resize(room.lights.size());
        for (size_t j = 0; j < room.lights.size(); j++)
        {
            room.savedStates[j] = room.lights[j]->isOn;
            room.lights[j]->isOn = false;
        }
    }
}

void createTrigger(Scene* scena)
{

    for (auto& room : roomsLights)
    {
        createTriggerRoom(scena, room.id, room.name, room.position, room.halfSize);
    }

    InitializeRoomLights(0);
}

