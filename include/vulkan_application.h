#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include <QtGui/QGuiApplication>
#include <QtGui/QVulkanFunctions>
#include <QtGui/QVulkanInstance>
#include <QtGui/QVulkanWindow>
#include <QtGui/QVulkanWindowRenderer>
#include <iostream>

#include "graphics.h"

static constexpr inline VkDeviceSize aligned(VkDeviceSize v,
                                             VkDeviceSize byte_align)
{
    return (v + byte_align - 1) & ~(byte_align - 1);
}

class VulkanRenderer : public QVulkanWindowRenderer
{
  public:
    VulkanRenderer(QVulkanWindow& window)
        : window_(window),
          device_functions_{nullptr},
          buffer_{nullptr},
          device_memory_{nullptr},
          uniform_buffer_info_{},
          descriptor_pool_{nullptr},
          descriptor_set_layout_{nullptr},
          descriptor_set_{},
          pipeline_cache_{nullptr},
          pipeline_layout_{nullptr},
          pipeline_{nullptr}
    {
    }

    void startNextFrame() override
    {
        const auto command_buffer = window_.currentCommandBuffer();
        const auto render_pass_info = GetRenderPassBeginInfo();

        // begin render pass
        device_functions_->vkCmdBeginRenderPass(
            command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        // set viewport and scissor
        const auto [viewport, scissor] = GetViewportAndScissor();
        device_functions_->vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        device_functions_->vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        // @todo Draw everything here

        // end render pass
        device_functions_->vkCmdEndRenderPass(command_buffer);
        window_.frameReady();
        window_.requestUpdate();
    }

    void initResources() override
    {
        // get device functions
        const auto& device = window_.device();
        const auto& vulkan_instance = window_.vulkanInstance();
        device_functions_ = vulkan_instance->deviceFunctions(device);

        CreateMemoryBuffer(device);
        const auto vertex_input_info = CreateVertexInputs(device);
        CreateGraphicsPipeline(device, vertex_input_info);
    }

    void initSwapChainResources() override
    {
        // @todo Init projection matrix etc. depending on window size here
    }

  private:
    VkRenderPassBeginInfo GetRenderPassBeginInfo();
    std::pair<VkViewport, VkRect2D> GetViewportAndScissor();
    void CreateMemoryBuffer(const VkDevice& device);
    VkPipelineVertexInputStateCreateInfo CreateVertexInputs(
        const VkDevice& device);
    void CreateGraphicsPipeline(
        const VkDevice& device,
        const VkPipelineVertexInputStateCreateInfo vertex_input_info);
    VkShaderModule CreateShader(const QString& name);

    QVulkanWindow& window_;
    QVulkanDeviceFunctions* device_functions_;

    const VkClearValue clear_values_{{{0.0F, 0.0F, 0.0F, 1.0F}}};
    VkBuffer buffer_;
    VkDeviceMemory device_memory_;

    VkDescriptorBufferInfo
        uniform_buffer_info_[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    VkDescriptorPool descriptor_pool_;
    VkDescriptorSetLayout descriptor_set_layout_;
    VkDescriptorSet descriptor_set_[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache pipeline_cache_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;
};

class VulkanWindow : public QVulkanWindow
{
  public:
    QVulkanWindowRenderer* createRenderer() override
    {
        return new VulkanRenderer(*this);
    }

  protected:
    void keyPressEvent(QKeyEvent*) override { close(); }
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
