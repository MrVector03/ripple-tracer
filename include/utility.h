//
// Created by viktor on 1/6/25.
//

#ifndef UTILITY_H
#define UTILITY_H

#include "glad/glad.h"

GLuint load_skybox(const char *faces[6]);

void setup_skybox(GLuint skybox_vao, GLuint skybox_vbo);

#endif //UTILITY_H
