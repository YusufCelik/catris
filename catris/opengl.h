#ifndef OPENGL_H
#define OPENGL_H

#include <cglm/struct.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "game_state.h"

typedef struct RenderComponent {
	unsigned int VAO;
	unsigned int VBO;
	unsigned int VEO;
	float tile_x;
	float tile_y;
} RenderComponent;

char* readShaderSource(const char* filePath);

void opengl_set_current_texture(GLuint texture);

GLuint opengl_load_texture_atlas(char* path);

void processInput(GLFWwindow *window);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void checkCompileErrors(unsigned int shader, const char* type);

void calculate_uv_coords(int texture_atlas_width, int texture_atlas_height, int tile_x, int tile_y, int tile_width, int tile_height, float* uv_coords);

unsigned int opengl_init_shaders(); 

GLFWwindow* opengl_create_window(GameState *gameState);

void opengl_translate_block(mat4 model, GameState *gameState);

void opengl_init_bg(GameState *gameState);

#endif