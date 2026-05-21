#ifndef RANDOM_HELPER_H
#define RANDOM_HELPER_H

#include <random>

class RandomHelper {
private:
    static std::mt19937& getEngine() {
        static std::random_device rd;
        static std::mt19937 engine(rd());
        return engine;
    }

public:
    static float Range(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(getEngine());
    }

    static int RangeInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(getEngine());
    }
};

#endif