#include <stdlib.h>
#include "glad\glad.h";

#include "app_context.h"
#include "opengl.h"

void initShaders(ApplicationContext * context) {
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
	
	context->shaderManager->programID = glCreateProgram();
	unsigned int ID = context->shaderManager->programID;
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	checkCompileErrors(ID, "PROGRAM");

	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void setupVertexAttrib(GLuint index, GLint size, GLsizei stride, const void* pointer) {
	glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride * sizeof(float), pointer);
	glEnableVertexAttribArray(index);
}

void setupShaderAndUniforms(GLuint shaderProgram, const mat4 projection, const mat4 model, float alpha) {
	glUseProgram(shaderProgram);
	GLint modelUniformLoc = glGetUniformLocation(shaderProgram, "model");
	GLint projUniformLoc = glGetUniformLocation(shaderProgram, "projection");
	GLint alphaLoc = glGetUniformLocation(shaderProgram, "alpha");

	glUniformMatrix4fv(projUniformLoc, 1, GL_FALSE, (float *)projection);
	glUniformMatrix4fv(modelUniformLoc, 1, GL_FALSE, (float *)model);
	glUniform1f(alphaLoc, alpha);
}

void setupVertexData(GLuint *VAO, GLuint *VBO, GLuint *VEO, const float *vertices, size_t vertSize, const unsigned int *indices, size_t indSize) {
	glGenVertexArrays(1, VAO);
	glGenBuffers(1, VBO);
	glGenBuffers(1, VEO);

	glBindVertexArray(*VAO);
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *VEO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indSize, indices, GL_STATIC_DRAW);

	setupVertexAttrib(POSITION_ATTRIBUTE, POSITION_SIZE, VERTEX_SIZE, (void*)0);
	setupVertexAttrib(COLOR_ATTRIBUTE, COLOR_SIZE, VERTEX_SIZE, (void*)(POSITION_SIZE * sizeof(float)));
	setupVertexAttrib(TEXTURE_COORD_ATTRIBUTE, TEXTURE_COORD_SIZE, VERTEX_SIZE, (void*)((POSITION_SIZE + COLOR_SIZE) * sizeof(float)));

	glBindVertexArray(0);
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
