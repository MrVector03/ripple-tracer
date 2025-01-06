#ifndef FRAME_BUFFERS_H
#define FRAME_BUFFERS_H
#include <glad/glad.h>

typedef struct WaterFrameBuffers {
    GLuint reflection_frame_buffer;
    GLuint reflection_texture;
    GLuint reflection_depth_buffer;

    GLuint refraction_frame_buffer;
    GLuint refraction_texture;
    GLuint refraction_depth_texture;
} WaterFrameBuffers;

void frame_buffers_core_init(WaterFrameBuffers w_fbo, int width, int height);
void water_buffer_clean_up(WaterFrameBuffers w_fbo);

void bind_reflection_frame_buffer(WaterFrameBuffers w_fbo);
void bind_refraction_frame_buffer(WaterFrameBuffers w_fbo);

void unbind_current_frame_buffer();
void bind_frame_buffer(GLuint frame_buffer, int width, int height);

GLuint create_frame_buffer();

GLuint create_texture_attachment(int width, int height);
GLuint create_depth_texture_attachment(int width, int height);
GLuint create_depth_buffer_attachment(int width, int height);

void initialize_reflection_frame_buffer(WaterFrameBuffers w_fbo);
void initialize_refraction_frame_buffer(WaterFrameBuffers w_fbo);

#endif //FRAME_BUFFERS_H
