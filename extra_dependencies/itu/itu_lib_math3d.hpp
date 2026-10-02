#ifndef ITU_LIB_MATH_3D_HPP
#define ITU_LIB_MATH_3D_HPP

#ifndef ITU_UNITY_BUILD

#define GLM_ENABLE_EXPERIMENTAL

#include <itu_common.hpp>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#endif

void      itu_lib_math_decompose_transform(glm::mat4 transform, glm::vec3* out_pos, glm::vec3* out_rot, glm::vec3* out_scale);
void      itu_lib_math_assemble_transform(glm::mat4* out_transform, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
glm::vec3 itu_lib_math_to_euler_safe(glm::mat4 m);
glm::vec3 itu_lib_math_to_euler_safe(glm::quat q);
glm::quat itu_lib_math_to_quaternion_safe(glm::vec3 eulerAngles);
void      itu_lib_math_reset_rotation(glm::mat4 *matrix);

#endif // ITU_LIB_MATH_3D_HPP
