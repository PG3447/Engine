//
// Created by CKLG on 26.04.2026.
//

#ifndef MIMICRY_EXPERIMENTS_CPU_TIMER_H
#define MIMICRY_EXPERIMENTS_CPU_TIMER_H

#pragma once
#include <chrono>

class CpuTimer {
public:
    void start() {
        m_start = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        m_end = std::chrono::high_resolution_clock::now();
    }

    float getMilliseconds() const {
        return std::chrono::duration<float, std::milli>(m_end - m_start).count();
    }

private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_end;
};

#endif //MIMICRY_EXPERIMENTS_CPU_TIMER_H