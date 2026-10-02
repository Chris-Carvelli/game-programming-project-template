#ifndef ITU_LIB_TRANSFORM_2D_HPP
#define ITU_LIB_TRANSFORM_2D_HPP

#ifndef ITU_UNITY_BUILD
#include <itu_common.hpp>
#endif // ITU_UNITY_INCLUDE

struct Transform2D
{
	vec2f position;
	vec2f scale;
	float rotation;
};

const Transform2D TRANSFORM_DEFAULT = Transform2D { { 0, 0 }, { 1, 1 }, 0 };


#endif // ITU_LIB_TRANSFORM_2D_HPP
