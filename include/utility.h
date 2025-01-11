//
// Created by viktor on 1/6/25.
//

#ifndef UTILITY_H
#define UTILITY_H

#include "glad/glad.h"

typedef struct {
	float x;
	float y;
	float z;
	float w;
} Vector4f;

void load_vector(GLuint location, Vector4f vector);

#endif //UTILITY_H
