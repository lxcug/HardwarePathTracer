#include <iostream>
#include "core/application/VulkanBackendApp.h"
#include "core/renderGraph/ShaderParameters.h"

auto main() -> int {
    using namespace HWPT;
    auto* App = new VulkanBackendApp();
    App->Init();
    App->Run();
    delete App;

    return 0;
}
