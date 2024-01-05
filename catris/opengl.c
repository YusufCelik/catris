#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "opengl.h"
#include "config.h"
#include "game_state.h"

char* readShaderSource(const char* filePath) {
	FILE *fp;
	long size = 0;
	char* shaderContent;

	/* Read File to get size */
	fp = fopen(filePath, "rb");
	if (fp == NULL) {
		return "";
	}
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp) + 1;
	fclose(fp);

	/* Read File for Content */
	fp = fopen(filePath, "r");
	shaderContent = memset(malloc(size), '\0', size);
	fread(shaderContent, 1, size - 1, fp);
	fclose(fp);

	return shaderContent;
}

void opengl_set_current_texture(GLuint texture) {
	glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
}

GLuint opengl_load_texture_atlas(char* path) {
	unsigned int texture;
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(1);
	unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// load and generate the texture

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		printf("Could not load texture");
	}


	stbi_image_free(data);

	return texture;
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, 1);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void checkCompileErrors(unsigned int shader, const char* type)
{
	GLint success;
	GLchar infoLog[1024];

	if (strcmp(type, "PROGRAM") != 0)
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
			printf("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n -- --------------------------------------------------- -- \n", type, infoLog);
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shader, sizeof(infoLog), NULL, infoLog);
			printf("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n -- --------------------------------------------------- -- \n", type, infoLog);
		}
	}
}


void calculate_uv_coords(int texture_atlas_width, int texture_atlas_height, int tile_x, int tile_y, int tile_width, int tile_height, float* uv_coords) {
	// Calculate the UV coordinates
	float left = (float)tile_x / texture_atlas_width;
	float right = (float)(tile_x + tile_width) / texture_atlas_width;
	float bottom = 1.0f - ((float)(tile_y + tile_height) / texture_atlas_height);
	float top = 1.0f - ((float)tile_y / texture_atlas_height);

	printf("UV Coordinates: {%.2f, %.2f, %.2f, %.2f}\n", left, right, bottom, top);

	// Store the UV coordinates in the provided array
	uv_coords[0] = right;
	uv_coords[1] = top;
	uv_coords[2] = right;
	uv_coords[3] = bottom;
	uv_coords[4] = left;
	uv_coords[5] = bottom;
	uv_coords[6] = left;
	uv_coords[7] = top;
}

unsigned int opengl_init_shaders() {
	unsigned int ID;
	unsigned int vertex, fragment;
	char* vertexShaderSource = readShaderSource("shaders/vertex_shader.glsl");
	char* fragmentShaderSource = readShaderSource("shaders/fragment_shader.glsl");

	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertexShaderSource, NULL);
	glCompileShader(vertex);
	checkCompileErrors(vertex, "VERTEX");

	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragment);
	checkCompileErrors(fragment, "FRAGMENT");

	// shader Program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	checkCompileErrors(ID, "PROGRAM");

	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return ID;
}

GLFWwindow* opengl_create_window(GameState *gameState) {
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		printf("Could not initialize glfwInit");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Catris", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		printf("could not create window");
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("failed to initialize GLAD");
	}

	return window;
}

void opengl_translate_block(mat4 model, GameState *gameState) {
	glUseProgram(gameState->SHADER_PROGRAM);
	GLint model_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "model");
	GLint projection_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "projection");

	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, (float *)gameState->projection);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, (float *)model);
}

void opengl_init_bg(GameState *gameState) {
	float vertices[] = {
		// positions          // colors           // texture coords
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top right
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom right
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,   // bottom left
		-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f    // top left 
	};

	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	glm_mat4_identity(gameState->bg.model);

	vec3 size = { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f };
	glm_scale(&gameState->bg.model, &size);

	glUseProgram(gameState->SHADER_PROGRAM);
	GLint model_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "model");
	GLint projection_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "projection");
	GLint alpha_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "alpha");

	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, (float *)gameState->projection);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, (float *)gameState->bg.model);
	glUniform1f(alpha_location, 1.0f);

	glGenVertexArrays(1, &gameState->bg.renderComponent.VAO);
	glGenBuffers(1, &gameState->bg.renderComponent.VBO);
	glGenBuffers(1, &gameState->bg.renderComponent.VEO);

	glBindVertexArray(gameState->bg.renderComponent.VAO);

	glBindBuffer(GL_ARRAY_BUFFER, gameState->bg.renderComponent.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gameState->bg.renderComponent.VEO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture coord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
}