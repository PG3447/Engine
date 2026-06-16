 #ifndef MIMICRY_EXPERIMENTS_AUDIOSYSTEM_H
 #define MIMICRY_EXPERIMENTS_AUDIOSYSTEM_H

 #include <core/ecs.h>
 #include <fmod.hpp>
 #include <string>

 class AudioSystem : public System {
 private:
     FMOD::System* system;
     FMOD::Channel* channel;


 public:
     void OnGameObjectUpdated(GameObject* e) override;

     void Update(ECS&, float dt) override;

     AudioSystem(ECS& ecs) {
         FMOD::System_Create(&system);
         system->init(512, FMOD_INIT_NORMAL, nullptr);
     };

     ~AudioSystem() {
         system->close();
         system->release();
     }

     void createSound(std::string name, FMOD::Sound*& sound, bool loop = false) {
         FMOD_MODE mode = FMOD_DEFAULT;
         if (loop) {
             mode |= FMOD_LOOP_NORMAL;
         }
         system->createSound(name.c_str(), mode, nullptr, &sound);
     }

     void playSound(FMOD::Sound* sound) {
         system->playSound(sound, nullptr, false, &channel);
     }

     FMOD::Channel* playSoundEx(FMOD::Sound* sound) {
         FMOD::Channel* channel = nullptr;
         system->playSound(sound, nullptr, false, &channel);
         return channel;
     }

 };


 #endif //MIMICRY_EXPERIMENTS_AUDIOSYSTEM_H