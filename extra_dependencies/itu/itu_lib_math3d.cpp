#include <itu_engine.hpp>


void itu_lib_math_decompose_transform(glm::mat4 transform, glm::vec3* out_pos, glm::vec3* out_rot, glm::vec3* out_scale)
{
	*out_pos = transform[3];
	*out_scale = glm::vec3{
		glm::length(glm::vec3(transform[0])),
		glm::length(glm::vec3(transform[1])),
		glm::length(glm::vec3(transform[2]))
	};

	// NOTE euler-engle extraction procedure is not sensible to translation, but might be scaling, need to double check
	glm::mat4 pure_rot = transform;
	pure_rot[0][0] /= (*out_scale)[0];
	pure_rot[1][1] /= (*out_scale)[1];
	pure_rot[2][2] /= (*out_scale)[2];
	*out_rot = itu_lib_math_to_euler_safe(pure_rot);
}

void itu_lib_math_assemble_transform(glm::mat4* out_transform, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
{
	glm::quat q = itu_lib_math_to_quaternion_safe(rot);
	glm::mat4 rot_matrix = glm::mat4_cast(glm::normalize(q));
	glm::mat4 translate_matrix = glm::translate(pos);
	glm::mat4 scale_matrix = glm::scale(scale);

	*out_transform = translate_matrix * rot_matrix * scale_matrix;
}

// TODO FIXME problem with this procedure, probably due to converting it from a row-major to a column-major system
//(from https://github.com/godotengine/godot/blob/0fdbf050e0c7fc7e0a9d42c2a41ee3bfdffbd8f1/core/math/basis.cpp#L455)
glm::vec3 itu_lib_math_to_euler_safe(glm::mat4 m)
{
	glm::vec3 euler;
	const double epsilon = 0.00000025;

	float m12 = m[2][1];

	if (m12 < (1 - epsilon)) {
		if (m12 > -(1 - epsilon)) {
			// is this a pure X rotation?
			if (
				check_equality(m[0][1], 0.0f) &&
				check_equality(m[1][0], 0.0f) &&
				check_equality(m[2][0], 0.0f) &&
				check_equality(m[0][2], 0.0f) &&
				check_equality(m[0][0], 1.0f)
			) {
				// return the simplest form (human friendlier in editor and scripts)
				euler.x = SDL_atan2(-m12, m[1][1]);
				euler.y = 0;
				euler.z = 0;
			} else {
				euler.x = SDL_asin(-m12);
				euler.y = SDL_atan2(m[2][0], m[2][2]);
				euler.z = SDL_atan2(m[0][1], m[1][1]);
			}
		} else { // m12 == -1
			euler.x = PI_HALF;
			euler.y = SDL_atan2(m[1][0], m[0][0]);
			euler.z = 0;
		}
	} else { // m12 == 1
		euler.x = -PI_HALF;
		euler.y = -SDL_atan2(m[1][0], m[0][0]);
		euler.z = 0;
	}

	return euler;
}

glm::vec3 itu_lib_math_to_euler_safe(glm::quat q) {
	float test = q.x * q.y + q.z * q.w;
	if (test > 0.499) { // singularity at north pole
		return {
			0,
			2 * atan2(q.x, q.w),
			glm::pi<float>() / 2
		};
	}
	if (test < -0.499) { // singularity at south pole
		return {
			0,
			-2 * glm::atan2(q.x, q.w),
			-glm::pi<float>() / 2
		};
	}
	double sqx = q.x * q.x;
	double sqy = q.y * q.y;
	double sqz = q.z * q.z;
	
	glm::vec3 ret = {
		glm::atan2(2.0f * q.x * q.w - 2.0f * q.y * q.z, (float)(1.0f - 2.0f * sqx - 2.0f * sqz)),
		glm::atan2(2.0f * q.y * q.w - 2.0f * q.x * q.z, (float)(1.0f - 2.0f * sqy - 2.0f * sqz)),
		glm::asin(2.0f * test)
	};

	return ret;
}

glm::quat itu_lib_math_to_quaternion_safe(glm::vec3 eulerAngles) {
	glm::vec3 sorted;
	sorted.x = eulerAngles.x;
	sorted.y = eulerAngles.y;
	sorted.z = eulerAngles.z;
	
	float c1 = glm::cos(sorted.x / 2);
	float s1 = glm::sin(sorted.x / 2);
	float c2 = glm::cos(sorted.y / 2);
	float s2 = glm::sin(sorted.y / 2);
	float c3 = glm::cos(sorted.z / 2);
	float s3 = glm::sin(sorted.z / 2);

	return {
		-s1*s2*s3 + c1*c2*c3,
		+s1*c2*c3 + s2*s3*c1,
		-s1*s3*c2 + s2*c1*c3,
		+s1*s2*c3 + s3*c1*c2
	};

	/*
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;
	loat c1 = glm::cos(sorted.y / 2);
	float s1 = glm::sin(sorted.y / 2);
	float c2 = glm::cos(sorted.x / 2);
	float s2 = glm::sin(sorted.x / 2);
	float c3 = glm::cos(sorted.z / 2);
	float s3 = glm::sin(sorted.z / 2);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;
	return {
		s1*s2*s3 + c1*c2*c3,
		s1*s3*c2 + s2*c1*c3,
		s1*c2*c3 - s2*s3*c1,
		-s1*s2*c3 + s3*c1*c2
	};*/
}

void itu_lib_math_reset_rotation(glm::mat4 *matrix)
{
	glm::vec3 pos, rot, scale;
	itu_lib_math_decompose_transform(*matrix, &pos, &rot, &scale);
	rot.x = 0;
	rot.y = 0;
	rot.z = 0;
	itu_lib_math_assemble_transform(matrix, pos, rot, scale);
}
