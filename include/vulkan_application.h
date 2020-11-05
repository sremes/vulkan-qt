#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include <QtGui/QGuiApplication>
#include <QtGui/QVulkanFunctions>
#include <QtGui/QVulkanInstance>
#include <QtGui/QVulkanWindow>
#include <QtGui/QVulkanWindowRenderer>

class VulkanRenderer : public QVulkanWindowRenderer
{
  public:
    VulkanRenderer(QVulkanWindow& window)
        : window_(window), device_functions_{nullptr}
    {
    }

    void startNextFrame() override
    {
        auto command_buffer = window_.currentCommandBuffer();
        auto render_pass_info = GetRenderPassBeginInfo();
        device_functions_->vkCmdBeginRenderPass(
            command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        // @todo Draw everything here
        device_functions_->vkCmdEndRenderPass(command_buffer);
        window_.frameReady();
        window_.requestUpdate();
    }

    void initResources() override
    {
        const auto& device = window_.device();
        const auto& vulkan_instance = window_.vulkanInstance();
        device_functions_ = vulkan_instance->deviceFunctions(device);
    }

    void initSwapChainResources() override
    {
        // @todo Init projection matrix etc. depending on window size here
    }

  private:
    VkRenderPassBeginInfo GetRenderPassBeginInfo();

    QVulkanWindow& window_;
    QVulkanDeviceFunctions* device_functions_;

    const VkClearValue clear_values_{{0.0F, 0.0F, 0.0F, 1.0F}};
};

class VulkanWindow : public QVulkanWindow
{
  public:
    QVulkanWindowRenderer* createRenderer() override
    {
        return new VulkanRenderer(*this);
    }

  protected:
    void keyPressEvent(QKeyEvent* event) override { close(); }
};

class VulkanApplication : public QGuiApplication
{
  public:
    VulkanApplication(int& argc, char**& argv) : QGuiApplication(argc, argv)
    {
        // Enable validation layer, if supported. Messages go to qDebug by
        // default.
        vulkan_instance_.setLayers(QByteArrayList()
                                   << "VK_LAYER_LUNARG_standard_validation"
                                   << "VK_LAYER_GOOGLE_threading"
                                   << "VK_LAYER_LUNARG_parameter_validation"
                                   << "VK_LAYER_LUNARG_object_tracker"
                                   << "VK_LAYER_LUNARG_core_validation"
                                   << "VK_LAYER_LUNARG_image"
                                   << "VK_LAYER_LUNARG_swapchain"
                                   << "VK_LAYER_GOOGLE_unique_objects");
        if (!vulkan_instance_.create())
        {
            qFatal("Failed to create vulkan instance: %d",
                   vulkan_instance_.errorCode());
        }
        vulkan_window_.setVulkanInstance(&vulkan_instance_);
        vulkan_window_.showMaximized();
    }

  private:
    QVulkanInstance vulkan_instance_;
    VulkanWindow vulkan_window_;
};

#endif  // VULKAN_APPLICATION_H
