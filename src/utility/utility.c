#include <rafgl.h>
#include <utility.h>

// SKYBOX VERTICES

void load_vector(GLuint location, Vector4f vector) {
	glUniform4f(location, vector.x, vector.y, vector.z, vector.w);;
}