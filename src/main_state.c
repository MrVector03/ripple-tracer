#include <main_state.h>
#include <glad/glad.h>
#include <math.h>
#include <rafgl.h>
#include <game_constants.h>
#include <utility.h>

static float vertices[] = {
    // Positions          // Texture Coords
    -0.5f, 0.0f, -0.5f,   0.0f, 0.0f,  // Bottom-left
     0.5f, 0.0f, -0.5f,   1.0f, 0.0f,  // Bottom-right
     0.5f, 0.0f,  0.5f,   1.0f, 1.0f,  // Top-right

    -0.5f, 0.0f, -0.5f,   0.0f, 0.0f,  // Bottom-left
     0.5f, 0.0f,  0.5f,   1.0f, 1.0f,  // Top-right
    -0.5f, 0.0f,  0.5f,   0.0f, 1.0f   // Top-left
};

static GLuint vao, vbo, shader_program_id, uni_M, uni_VP, uni_phase, uni_camera_pos, uni_time;
static GLuint location_plane;

// SKYBOX
static rafgl_texture_t skybox_texture;

static GLuint skybox_shader, skybox_shader_cell;
static GLuint skybox_uni_P, skybox_uni_V;
static GLuint skybox_cell_uni_P, skybox_cell_uni_V;

static rafgl_meshPUN_t skybox_mesh;

static Vector4f plane = {0.0f, -1.0f, 0.0f, 15.0f};

int num_meshes;

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

int selected_mesh = 0;

GLuint reflectionFBO, refractionFBO;
GLuint reflectionTexture, refractionTexture;
GLuint reflectionDepthBuffer, refractionDepthBuffer;

void createFramebuffers(int width, int height) {
    glGenFramebuffers(1, &reflectionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);

    glGenTextures(1, &reflectionTexture);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTexture, 0);

    glGenRenderbuffers(1, &reflectionDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, reflectionDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, reflectionDepthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Reflection Framebuffer not complete!\n");

    glGenFramebuffers(1, &refractionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, refractionFBO);

    glGenTextures(1, &refractionTexture);
    glBindTexture(GL_TEXTURE_2D, refractionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractionTexture, 0);

    glGenRenderbuffers(1, &refractionDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, refractionDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, refractionDepthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Refraction Framebuffer not complete!\n");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderToReflectionFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4_t reflectionView = m4_look_at(
        vec3(camera_position.x, -camera_position.y, camera_position.z),
        vec3(camera_target.x, -camera_target.y, camera_target.z),
        vec3(camera_up.x, -camera_up.y, camera_up.z)
    );

    mat4_t reflectionViewProjection = m4_mul(projection, reflectionView);

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skybox_shader);
    glUniformMatrix4fv(skybox_uni_V, 1, GL_FALSE, (void*) reflectionView.m);
    glUniformMatrix4fv(skybox_uni_P, 1, GL_FALSE, (void*) projection.m);
    glBindVertexArray(skybox_mesh.vao_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture.tex_id);
    glDrawArrays(GL_TRIANGLES, 0, skybox_mesh.vertex_count);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

    glUseProgram(shader_program_id);
    glUniformMatrix4fv(uni_VP, 1, GL_FALSE, (void*) reflectionViewProjection.m);
    glUniform1f(uni_time, time);
    glUniform3f(uni_camera_pos, camera_position.x, camera_position.y, camera_position.z);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderToRefractionFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, refractionFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4_t refractionView = view;
    mat4_t refractionViewProjection = m4_mul(projection, refractionView);

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skybox_shader);
    glUniformMatrix4fv(skybox_uni_V, 1, GL_FALSE, (void*) refractionView.m);
    glUniformMatrix4fv(skybox_uni_P, 1, GL_FALSE, (void*) projection.m);
    glBindVertexArray(skybox_mesh.vao_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture.tex_id);
    glDrawArrays(GL_TRIANGLES, 0, skybox_mesh.vertex_count);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

    glUseProgram(shader_program_id);
    glUniformMatrix4fv(uni_VP, 1, GL_FALSE, (void*) refractionViewProjection.m);
    glUniform1f(uni_time, time);
    glUniform3f(uni_camera_pos, camera_position.x, camera_position.y, camera_position.z);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void main_state_init(GLFWwindow *window, void *args, int width, int height)
{

    // SHADER PROGRAM
    shader_program_id = rafgl_program_create_from_name("custom_water_shader_v1");


    uni_M = glGetUniformLocation(shader_program_id, "uni_M");
    uni_VP = glGetUniformLocation(shader_program_id, "uni_VP");
    uni_phase = glGetUniformLocation(shader_program_id, "uni_phase");
    uni_camera_pos = glGetUniformLocation(shader_program_id, "uni_camera_pos");
    location_plane = glGetUniformLocation(shader_program_id, "plane");
    uni_time = glGetUniformLocation(shader_program_id, "uni_time");

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

    createFramebuffers(width, height);


    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // VAO unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args) {
    // rafgl_log_fps(RAFGL_TRUE);
    time += delta_time;

    glEnable(GL_CLIP_DISTANCE0);

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

        if (game_data->keys_down[RAFGL_KEY_LEFT_SHIFT]) camera_position.y += move_speed * delta_time;
        if (game_data->keys_down[RAFGL_KEY_LEFT_CONTROL]) camera_position.y -= move_speed * delta_time;
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

        if(game_data->keys_pressed[RAFGL_KEY_KP_ADD]) selected_mesh = (selected_mesh + 1) % num_meshes;
        if(game_data->keys_pressed[RAFGL_KEY_KP_SUBTRACT]) selected_mesh = (selected_mesh + num_meshes - 1) % num_meshes;
    }

    model = m4_identity();
    view_projection = m4_mul(projection, view);
}


void main_state_render(GLFWwindow *window, void *args)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Render to reflection and refraction framebuffers
    renderToReflectionFramebuffer();
    renderToRefractionFramebuffer();

    // CLEAR
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    float aspect = (float)width / (float)height;
    projection = m4_perspective(fov, aspect, 0.1f, 1000.0f);

    // SKYBOX RENDER
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skybox_shader);
    glUniformMatrix4fv(skybox_uni_V, 1, GL_FALSE, (void*) view.m);
    glUniformMatrix4fv(skybox_uni_P, 1, GL_FALSE, (void*) projection.m);
    glBindVertexArray(skybox_mesh.vao_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture.tex_id);
    glDrawArrays(GL_TRIANGLES, 0, skybox_mesh.vertex_count);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // Reset depth function

    // WATER RENDER
    glUseProgram(shader_program_id);

    glUniformMatrix4fv(uni_M, 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(uni_VP, 1, GL_FALSE, (void*) view_projection.m);
    glUniform1f(uni_time, time);
    glUniform3f(uni_camera_pos, camera_position.x, camera_position.y, camera_position.z);

    // Bind reflection texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);
    glUniform1i(glGetUniformLocation(shader_program_id, "reflectionTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, refractionTexture);
    glUniform1i(glGetUniformLocation(shader_program_id, "refractionTexture"), 1);


    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDisable(GL_DEPTH_TEST);
}

void main_state_cleanup(GLFWwindow *window, void *args)
{
    glDeleteFramebuffers(1, &reflectionFBO);
    glDeleteFramebuffers(1, &refractionFBO);
    glDeleteTextures(1, &reflectionTexture);
    glDeleteTextures(1, &refractionTexture);
    glDeleteRenderbuffers(1, &reflectionDepthBuffer);
    glDeleteRenderbuffers(1, &refractionDepthBuffer);
}


