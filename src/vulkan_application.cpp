#include "vulkan_application.h"

VkRenderPassBeginInfo VulkanRenderer::GetRenderPassBeginInfo()
{
    VkRenderPassBeginInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = window_.defaultRenderPass();
    render_pass_info.framebuffer = window_.currentFramebuffer();
    
    const QSize size = window_.swapChainImageSize();
    render_pass_info.renderArea.extent.width = size.width();
    render_pass_info.renderArea.extent.height = size.height();

    const VkClearValue clear_values = {0.0F, 0.0F, 0.1F, 1.0F};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_values;

    return render_pass_info;
}