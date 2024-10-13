//
// Created by HUSTLX on 2024/10/12.
//

#include "FPSCalculator.h"
#include <iostream>


namespace HWPT {

    FPSCalculator::FPSCalculator(float RecordInterval): m_recordInterval(RecordInterval) {}

    auto FPSCalculator::Tick() -> bool {
        auto StartTime = std::chrono::high_resolution_clock::now();
        m_deltaTime = std::chrono::duration<double>(StartTime - m_currentTime).count();
        m_elapsedTime += m_deltaTime;
        m_currentTime = StartTime;
        m_frameCount++;

        if (m_elapsedTime >= m_recordInterval) {
            m_fps = static_cast<uint>(m_frameCount / m_elapsedTime);
            m_elapsedTime = 0.f;
            m_frameCount = 0;
            return true;
        }

        return false;
    }


}
