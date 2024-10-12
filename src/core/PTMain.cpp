#include <iostream>
#include "core/application/VulkanBackendApp.h"

auto main() -> int {
    HWPT::VulkanBackendApp App;
    App.Init();

    App.Run();

    return 0;
}
