#include <iostream>
#include "core/application/VulkanBackendApp.h"
#include "application/VulkanRayTracingApp.h"


auto main(int argc, char* argv[]) -> int {
    auto* App = new HWPT::VulkanRayTracingApp();
    App->Init();
    App->Run();

    delete App;
    return 0;
}
