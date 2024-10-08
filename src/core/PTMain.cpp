#include <iostream>
#include "core/application/VulkanBackendApp.h"

int main() {
    HWPT::VulkanBackendApp App;
    App.Init();

    App.Run();

    return 0;
}
