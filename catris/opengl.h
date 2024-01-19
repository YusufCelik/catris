#ifndef OPENGL_H
#define OPENGL_H

#include "app_context.h"

#define POSITION_ATTRIBUTE 0
#define COLOR_ATTRIBUTE 1
#define TEXTURE_COORD_ATTRIBUTE 2
#define VERTEX_SIZE 8 // Number of floats per vertex
#define POSITION_SIZE 3
#define COLOR_SIZE 3
#define TEXTURE_COORD_SIZE 2

void initShaders(ApplicationContext *context);
void setupVertexAttrib(GLuint index, GLint size, GLsizei stride, const void* pointer);
void setupShaderAndUniforms(GLuint shaderProgram, const mat4 projection, const mat4 model, float alpha);
void setupVertexData(GLuint *VAO, GLuint *VBO, GLuint *VEO, const float *vertices, size_t vertSize, const unsigned int *indices, size_t indSize);
void opengl_set_current_texture(GLuint texture);
GLuint opengl_load_texture_atlas(char* path);

#endif