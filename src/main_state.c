#include <float.h>
#include <main_state.h>
#include <glad/glad.h>
#include <math.h>
#include <rafgl.h>
#include <game_constants.h>
#include <utility.h>
#include <time.h>
#include "stb_image_write.h"

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

static int p[512];

static int grad3[12][3] = {
    {1, 1, 0}, {-1, 1, 0}, {1, -1, 0},{-1, -1, 0},
    {1, 0, 1}, {-1, 0, 1},{1, 0, -1}, {-1, 0, -1},
    {0, 1, 1},{0, -1, 1}, {0, 1, -1}, {0, -1, -1}
};

void init_perm() {
    for (int i = 0; i < 256; ++i) {
        p[i] = i;
    }
    for (int i = 255; i > 0; --i) {
        int j = rand() % (i + 1);
        int temp = p[i];
        p[i] = p[j];
        p[j] = temp;
    }
    for (int i = 0; i < 256; ++i) {
        p[256 + i] = p[i];
    }
}

float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float dot(int* g, float x, float y) {
    return g[0] * x + g[1] * y;
}

int* grad(int hash) {
    return grad3[hash % 12];
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float perlin(float x, float y) {
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    float xf = x - floor(x);
    float yf = y - floor(y);
    float u = fade(xf);
    float v = fade(yf);

    int A = p[X] + Y;
    int AA = p[A];
    int AB = p[A + 1];
    int B = p[X + 1] + Y;
    int BA = p[B];
    int BB = p[B + 1];

    // Calculate the dot products
    float gradAA = dot(grad(AA), xf, yf);
    float gradBA = dot(grad(BA), xf - 1, yf);
    float gradAB = dot(grad(AB), xf, yf - 1);
    float gradBB = dot(grad(BB), xf - 1, yf - 1);

    // Interpolate between the values
    float lerpX1 = lerp(gradAA, gradBA, u);
    float lerpX2 = lerp(gradAB, gradBB, u);
    return lerp(lerpX1, lerpX2, v);
}

#define HEIGHT_MAP_WIDTH 1000
#define HEIGHT_MAP_HEIGHT 1000

void generate_height_map(vertex_t *vertices, int num_vertices, float height_map[HEIGHT_MAP_WIDTH][HEIGHT_MAP_HEIGHT]) {
    // Initialize the height map to zero
    for (int i = 0; i < HEIGHT_MAP_WIDTH; i++) {
        for (int j = 0; j < HEIGHT_MAP_HEIGHT; j++) {
            height_map[i][j] = 0.0f;
        }
    }

    // Determine the range of the vertex coordinates
    float min_x = FLT_MAX, max_x = -FLT_MAX;
    float min_z = FLT_MAX, max_z = -FLT_MAX;
    for (int i = 0; i < num_vertices; i++) {
        if (vertices[i].position.x < min_x) min_x = vertices[i].position.x;
        if (vertices[i].position.x > max_x) max_x = vertices[i].position.x;
        if (vertices[i].position.z < min_z) min_z = vertices[i].position.z;
        if (vertices[i].position.z > max_z) max_z = vertices[i].position.z;
    }

    // Iterate through the vertices and update the height map
    for (int i = 0; i < num_vertices; i++) {
        int x = (int)((vertices[i].position.x - min_x) / (max_x - min_x) * (HEIGHT_MAP_WIDTH - 1));
        int z = (int)((vertices[i].position.z - min_z) / (max_z - min_z) * (HEIGHT_MAP_HEIGHT - 1));
        float height = vertices[i].position.y;

        // Ensure the coordinates are within bounds
        if (x >= 0 && x < HEIGHT_MAP_WIDTH && z >= 0 && z < HEIGHT_MAP_HEIGHT) {
            height_map[x][z] = height;
        }
    }
}

void save_height_map_as_image(float height_map[HEIGHT_MAP_WIDTH][HEIGHT_MAP_HEIGHT], const char *filename) {
    unsigned char *image = (unsigned char *)malloc(HEIGHT_MAP_WIDTH * HEIGHT_MAP_HEIGHT);
    float max_height = 0.0f;

    // Find the maximum height value
    for (int i = 0; i < HEIGHT_MAP_WIDTH; i++) {
        for (int j = 0; j < HEIGHT_MAP_HEIGHT; j++) {
            if (height_map[i][j] > max_height) {
                max_height = height_map[i][j];
            }
        }
    }

    // Normalize the height values and convert to grayscale
    for (int i = 0; i < HEIGHT_MAP_WIDTH; i++) {
        for (int j = 0; j < HEIGHT_MAP_HEIGHT; j++) {
            image[i + j * HEIGHT_MAP_WIDTH] = (unsigned char)((height_map[i][j] / max_height) * 255.0f);
        }
    }

    // Save the image as a PNG file
    stbi_write_png(filename, HEIGHT_MAP_WIDTH, HEIGHT_MAP_HEIGHT, 1, image, HEIGHT_MAP_WIDTH);
    free(image);
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

vec3_t camera_position = {10.013418, 10.693488, 10.864704};
vec3_t camera_target = {0.0f, 0.0f, 0.0f};
vec3_t camera_up = {0.0f, 1.0f, 0.0f};
vec3_t aim_dir = {0.0f, 0.0f, 0.0f};

float camera_angle = -M_PIf / 2.0f;
float angle_speed = 0.2f * M_PIf;
float move_speed = 8.8f;

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

// HILLS

GLuint hill_shader_program_id;
GLuint hill_vao, hill_vbo, hill_ebo;
int hill_vertex_count, hill_index_count;
rafgl_raster_t hill_raster, hill_sand_raster, hill_grass_raster;
rafgl_texture_t hill_texture, hill_sand_texture, hill_grass_texture;
static GLuint hill_texture_id, hill_sand_texture_id, hill_grass_texture_id;

// WATER LEVEL
float water_level = -4.0f;

// LIGHT SOURCE
GLuint depthFBO;
GLuint depthMap;
GLuint lightning_shader_program_id;

GLuint fog_color_location;
GLuint fog_density_location;

float fog_density = 0.00001f;
vec3_t fog_color = {0.1f, 0.1, 0.1f};

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

void initialize_reflection_refraction_framebuffers_v2(int width, int height) {
    // Reflection framebuffer
    glGenFramebuffers(1, &reflectionFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFrameBuffer);

    // Create texture for reflection
    glGenTextures(1, &reflectionTexture);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTexture, 0);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Reflection Framebuffer not complete!\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Refraction framebuffer
    glGenFramebuffers(1, &refractionFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, refractionFrameBuffer);

    // Create texture for refraction
    glGenTextures(1, &refractionTexture);
    glBindTexture(GL_TEXTURE_2D, refractionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractionTexture, 0);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Refraction Framebuffer not complete!\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void frame_buffer_cleanup() {
    glDeleteFramebuffers(1, &reflectionFrameBuffer);
    glDeleteTextures(1, &reflectionTexture);
    glDeleteRenderbuffers(1, &reflectionDepthBuffer);
    glDeleteFramebuffers(1, &refractionFrameBuffer);
    glDeleteTextures(1, &refractionTexture);
    glDeleteTextures(1, &refractionDepthTexture);
}

float noise(float x, float z) {
    return 0.0f;
}

float smoothstep(float x, float river_width, float river_mask) {
    float t = rafgl_clampf((x - river_width) / (river_mask - river_width), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float mix(float x, float x1, float river_mask) {
    return x * (1.0f - river_mask) + x1 * river_mask;
}

vertex_t* generate_hills(int width, int height, float scale, int* vertex_count, float water_level, float river_width) {
    int num_vertices = width * height;
    vertex_t* vertices = (vertex_t*)malloc(num_vertices * sizeof(vertex_t));
    *vertex_count = num_vertices;

    float x_offset = width / 2.0f;
    float z_offset = height / 2.0f;

    srand(time(NULL));

    init_perm();

    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            float noise_value = perlin(x * 0.02f, z * 0.02f);
            float random_noise = ((float)rand() / RAND_MAX) * 0.2f - 0.1f;
            float height_variation = (noise_value + random_noise) * scale;

            float y = height_variation * 0.5f;
            y += ((float)rand() / RAND_MAX) * scale * 0.1f;

            if (y < water_level) {
                y = water_level + (y - water_level) * 0.7f;
            }

            float underwater_mask = perlin(x * 0.03f, z * 0.03f);
            underwater_mask = smoothstep(0.3f, 0.7f, underwater_mask);
            y = mix(y, water_level - scale * 0.2f, underwater_mask);

            float river_mask = perlin(x * 0.01f, z * 0.01f);
            river_mask = fabs(river_mask);
            river_mask = 1.0f - smoothstep(0.0f, river_width, river_mask);
            y = mix(y, water_level - scale * 0.5f, river_mask);

            if (y < water_level) {
                y = water_level;
            }

            // Assign the vertex
            vertices[z * width + x] = vertex(vec3(x - x_offset, y, z - z_offset),
                                             vec3(0.0f, 1.0f, 0.0f), 1.0f,
                                             (float)x / width, (float)z / height,
                                             vec3(0.0f, 1.0f, 0.0f));
        }
    }

    return vertices;
}

// Function to generate indices for the grid of vertices
GLuint* generate_hill_indices(int width, int height, int* index_count) {
    int num_indices = (width - 1) * (height - 1) * 6;
    GLuint* indices = malloc(num_indices * sizeof(GLuint));
    *index_count = num_indices;

    int index = 0;
    for (int z = 0; z < height - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
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

vec3_t light_position = {10.0f, 20.0f, 10.0f};
vec3_t light_color = {1.0f, 1.0f, 1.0f};

float reflection_uv_offset = 0.0f;

void main_state_init(GLFWwindow *window, void *args, int width, int height)
{
    // WATER
    rafgl_raster_load_from_image(&water_normal_raster, "res/images/water_normal2.jpg");
    rafgl_texture_init(&water_normal_map_tex);

    initialize_reflection_refraction_framebuffers_v2(width, height);

    rafgl_texture_load_from_raster(&water_normal_map_tex, &water_normal_raster);

    glBindTexture(GL_TEXTURE_2D, water_normal_map_tex.tex_id); /* bajndujemo doge teksturu */

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


    glBindTexture(GL_TEXTURE_2D, 0);

    vertices[0] = vertex(vec3( -1000.0f,  0.0f,  1000.0f), RAFGL_RED, 1.0f, 0.0f, 0.0f, RAFGL_VEC3_Y);
    vertices[1] = vertex(vec3( -1000.0f,  0.0f, -1000.0f), RAFGL_GREEN, 1.0f, 0.0f, 1.0f, RAFGL_VEC3_Y);
    vertices[2] = vertex(vec3(  1000.0f,  0.0f,  1000.0f), RAFGL_GREEN, 1.0f, 1.0f, 0.0f, RAFGL_VEC3_Y);

    vertices[3] = vertex(vec3(  1000.0f,  0.0f,  1000.0f), RAFGL_GREEN, 1.0f, 1.0f, 0.0f, RAFGL_VEC3_Y);
    vertices[4] = vertex(vec3( -1000.0f,  0.0f, -1000.0f), RAFGL_GREEN, 1.0f, 0.0f, 1.0f, RAFGL_VEC3_Y);
    vertices[5] = vertex(vec3(  1000.0f,  0.0f, -1000.0f), RAFGL_BLUE, 1.0f, 1.0f, 1.0f, RAFGL_VEC3_Y);


    shader_program_id = rafgl_program_create_from_name("custom_water_shader_v3");
    uni_M = glGetUniformLocation(shader_program_id, "uni_M");
    uni_VP = glGetUniformLocation(shader_program_id, "uni_VP");
    uni_phase = glGetUniformLocation(shader_program_id, "uni_phase");
    uni_camera_pos = glGetUniformLocation(shader_program_id, "uni_camera_pos");


    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vertex_t), vertices, GL_STATIC_DRAW);

    /* position */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*) 0);

    /* colour */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*) (sizeof(vec3_t)));

    /* alpha */
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*) (2 * sizeof(vec3_t)));

    /* UV coords */
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*) (2 * sizeof(vec3_t) + 1 * sizeof(float)));

    /* normal */
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*) (2 * sizeof(vec3_t) + 3 * sizeof(float)));


    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // HILLS ROCKS
    rafgl_raster_load_from_image(&hill_raster, "res/images/rock_texture.jpg");
    rafgl_texture_init(&hill_texture);
    rafgl_texture_load_from_raster(&hill_texture, &hill_raster);
    hill_texture_id = hill_texture.tex_id;

    // HILLS SAND
    rafgl_raster_load_from_image(&hill_sand_raster, "res/images/sand_texture.jpg");
    rafgl_texture_init(&hill_sand_texture);
    rafgl_texture_load_from_raster(&hill_sand_texture, &hill_sand_raster);
    hill_sand_texture_id = hill_sand_texture.tex_id;

    // HILLS GRASS
    rafgl_raster_load_from_image(&hill_grass_raster, "res/images/grass_field_texture.jpg");
    rafgl_texture_init(&hill_grass_texture);
    rafgl_texture_load_from_raster(&hill_grass_texture, &hill_grass_raster);
    hill_grass_texture_id = hill_grass_texture.tex_id;

    glBindTexture(GL_TEXTURE_2D, hill_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, hill_sand_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, hill_grass_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);


    lightning_shader_program_id = rafgl_program_create_from_name("custom_depth_lightning_v1");

    // LIGHT SOURCE
    glGenFramebuffers(1, &depthFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // HILLS
    hill_shader_program_id = rafgl_program_create_from_name("custom_hills_shader_v1");

    glUniformMatrix4fv(glGetUniformLocation(hill_shader_program_id, "model"), 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(glGetUniformLocation(hill_shader_program_id, "view"), 1, GL_FALSE, (void*) view.m);
    glUniformMatrix4fv(glGetUniformLocation(hill_shader_program_id, "projection"), 1, GL_FALSE, (void*) projection.m);

    vertex_t *hill_vertices = generate_hills(1000, 1000, 75.0f, &hill_vertex_count, water_level, 400);
    int num_hills_vertices = hill_vertex_count;
    float height_map[HEIGHT_MAP_WIDTH][HEIGHT_MAP_HEIGHT];
    generate_height_map((float*)hill_vertices, num_hills_vertices, height_map);
    save_height_map_as_image(height_map, "height_map.png");

    GLuint *hill_indices = generate_hill_indices(1000, 1000, &hill_index_count);

    glGenVertexArrays(1, &hill_vao);
    glBindVertexArray(hill_vao);

    glGenBuffers(1, &hill_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, hill_vbo);
    glBufferData(GL_ARRAY_BUFFER, hill_vertex_count * sizeof(vertex_t), hill_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &hill_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hill_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, hill_index_count * sizeof(GLuint), hill_indices, GL_STATIC_DRAW);

    glBindVertexArray(hill_vao);

    // Position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position));
    glEnableVertexAttribArray(0);

    // Color (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, colour));
    glEnableVertexAttribArray(1);

    // Alpha (float)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, alpha));
    glEnableVertexAttribArray(2);

    // Texture (u, v)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, u));
    glEnableVertexAttribArray(3);

    // Normal (vec3)
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

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position));
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, colour));
    glEnableVertexAttribArray(1);

    // Alpha
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, alpha));
    glEnableVertexAttribArray(2);

    // Texture coordinate
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, u));
    glEnableVertexAttribArray(3);

    // Normal
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, normal));
    glEnableVertexAttribArray(4);

    // VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    free(hill_vertices);
    free(hill_indices);
}

void render_hills(mat4_t view_projection) {
    glUseProgram(hill_shader_program_id);

    glUniformMatrix4fv(glGetUniformLocation(hill_shader_program_id, "view_projection"), 1, GL_FALSE, (void*)view_projection.m);
    glUniform3f(glGetUniformLocation(hill_shader_program_id, "light_position"), light_position.x, light_position.y, light_position.z);
    glUniform3f(glGetUniformLocation(hill_shader_program_id, "light_color"), light_color.x, light_color.y, light_color.z);
    glUniform3f(glGetUniformLocation(hill_shader_program_id, "view_position"), camera_position.x, camera_position.y, camera_position.z);
    glUniform1f(glGetUniformLocation(hill_shader_program_id, "water_height"), water_level);

    glUniform3f(glGetUniformLocation(hill_shader_program_id, "fog_color"), fog_color.x, fog_color.y, fog_color.z);
    glUniform1f(glGetUniformLocation(hill_shader_program_id, "fog_density"), fog_density);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hill_texture_id);
    glUniform1i(glGetUniformLocation(hill_shader_program_id, "hillTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, hill_sand_texture_id);
    glUniform1i(glGetUniformLocation(hill_shader_program_id, "sandTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, hill_grass_texture_id);
    glUniform1i(glGetUniformLocation(hill_shader_program_id, "grassTexture"), 2);

    glBindVertexArray(hill_vao);
    glDrawElements(GL_TRIANGLES, hill_index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void render_scene(mat4_t view_projection, int width, int height) {

    glViewport(0, 0, width, height);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // SKYBOX
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture.tex_id);
    glUniform1i(glGetUniformLocation(skybox_shader, "skyboxTexture"), 1);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skybox_shader);
    glUniformMatrix4fv(skybox_uni_V, 1, GL_FALSE, (void*) view.m);
    glUniformMatrix4fv(skybox_uni_P, 1, GL_FALSE, (void*) projection.m);

    // Fog parameters for skybox
    glUniform3f(glGetUniformLocation(skybox_shader, "fog_color"), fog_color.x, fog_color.y, fog_color.z); // Example fog color
    glUniform1f(glGetUniformLocation(skybox_shader, "fog_density"), fog_density); // Increased fog density
    glUniform3f(glGetUniformLocation(skybox_shader, "uni_camera_pos"), camera_position.x, camera_position.y, camera_position.z);

    glBindVertexArray(skybox_mesh.vao_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture.tex_id);
    glDrawArrays(GL_TRIANGLES, 0, skybox_mesh.vertex_count);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glEnable(GL_DEPTH_TEST);

    float aspect = (float)width / (float)height;
    projection = m4_perspective(fov, aspect, 0.1f, 1000.0f);
    view_projection = m4_mul(projection, view);

    // HILLS
    render_hills(view_projection);
}


void render_water(mat4_t view_projection) {
    glUseProgram(shader_program_id);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Bind the normal map texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, water_normal_map_tex.tex_id);
    glUniform1i(glGetUniformLocation(shader_program_id, "normal_map"), 0);

    // Bind the reflection texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);
    glUniform1i(glGetUniformLocation(shader_program_id, "reflection_texture"), 1);

    // Bind the refraction texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, refractionTexture);
    glUniform1i(glGetUniformLocation(shader_program_id, "refraction_texture"), 2);

    glUniform1f(glGetUniformLocation(shader_program_id, "refraction_index"), 1.33f);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glBindVertexArray(vao);

    glUniformMatrix4fv(uni_M, 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(uni_VP, 1, GL_FALSE, (void*) view_projection.m);
    glUniform1f(uni_phase, time_tick * 0.1f);
    glUniform3f(uni_camera_pos, camera_position.x, camera_position.y, camera_position.z);
    glUniform3f(glGetUniformLocation(shader_program_id, "light_position"), light_position.x, light_position.y, light_position.z);
    glUniform3f(glGetUniformLocation(shader_program_id, "light_color"), light_color.x, light_color.y, light_color.z);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args) {
    time_tick += delta_time;

    // rotate light source
    light_position.x = 10000.0f * cosf(time_tick / 20.0f);
    light_position.y = 10000.0f * sinf(time_tick / 20.0f);

    if (game_data->keys_down['Q'])
        reflection_uv_offset += 1.0f;
    else if (game_data->keys_down['E'])
        reflection_uv_offset -= 1.0f;

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

    //render_scene(reflected_view, delta_time);
    //render_water(view_projection, delta_time);

    // REFRACTION
    bindRefractionFrameBuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform4f(location_plane, 0.0f, -1.0f, 0.0f, 15.0f);
    render_scene(view_projection, game_data->raster_width, game_data->raster_height);

    unbindCurrentFrameBuffer(game_data->raster_width, game_data->raster_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform4f(location_plane, 0.0f, -1.0f, 0.0f, 15.0f);
    render_scene(view_projection, game_data->raster_width, game_data->raster_height);

    // ASSIGN FOG UNIFORMS
    glUseProgram(shader_program_id);
    glUniform3f(glGetUniformLocation(shader_program_id, "fog_color"), fog_color.x, fog_color.y, fog_color.z);
    glUniform1f(glGetUniformLocation(shader_program_id, "fog_density"), fog_density);
    glUniform1f(glGetUniformLocation(shader_program_id, "reflection_uv_scale"), reflection_uv_offset);

    glUseProgram(hill_shader_program_id);
    glUniform3f(glGetUniformLocation(hill_shader_program_id, "fog_color"), fog_color.x, fog_color.y, fog_color.z);
    glUniform1f(glGetUniformLocation(hill_shader_program_id, "fog_density"), fog_density);

    // RENDER LIGHT SOURCE
    mat4_t light_projection = m4_ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 20.0f);
    mat4_t light_view = m4_look_at(vec3(0.0f, 10.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f));
    mat4_t light_space_matrix = m4_mul(light_projection, light_view);

    if (game_data->keys_down['D'])
        camera_position = v3_sub(camera_position, v3_muls(v3_cross(camera_up, aim_dir), move_speed * delta_time));
    if (game_data->keys_down['A'])
        camera_position = v3_add(camera_position, v3_muls(v3_cross(camera_up, aim_dir), move_speed * delta_time));

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

void main_state_render(GLFWwindow *window, void *args)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    render_scene(m4_mul(projection, view), width, height);
    render_water(m4_mul(projection, view));
}

void main_state_cleanup(GLFWwindow *window, void *args)
{
    frame_buffer_cleanup();
}