#include <frame_buffers.h>
#include <main_state.h>
#include <glad/glad.h>
#include <math.h>


#include <rafgl.h>

#include <game_constants.h>

static float vertices[] = {
    -0.5f, 0.0f, -0.5f,  // Bottom-left
    0.5f, 0.0f, -0.5f,  // Bottom-right
    0.5f, 0.0f,  0.5f,  // Top-right

   -0.5f, 0.0f, -0.5f,  // Bottom-left
    0.5f, 0.0f,  0.5f,  // Top-right
   -0.5f, 0.0f,  0.5f   // Top-left
};

WaterFrameBuffers w_fbo;

static GLuint vao, vbo, shader_program_id, uni_M, uni_VP, uni_phase, uni_camera_pos, gui_texture;

// SKYBOX
static rafgl_texture_t skybox_texture;

static GLuint skybox_shader, skybox_shader_cell;
static GLuint skybox_uni_P, skybox_uni_V;
static GLuint skybox_cell_uni_P, skybox_cell_uni_V;

static rafgl_meshPUN_t skybox_mesh;

void main_state_init(GLFWwindow *window, void *args, int width, int height)
{
    // SHADER PROGRAM
    shader_program_id = rafgl_program_create_from_name("custom_water_shader_v1");

    uni_M = glGetUniformLocation(shader_program_id, "uni_M");
    uni_VP = glGetUniformLocation(shader_program_id, "uni_VP");
    uni_phase = glGetUniformLocation(shader_program_id, "uni_phase");
    uni_camera_pos = glGetUniformLocation(shader_program_id, "uni_camera_pos");

    // BUFFERS
    frame_buffers_core_init(w_fbo, width, height);

    // SKYBOX
    rafgl_texture_load_cubemap_named(&skybox_texture, "above_the_sea_2", "jpg");
    skybox_shader = rafgl_program_create_from_name("custom_skybox_shader");
    skybox_shader_cell = rafgl_program_create_from_name("custom_skybox_shader_cell");

    skybox_uni_P = glGetUniformLocation(skybox_shader, "uni_P");
    skybox_uni_V = glGetUniformLocation(skybox_shader, "uni_V");

    skybox_cell_uni_P = glGetUniformLocation(skybox_shader_cell, "uni_P");
    skybox_cell_uni_V = glGetUniformLocation(skybox_shader_cell, "uni_V");

    rafgl_meshPUN_init(&skybox_mesh);
    rafgl_meshPUN_load_cube(&skybox_mesh, 1.0f);

    // GUI // TODO: NO IDEA WHAT THIS DOES
    glGenTextures(1, &gui_texture);
    glBindTexture(GL_TEXTURE_2D, gui_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, w_fbo.reflection_texture);

    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // DEF VERTEX ATTRS
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
    glEnableVertexAttribArray(0);

    // VAO UNBIND
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

float fov = 75.0f;

vec3_t camera_position = {1.013418, 0.693488, 0.864704};
vec3_t camera_target = {0.0f, 0.0f, 0.0f};
vec3_t camera_up = {0.0f, 1.0f, 0.0f};
vec3_t aim_dir = {0.0f, 0.0f, 0.0f};

float camera_angle = -M_PIf / 2.0f;
float angle_speed = 0.2f * M_PIf;
float move_speed = 0.8f;

float hoffset = -0.35f * M_PIf;

float time = 0.0f;
int reshow_cursor = 0;
int last_lmb = 0;

mat4_t model, view, projection, view_projection;


void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args) {
    // rafgl_log_fps(RAFGL_TRUE);
    time += delta_time;

    bind_reflection_frame_buffer(w_fbo);

    // INPUT
    if (1) {
        vec3_t right_dir = v3_cross(camera_up, aim_dir); // Right direction
        if (game_data->keys_down['D'])
            camera_position = v3_sub(camera_position, v3_muls(right_dir, move_speed * delta_time));
        if (game_data->keys_down['A'])
            camera_position = v3_add(camera_position, v3_muls(right_dir, move_speed * delta_time));

        if (game_data->is_lmb_down) {
            if (reshow_cursor == 0)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            float dx = game_data->mouse_pos_x - game_data->raster_width / 2;
            float dy = game_data->mouse_pos_y - game_data->raster_height / 2;

            if (!last_lmb) {
                dx = 0;
                dy = 0;
            }

            hoffset -= dy / game_data->raster_height;
            camera_angle += dx / game_data->raster_width;

            glfwSetCursorPos(window, game_data->raster_width / 2, game_data->raster_height / 2);
            reshow_cursor = 1;
        } else if (reshow_cursor) {
            reshow_cursor = 0;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        last_lmb = game_data->is_lmb_down;

        aim_dir = vec3(cosf(camera_angle), 0.0f, sinf(camera_angle));

        if (game_data->keys_down['W']) camera_position = v3_add(camera_position, v3_muls(aim_dir, move_speed * delta_time));
        if (game_data->keys_down['S']) camera_position = v3_sub(camera_position, v3_muls(aim_dir, move_speed * delta_time));

        if (game_data->keys_down[RAFGL_KEY_SPACE]) camera_position.y += move_speed * delta_time;
        if (game_data->keys_down[RAFGL_KEY_LEFT_SHIFT]) camera_position.y -= move_speed * delta_time;
        // printf("Camera Position: (%f, %f, %f)\n", camera_position.x, camera_position.y, camera_position.z);

        float aspect = ((float)(game_data->raster_width)) / game_data->raster_height;
        projection = m4_perspective(fov, aspect, 0.1f, 100.0f);

        if(!game_data->keys_down['T'])
        {
            view = m4_look_at(camera_position, v3_add(camera_position, v3_add(aim_dir, vec3(0.0f, hoffset, 0.0f))), camera_up);
        }
        else
        {
            view = m4_look_at(camera_position, vec3(0.0f, 0.0f, 0.0f), camera_up);
        }
    }

    model = m4_identity();
    view_projection = m4_mul(projection, view);
}


void main_state_render(GLFWwindow *window, void *args)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // CLEAR
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);



    // SKYBOX
    glUseProgram(skybox_shader);
    glUniformMatrix4fv(skybox_uni_V, 1, GL_FALSE, (void*) view.m);
    glUniformMatrix4fv(skybox_uni_P, 1, GL_FALSE, (void*) projection.m);
    glBindVertexArray(skybox_mesh.vao_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture.tex_id);
    glDrawArrays(GL_TRIANGLES, 0, skybox_mesh.vertex_count);

    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture.tex_id);

    // SHADER
    glUseProgram(shader_program_id);

    // BIND VAD AND DRAW
    glBindVertexArray(vao);

    glUniformMatrix4fv(uni_M, 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(uni_VP, 1, GL_FALSE, (void*) view_projection.m);
    glUniform1f(uni_phase, time * 0.1f);
    glUniform3f(uni_camera_pos, camera_position.x, camera_position.y, camera_position.z);

    glDrawArrays(GL_TRIANGLES, 0, 6);



    glBindVertexArray(0);
}


void main_state_cleanup(GLFWwindow *window, void *args)
{
    water_buffer_clean_up(w_fbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader_program_id);
}

