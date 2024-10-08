//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_APPLICATION_H
#define HARDWAREPATHTRACER_APPLICATION_H

#include "core/Core.h"


namespace HWPT {
    class ApplicationBase {
    public:
        virtual void Run() = 0;

        virtual void DrawFrame() = 0;

        virtual ~ApplicationBase() = default;

    private:
        virtual void Init() = 0;
    };
}


#endif //HARDWAREPATHTRACER_APPLICATION_H
