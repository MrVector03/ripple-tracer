#include <frame_buffers.h>
#include <glad/glad.h>
#include <rafgl.h>

static const int REFLECTION_WIDTH = 320;
static const int REFLECTION_HEIGHT = 180;

static const int REFRACTION_WIDTH = 1280;
static const int REFRACTION_HEIGHT = 720;

int DISPLAY_WIDTH;
int DISPLAY_HEIGHT;


void frame_buffers_core_init(WaterFrameBuffers w_fbo, int width, int height) {
    DISPLAY_WIDTH = width;
    DISPLAY_HEIGHT = height;
    initialize_reflection_frame_buffer(w_fbo);
    initialize_refraction_frame_buffer(w_fbo);
}

void water_buffer_clean_up(WaterFrameBuffers w_fbo) {
    glDeleteFramebuffers(1, &w_fbo.reflection_frame_buffer);
    glDeleteTextures(1, &w_fbo.reflection_texture);
    glDeleteRenderbuffers(1, &w_fbo.reflection_depth_buffer);
    glDeleteFramebuffers(1, &w_fbo.refraction_frame_buffer);
    glDeleteTextures(1, &w_fbo.refraction_texture);
    glDeleteTextures(1, &w_fbo.refraction_depth_texture);
}

void bind_reflection_frame_buffer(WaterFrameBuffers w_fbo) {
    bind_frame_buffer(w_fbo.reflection_frame_buffer, REFLECTION_WIDTH, REFLECTION_HEIGHT); // TODO: POSSIBLE ERROR
}

void bind_refraction_frame_buffer(WaterFrameBuffers w_fbo) {
    bind_frame_buffer(w_fbo.refraction_frame_buffer, REFRACTION_WIDTH, REFRACTION_HEIGHT); // TODO: POSSIBLE ERROR
}

void unbind_current_frame_buffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

void initialize_reflection_frame_buffer(WaterFrameBuffers w_fbo) {
    w_fbo.reflection_frame_buffer = create_frame_buffer(w_fbo);
    w_fbo.reflection_texture = create_texture_attachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
    w_fbo.reflection_depth_buffer = create_depth_buffer_attachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
    unbind_current_frame_buffer();
}

void initialize_refraction_frame_buffer(WaterFrameBuffers w_fbo) {
    w_fbo.refraction_frame_buffer = create_frame_buffer(w_fbo);
    w_fbo.refraction_texture = create_texture_attachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
    w_fbo.refraction_depth_texture = create_depth_texture_attachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
    unbind_current_frame_buffer();
}

void bind_frame_buffer(GLuint frame_buffer, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glViewport(0, 0, width, height);
}

GLuint create_frame_buffer() {
    GLuint frame_buffer = 0;
    glGenFramebuffers(1, &frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    return frame_buffer;
}

GLuint create_texture_attachment(int width, int height) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    return texture;
}

GLuint create_depth_texture_attachment(int width, int height) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);

    return texture;
}

GLuint create_depth_buffer_attachment(int width, int height) {
    GLuint depth_buffer = 0;
    glGenRenderbuffers(1, &depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
    return depth_buffer;
}


