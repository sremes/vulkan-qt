#include "vulkan_application.h"

#include <QtCore/QFile>
#include <cstring>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

// clang-format off
// vertex data as (x, y, r, g, b) tuples
constexpr float vertex_data[] = { // Y down, front = CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f
};
// clang-format on

constexpr std::size_t vertex_data_size{15 * sizeof(float)};
constexpr std::size_t uniform_data_size{16 * sizeof(float)};

VkRenderPassBeginInfo VulkanRenderer::GetRenderPassBeginInfo()
{
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = window_.defaultRenderPass();
    render_pass_info.framebuffer = window_.currentFramebuffer();

    const QSize size = window_.swapChainImageSize();
    render_pass_info.renderArea.extent.width = size.width();
    render_pass_info.renderArea.extent.height = size.height();

    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_values_;

    return render_pass_info;
}

std::pair<VkViewport, VkRect2D> VulkanRenderer::GetViewportAndScissor()
{
    VkViewport viewport{};
    const QSize size = window_.swapChainImageSize();
    viewport.x = viewport.y = 0;
    viewport.width = size.width();
    viewport.height = size.height();
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    VkRect2D scissor{};
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;

    return std::make_pair(viewport, scissor);
}

void VulkanRenderer::CreateMemoryBuffer(const VkDevice &device)
{
    // init buffer
    const int concurrent_frames = window_.concurrentFrameCount();
    qDebug("concurrent frames: %d", concurrent_frames);
    const VkPhysicalDeviceLimits device_limits =
        window_.physicalDeviceProperties()->limits;
    const VkDeviceSize uniform_align =
        device_limits.minUniformBufferOffsetAlignment;
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    const VkDeviceSize vertex_alloc_size =
        aligned(vertex_data_size, uniform_align);
    const VkDeviceSize uniform_alloc_size =
        aligned(uniform_data_size, uniform_align);
    buffer_info.size =
        vertex_alloc_size + concurrent_frames * uniform_alloc_size;
    qDebug("using a buffer of size: %lu bytes", buffer_info.size);
    buffer_info.usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    auto err = device_functions_->vkCreateBuffer(device, &buffer_info, nullptr,
                                                 &buffer_);
    if (err != VK_SUCCESS) qFatal("Failed to create buffer: %d", err);

    // allocate the memory
    VkMemoryRequirements memory_requirements{};
    device_functions_->vkGetBufferMemoryRequirements(device, buffer_,
                                                     &memory_requirements);

    VkMemoryAllocateInfo allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
        memory_requirements.size, window_.hostVisibleMemoryIndex()};

    err = device_functions_->vkAllocateMemory(device, &allocate_info, nullptr,
                                              &device_memory_);
    if (err != VK_SUCCESS) qFatal("Failed to allocate memory: %d", err);

    err = device_functions_->vkBindBufferMemory(device, buffer_, device_memory_,
                                                0);
    if (err != VK_SUCCESS) qFatal("Failed to bind buffer memory: %d", err);

    // initialize the memory by mapping it
    quint8 *p{};
    err = device_functions_->vkMapMemory(device, device_memory_, 0,
                                         memory_requirements.size, 0,
                                         reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS) qFatal("Failed to map memory: %d", err);

    // copy vertex data
    std::memcpy(p, vertex_data, vertex_data_size);

    // copy uniform for each frame
    constexpr auto identity = glm::identity<glm::mat4>();
    for (int i = 0; i < concurrent_frames; ++i)
    {
        const VkDeviceSize offset = vertex_alloc_size + i * uniform_alloc_size;
        std::memcpy(p + offset, glm::value_ptr(identity), 16 * sizeof(float));
        uniform_buffer_info_[i].buffer = buffer_;
        uniform_buffer_info_[i].offset = offset;
        uniform_buffer_info_[i].range = uniform_alloc_size;
    }
    device_functions_->vkUnmapMemory(device, device_memory_);
}

VkPipelineVertexInputStateCreateInfo VulkanRenderer::CreateVertexInputs(
    const VkDevice &device)
{
    // clang-format off
    constexpr VkVertexInputBindingDescription binding_description = {
        0,  // binding
        5 * sizeof(float), 
        VK_VERTEX_INPUT_RATE_VERTEX
    };
    constexpr VkVertexInputAttributeDescription attribute_description[] = {
        { // position
            0, // location
            0, // binding
            VK_FORMAT_R32G32_SFLOAT,
            0
        },
        { // color
            1,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            2 * sizeof(float)
        }
    };
    // clang-format on

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = 2;
    vertex_input_info.pVertexAttributeDescriptions = attribute_description;

    // Set up descriptor set and its layout.
    const int concurrent_frames = window_.concurrentFrameCount();
    const VkDescriptorPoolSize pool_sizes = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        static_cast<std::uint32_t>(concurrent_frames)};
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = concurrent_frames;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_sizes;
    auto err = device_functions_->vkCreateDescriptorPool(
        device, &pool_info, nullptr, &descriptor_pool_);
    if (err != VK_SUCCESS) qFatal("Failed to create descriptor pool: %d", err);

    VkDescriptorSetLayoutBinding layout_binding = {
        0,  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,
        nullptr};
    VkDescriptorSetLayoutCreateInfo layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1,
        &layout_binding};
    err = device_functions_->vkCreateDescriptorSetLayout(
        device, &layout_info, nullptr, &descriptor_set_layout_);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", err);

    for (int i = 0; i < concurrent_frames; ++i)
    {
        const VkDescriptorSetAllocateInfo allocate_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr,
            descriptor_pool_, 1, &descriptor_set_layout_};
        err = device_functions_->vkAllocateDescriptorSets(
            device, &allocate_info, &descriptor_set_[i]);
        if (err != VK_SUCCESS)
            qFatal("Failed to allocate descriptor set: %d", err);

        VkWriteDescriptorSet write_descriptor{};
        write_descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor.dstSet = descriptor_set_[i];
        write_descriptor.descriptorCount = 1;
        write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor.pBufferInfo = &uniform_buffer_info_[i];
        device_functions_->vkUpdateDescriptorSets(device, 1, &write_descriptor,
                                                  0, nullptr);
    }
    return vertex_input_info;
}

void VulkanRenderer::CreateGraphicsPipeline(
    const VkDevice &device,
    const VkPipelineVertexInputStateCreateInfo vertex_input_info)
{
    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo{};
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    auto err = device_functions_->vkCreatePipelineCache(
        device, &pipelineCacheInfo, nullptr, &pipeline_cache_);
    if (err != VK_SUCCESS) qFatal("Failed to create pipeline cache: %d", err);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout_;
    err = device_functions_->vkCreatePipelineLayout(device, &pipelineLayoutInfo,
                                                    nullptr, &pipeline_layout_);
    if (err != VK_SUCCESS) qFatal("Failed to create pipeline layout: %d", err);

    // Shaders
    VkShaderModule vertShaderModule =
        CreateShader(QStringLiteral("shaders/vertex.glsl"));
    VkShaderModule fragShaderModule =
        CreateShader(QStringLiteral("shaders/fragment.glsl"));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main", nullptr}};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertex_input_info;

    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.pInputAssemblyState = &ia;

    // The viewport and scissor will be set dynamically via
    // vkCmdSetViewport/Scissor. This way the pipeline does not need to be
    // touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;  // we want the back face as well
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampling.
    ms.rasterizationSamples = window_.sampleCountFlagBits();
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &ds;

    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // no blend, write out all of rgba
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    pipelineInfo.pColorBlendState = &cb;

    const VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                             VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount =
        sizeof(dynamic_states) / sizeof(VkDynamicState);
    dynamic_state_info.pDynamicStates = dynamic_states;
    pipelineInfo.pDynamicState = &dynamic_state_info;

    pipelineInfo.layout = pipeline_layout_;
    pipelineInfo.renderPass = window_.defaultRenderPass();

    err = device_functions_->vkCreateGraphicsPipelines(
        device, pipeline_cache_, 1, &pipelineInfo, nullptr, &pipeline_);
    if (err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);

    if (vertShaderModule)
        device_functions_->vkDestroyShaderModule(device, vertShaderModule,
                                                 nullptr);
    if (fragShaderModule)
        device_functions_->vkDestroyShaderModule(device, fragShaderModule,
                                                 nullptr);
}

VkShaderModule VulkanRenderer::CreateShader(const QString &name)
{
    QFile file(name);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning("Failed to read shader %s", qPrintable(name));
        return VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t *>(blob.constData());
    VkShaderModule shaderModule;
    VkResult err = device_functions_->vkCreateShaderModule(
        window_.device(), &shaderInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS)
    {
        qWarning("Failed to create shader module: %d", err);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}