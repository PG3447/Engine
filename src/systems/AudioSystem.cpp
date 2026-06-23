 #include "AudioSystem.h"

 void AudioSystem::OnGameObjectUpdated(GameObject *e) {
     //unused
 }

 void AudioSystem::Update(ECS&, float dt) {
     if (system) {
         system->update();
     }
 }