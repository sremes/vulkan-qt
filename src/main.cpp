#include "vulkan_application.h"

int main(int argc, char* argv[])
{
    VulkanApplication app(argc, argv);
    return app.exec();
}