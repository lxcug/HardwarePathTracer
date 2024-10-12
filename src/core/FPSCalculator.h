//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_FPSCALCULATOR_H
#define HARDWAREPATHTRACER_FPSCALCULATOR_H

#include "core/Core.h"
#include <chrono>


namespace HWPT {
    class FPSCalculator {
    public:
        FPSCalculator(float RecordInterval = 1.f);  // NOLINT

        bool Tick();

        [[nodiscard]] uint GetFPS() const {
            return m_fps;
        }

        [[nodiscard]] double GetDeltaTime() const {
            return m_deltaTime;
        }

    private:
        float m_recordInterval = 1.f;  // seconds
        std::chrono::high_resolution_clock::time_point m_currentTime =
                std::chrono::high_resolution_clock::now();
        uint m_frameCount = 0;
        double m_elapsedTime = 0.;
        double m_deltaTime = 0.;
        uint m_fps = 0;
    };
}

#endif //HARDWAREPATHTRACER_FPSCALCULATOR_H
