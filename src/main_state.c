#include <main_state.h>
#include <glad/glad.h>
#include <math.h>


#include <rafgl.h>

#include <game_constants.h>

static float vertices[] =
{
    -1, -1, 0,
     1, -1, 0,
     0,  1, 0,
};

static GLuint vao, vbo, shader_program_id;

void main_state_init(GLFWwindow *window, void *args, int width, int height)
{
    shader_program_id = rafgl_program_create_from_name("first_shader");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(float), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

}

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args)
{

}


void main_state_render(GLFWwindow *window, void *args)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program_id);

    glEnableVertexAttribArray(0);
    glBindVertexArray(vao);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glUseProgram(0);
    //    glDisableVertexAttribArray(0);
}


void main_state_cleanup(GLFWwindow *window, void *args)
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

}

