#ifndef VULKAN_QT_INCLUDE_GRAPHICS_H
#define VULKAN_QT_INCLUDE_GRAPHICS_H

#include <glm/mat4x4.hpp>

glm::mat4 GetMVPMatrix(const float aspect, const float translate_z,
                       const float rotate_x, const float rotate_y);

#endif  // VULKAN_QT_INCLUDE_GRAPHICS_H