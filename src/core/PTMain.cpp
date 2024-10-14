#include <iostream>
#include "core/application/VulkanBackendApp.h"

auto main() -> int {
    auto* App = new HWPT::VulkanBackendApp();
    App->Init();
    App->Run();

    delete App;
    return 0;
}
