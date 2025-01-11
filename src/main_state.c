#include <main_state.h>
#include <glad/glad.h>
#include <math.h>
#include <rafgl.h>
#include <game_constants.h>
#include <utility.h>
#include <time.h>

typedef struct _vertex_t
{                       /* offsets      */
    vec3_t position;    /* 0            */
    vec3_t colour;      /* 3 * float    */
    float alpha;        /* 6 * float    */
    float u, v;         /* 7 * float    */
    vec3_t normal;       /* 9 * float    */
} vertex_t;

vertex_t vertex(vec3_t pos, vec3_t col, float alpha, float u, float v, vec3_t normal)
{
    vertex_t vert;

    vert.position = pos;
    vert.colour = col;
    vert.alpha = alpha;
    vert.u = u;
    vert.v = v;
    vert.normal = normal;

    return vert;
}

vertex_t vertices[6];

static GLuint vao, vbo, shader_program_id, uni_M, uni_VP, uni_phase, uni_camera_pos, uni_time;
static GLuint location_plane;

// SKYBOX
static rafgl_texture_t skybox_texture;

static rafgl_raster_t water_normal_raster;
static rafgl_texture_t water_normal_map_tex;

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

float time_tick = 0.0f;
int reshow_cursor = 0;
int last_lmb = 0;

mat4_t model, view, projection, view_projection;

int selected_mesh = 0;

static const int REFLECTION_WIDTH = 320;
static const int REFLECTION_HEIGHT = 180;

static const int REFRACTION_WIDTH = 1280;
static const int REFRACTION_HEIGHT = 720;

static GLuint reflectionFrameBuffer;
static GLuint reflectionTexture;
static GLuint reflectionDepthBuffer;

static GLuint refractionFrameBuffer;
static GLuint refractionTexture;
static GLuint refractionDepthTexture;

GLuint hill_shader_program_id;
GLuint hill_vao, hill_vbo, hill_ebo;
int hill_vertex_count, hill_index_count;


GLuint create_framebuffer()
{
    GLuint framebuffer;

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    return framebuffer;
}

GLuint create_texture_attachment(int width, int height) {
    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    return texture;
}

GLuint create_depth_buffer_attachment(int width, int height) {
    GLuint depthBuffer;

    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    return depthBuffer;
}

GLuint create_depth_texture_attachment(int width, int height) {
    GLuint depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    return depthTexture;
}

void bindFrameBuffer(GLuint framebuffer, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
}

void unbindCurrentFrameBuffer(int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
}

void bindReflectionFrameBuffer()
{
    bindFrameBuffer(reflectionFrameBuffer, REFLECTION_WIDTH, REFLECTION_HEIGHT);
}

void bindRefractionFrameBuffer()
{
    bindFrameBuffer(refractionFrameBuffer, REFRACTION_WIDTH, REFRACTION_HEIGHT);
}

void initialize_reflection_refraction_framebuffers(int width, int height)
{
    // Reflection init
    reflectionFrameBuffer = create_framebuffer();
    reflectionTexture = create_texture_attachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
    reflectionDepthBuffer = create_depth_buffer_attachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
    unbindCurrentFrameBuffer(width, height);



    // Refraction init
    refractionFrameBuffer = create_framebuffer();
    refractionTexture = create_texture_attachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
    refractionDepthTexture = create_depth_texture_attachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
    unbindCurrentFrameBuffer(width, height);
}

void frame_buffer_cleanup() {
    glDeleteFramebuffers(1, &reflectionFrameBuffer);
    glDeleteTextures(1, &reflectionTexture);
    glDeleteRenderbuffers(1, &reflectionDepthBuffer);
    glDeleteFramebuffers(1, &refractionFrameBuffer);
    glDeleteTextures(1, &refractionTexture);
    glDeleteTextures(1, &refractionDepthTexture);
}

vertex_t* generate_hills(int width, int height, float scale, int* vertex_count) {
    int num_vertices = width * height;
    vertex_t* vertices = (vertex_t*)malloc(num_vertices * sizeof(vertex_t));
    *vertex_count = num_vertices;

    float x_offset = width / 2.0f;
    float z_offset = height / 2.0f;

    srand(time(NULL));

    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            float random_height = ((float)rand() / RAND_MAX) * scale; // Random height between 0 and scale
            float y = sinf((x - x_offset) * 0.1f) * cosf((z - z_offset) * 0.1f) * random_height;
            vertices[z * width + x] = vertex(vec3(x - x_offset, y, z - z_offset), vec3(0.0f, 1.0f, 0.0f), 1.0f, (float)x / width, (float)z / height, vec3(0.0f, 1.0f, 0.0f));
        }
    }

    return vertices;
}

GLuint* generate_hill_indices(int width, int height, int *index_count)
{
    int num_indices = (width - 1) * (height - 1) * 6;
    GLuint *indices = malloc(num_indices * sizeof(GLuint));
    *index_count = num_indices;

    int index = 0;
    for(int z = 0; z < height - 1; z++)
    {
        for(int x = 0; x < width - 1; x++)
        {
            int tl = z * width + x;
            int tr = z * width + x + 1;
            int bl = (z + 1) * width + x;
            int br = (z + 1) * width + x + 1;

            indices[index++] = tl;
            indices[index++] = bl;
            indices[index++] = tr;

            indices[index++] = tr;
            indices[index++] = bl;
            indices[index++] = br;
        }
    }

    return indices;
}

void main_state_init(GLFWwindow *window, void *args, int width, int height)
{
    // SHADER PROGRAM

    initialize_reflection_refraction_framebuffers(width, height);

    rafgl_raster_load_from_image(&water_normal_raster, "res/images/water_normal.jpg");
    rafgl_texture_init(&water_normal_map_tex);

    rafgl_texture_load_from_raster(&water_normal_map_tex, &water_normal_raster);

    //glBindTexture(GL_TEXTURE_2D, water_normal_map_tex.tex_id);
    //glGenerateMipmap(GL_TEXTURE_2D);

    vertices[0] = vertex(vec3( -100.0f,  0.0f,  100.0f), RAFGL_RED, 5.0f, 0.0f, 0.0f, RAFGL_VEC3_Y);
    vertices[1] = vertex(vec3( -100.0f,  0.0f, -100.0f), RAFGL_GREEN, 5.0f, 0.0f, 1.0f, RAFGL_VEC3_Y);
    vertices[2] = vertex(vec3(  100.0f,  0.0f,  100.0f), RAFGL_GREEN, 5.0f, 1.0f, 0.0f, RAFGL_VEC3_Y);

    vertices[3] = vertex(vec3(  100.0f,  0.0f,  100.0f), RAFGL_GREEN, 5.0f, 1.0f, 0.0f, RAFGL_VEC3_Y);
    vertices[4] = vertex(vec3( -100.0f,  0.0f, -100.0f), RAFGL_GREEN, 5.0f, 0.0f, 1.0f, RAFGL_VEC3_Y);
    vertices[5] = vertex(vec3(  100.0f,  0.0f, -100.0f), RAFGL_BLUE, 5.0f, 1.0f, 1.0f, RAFGL_VEC3_Y);

    shader_program_id = rafgl_program_create_from_name("custom_water_shader_v1");

    hill_shader_program_id = rafgl_program_create_from_name("custom_hills_shader_v1");

    //glUseProgram(hill_shader_program_id);

    glUniformMatrix4fv(glGetUniformLocation(hill_shader_program_id, "model"), 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(glGetUniformLocation(hill_shader_program_id, "view"), 1, GL_FALSE, (void*) view.m);
    glUniformMatrix4fv(glGetUniformLocation(hill_shader_program_id, "projection"), 1, GL_FALSE, (void*) projection.m);

    //glUseProgram(shader_program_id);

    vertex_t *hill_vertices = generate_hills(100, 100, 50.0f, &hill_vertex_count);
    GLuint *hill_indices = generate_hill_indices(100, 100, &hill_index_count);

    glGenVertexArrays(1, &hill_vao);
    glBindVertexArray(hill_vao);

    glGenBuffers(1, &hill_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, hill_vbo);
    glBufferData(GL_ARRAY_BUFFER, hill_vertex_count * sizeof(vertex_t), hill_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &hill_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hill_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, hill_index_count * sizeof(GLuint), hill_indices, GL_STATIC_DRAW);

    glBindVertexArray(hill_vao);

    // Position attribute (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position));
    glEnableVertexAttribArray(0);

    // Color attribute (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, colour));
    glEnableVertexAttribArray(1);

    // Alpha attribute (float)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, alpha));
    glEnableVertexAttribArray(2);

    // Texture coordinates (u, v)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, u));
    glEnableVertexAttribArray(3);

    // Normal attribute (vec3)
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, normal));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0); // Unbind VAO after setting it up

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

    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position));
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, colour));
    glEnableVertexAttribArray(1);

    // Alpha attribute
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, alpha));
    glEnableVertexAttribArray(2);

    // Texture coordinate attribute
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, u));
    glEnableVertexAttribArray(3);

    // Normal attribute
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, normal));
    glEnableVertexAttribArray(4);

    // VAO unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    free(hill_vertices);
    free(hill_indices);
}

void render_scene(mat4_t view_projection, float delta_time) {
    glUseProgram(shader_program_id);
    glUniformMatrix4fv(uni_VP, 1, GL_FALSE, (void*) view_projection.m);
    glUniform1f(uni_time, time_tick);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skybox_shader);
    glUniformMatrix4fv(skybox_uni_V, 1, GL_FALSE, (void*) view.m);
    glUniformMatrix4fv(skybox_uni_P, 1, GL_FALSE, (void*) projection.m);
    glBindVertexArray(skybox_mesh.vao_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture.tex_id);
    glDrawArrays(GL_TRIANGLES, 0, skybox_mesh.vertex_count);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void render_hills(mat4_t view_projection) {
    glUseProgram(hill_shader_program_id);
    glUniformMatrix4fv(glGetUniformLocation(hill_shader_program_id, "view_projection"), 1, GL_FALSE, (void*)view_projection.m);
    glBindVertexArray(hill_vao);
    glDrawElements(GL_TRIANGLES, hill_index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void render_water(mat4_t view_project, float delta_time) {
    glUseProgram(shader_program_id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);

    glUniformMatrix4fv(uni_VP, 1, GL_FALSE, (void*) view_project.m);
    glUniform1f(uni_time, time_tick);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args) {
    // rafgl_log_fps(RAFGL_TRUE);
    time_tick += delta_time;

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

    view = m4_look_at(camera_position, camera_target, camera_up);
    projection = m4_perspective(fov, (float)game_data->raster_width / game_data->raster_height, 0.1f, 1000.0f);
    view_projection = m4_mul(projection, view);

    // REFLECTION
    bindReflectionFrameBuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform4f(location_plane, 0.0f, 1.0f, 0.0f, 1.0f);
    vec3_t reflected_camera_pos = camera_position;
    reflected_camera_pos.y = -camera_position.y;
    mat4_t reflected_view = m4_look_at(reflected_camera_pos, camera_target, camera_up);

    render_scene(reflected_view, delta_time);

    // REFRACTION
    bindRefractionFrameBuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform4f(location_plane, 0.0f, -1.0f, 0.0f, 1.0f);
    render_scene(view_projection, delta_time);

    unbindCurrentFrameBuffer(game_data->raster_width, game_data->raster_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // OTHER
    glUniform4f(location_plane, 0.0f, -1.0f, 0.0f, 1.0f);
    render_scene(view_projection, delta_time);

    render_water(view_projection, delta_time);

    render_hills(view_projection);

    //glfwSwapBuffers(window);
    //glfwPollEvents();
}

void main_state_render(GLFWwindow *window, void *args)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // CLEAR
    // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    //glBindTexture(GL_TEXTURE_2D, water_normal_map_tex.tex_id);

    float aspect = (float)width / (float)height;
    projection = m4_perspective(fov, aspect, 0.1f, 1000.0f);

    // SKYBOX RENDER

    // WATER RENDER
    glUseProgram(shader_program_id);

    glUniformMatrix4fv(uni_M, 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(uni_VP, 1, GL_FALSE, (void*) view_projection.m);
    glUniform1f(uni_time, time_tick);
    glUniform3f(uni_camera_pos, camera_position.x, camera_position.y, camera_position.z);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDisable(GL_DEPTH_TEST);
}

void main_state_cleanup(GLFWwindow *window, void *args)
{
    frame_buffer_cleanup();
}


