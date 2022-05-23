#include "graphics.h"

#include <glm/ext/matrix_clip_space.hpp>  // glm::perspective
#include <glm/ext/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale
#include <glm/ext/scalar_constants.hpp>  // glm::pi
#include <glm/vec3.hpp>                  // glm::vec3
#include <glm/vec4.hpp>                  // glm::vec4

glm::mat4 GetMVPMatrix(const float aspect, const float translate_z,
                       const float rotate_x, const float rotate_y)
{
    const glm::mat4 projection =
        glm::perspective(glm::pi<float>() * 0.25f, aspect, 0.1f, 100.f);
    glm::mat4 view =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -translate_z));
    view = glm::rotate(view, rotate_x, glm::vec3(-1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, rotate_y, glm::vec3(0.0f, 1.0f, 0.0f));
    constexpr glm::mat4 model = glm::identity<glm::mat4>();
    return projection * view * model;
}