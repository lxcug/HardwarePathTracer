#include <iostream>
#include "Core.h"
#include "application/VulkanBackendApp.h"
#include "application/VulkanRayTracingApp.h"


auto main(int argc, char* argv[]) -> int {
    auto* App = new Shadowy::VulkanRayTracingApp();
    App->Init();
    App->Run();

    delete App;
    return 0;
}
