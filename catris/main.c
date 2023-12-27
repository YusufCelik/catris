#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <cglm\cam.h>
#include <cglm/struct.h>
#include <math.h>
#include <time.h> 

void processInput(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void checkCompileErrors(unsigned int shader, const char* type);
void calculate_uv_coords(int texture_atlas_width, int texture_atlas_height, int tile_x, int tile_y, int tile_width, int tile_height, float* uv_coords);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

typedef enum {
	TETROMINO_I,
	TETROMINO_O,
	TETROMINO_T,
	TETROMINO_J,
	TETROMINO_L,
	TETROMINO_S,
	TETROMINO_Z
} TetrominoShape;

typedef enum {
	BLOCK_DESCENDING,
	BLOCK_COLLIDED
} BlockStates;

typedef enum {
	IDLE,
	SPAWN_NEXT_BLOCK
} SystemActions;

typedef struct RenderComponent {
	unsigned int VAO;
	unsigned int VBO;
	unsigned int VEO;
	float tile_x;
	float tile_y;
} RenderComponent;

typedef struct SingleBlock {
	mat4 model;
	BlockStates currentState;
	RenderComponent renderComponent;
} SingleBlock;

typedef struct BG {
	mat4 model;
	RenderComponent renderComponent;
} BG;

typedef struct GameState {
	SingleBlock blocks[160];
	BG bg;
	TetrominoShape current_shape;
	int num_blocks;
	mat4 projection;
	unsigned int SHADER_PROGRAM;
	float SCREEN_HEIGHT;
	float SCREEN_WIDTH;
	float TILE_SIZE;
	SystemActions action_queue;
} GameState;

typedef struct cVec2 {
	float x;
	float y;
} cVec2;


cVec2 get_block_position(mat4 local_transform) {
	cVec2 pos;
	pos.x = local_transform[3][0];
	pos.y = local_transform[3][1];

	return pos;
}

cVec2 get_block_scale(mat4 local_transform) {
	cVec2 scale;
	scale.x = local_transform[0][0];
	scale.y = local_transform[1][1];

	return scale;
}



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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
	window = glfwCreateWindow(gameState->SCREEN_WIDTH, gameState->SCREEN_HEIGHT, "Catris", NULL, NULL);
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

void opengl_translate_block(unsigned int block_id, GameState *gameState) {
	glUseProgram(gameState->SHADER_PROGRAM);
	GLint model_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "model");
	GLint projection_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "projection");

	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, (float *)gameState->projection);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, (float *)gameState->blocks[block_id].model);
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

	vec3 size = { 1024.0f, 768.0f, 1.0f };
	glm_scale(&gameState->bg.model, &size);

	glUseProgram(gameState->SHADER_PROGRAM);
	GLint model_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "model");
	GLint projection_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "projection");

	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, (float *)gameState->projection);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, (float *)gameState->bg.model);

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
}

void opengl_init_block(unsigned int block_id, GameState *gameState) {
	float uv_coords[8];
	int texture_atlas_width = 640;
	int texture_atlas_height = 640;
	int tile_width = 64;
	int tile_height = 64;

	calculate_uv_coords(
		texture_atlas_width, texture_atlas_height, 
		gameState->blocks[block_id].renderComponent.tile_x, gameState->blocks[block_id].renderComponent.tile_y, tile_width, tile_height, uv_coords);

	float vertices[] = {
		// positions          // colors           // texture coords
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, uv_coords[0], uv_coords[1],   // top right
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, uv_coords[2], uv_coords[3],   // bottom right
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, uv_coords[4], uv_coords[5],   // bottom left
		-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, uv_coords[6], uv_coords[7]    // top left 
	};

	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	glm_mat4_identity(gameState->blocks[block_id].model);

	vec3 size = { 64, 64, 1.0f };
	glm_scale(&gameState->blocks[block_id].model, &size);

	glUseProgram(gameState->SHADER_PROGRAM);
	GLint model_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "model");
	GLint projection_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "projection");

	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, (float *)gameState->projection);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, (float *)gameState->blocks[block_id].model);

	glGenVertexArrays(1, &gameState->blocks[block_id].renderComponent.VAO);
	glGenBuffers(1, &gameState->blocks[block_id].renderComponent.VBO);
	glGenBuffers(1, &gameState->blocks[block_id].renderComponent.VEO);

	glBindVertexArray(gameState->blocks[block_id].renderComponent.VAO);

	glBindBuffer(GL_ARRAY_BUFFER, gameState->blocks[block_id].renderComponent.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gameState->blocks[block_id].renderComponent.VEO);
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
}


void opengl_setup_camera(GameState *gameState) {
	float right = gameState->SCREEN_WIDTH / 2;
	float left = -right; 
	float top = gameState->SCREEN_HEIGHT / 2;
	float bottom = -top;  
	float near = -1.0f;    // Near clipping plane
	float far = 1.0f;      // Far clipping plane

	glm_ortho(left, right, bottom, top, near, far, gameState->projection);
}


void calculateBoundingBox(GameState *gameState, float *bbox) {
	int start_block_id = gameState->num_blocks - 4;
	float min_x = 1e6, min_y = 1e6;
	float max_x = -1e6, max_y = -1e6;

	for (int i = start_block_id; i < start_block_id + 4; ++i) {
		float x = gameState->blocks[i].model[3][0];
		float y = gameState->blocks[i].model[3][1];

		if (x < min_x) min_x = x;
		if (y < min_y) min_y = y;
		if (x > max_x) max_x = x;
		if (y > max_y) max_y = y;
	}

	bbox[0] = min_x;
	bbox[1] = min_y;
	bbox[2] = (max_x - min_x) + gameState->TILE_SIZE; // width
	bbox[3] = (max_y - min_y) + gameState->TILE_SIZE; // height
	bbox[4] = (min_x + max_x) / 2; // center_x
	bbox[5] = (min_y + max_y) / 2; // center_y
}


void rotate_block_around_pivot(unsigned int pivot_id, unsigned int block_id, GameState *gameState, unsigned int center) {
	vec3 rot = { 0.0f, 0.0f, 1.0f };
	float bbox[6]; // x, y, width, height, center_x, center_y
	calculateBoundingBox(gameState, bbox);

	unsigned int pivot_id_start_offset = gameState->num_blocks - 4;

	float move_to_x;
	float move_to_y;

	if (center == 1) {
		move_to_x = bbox[4];
		move_to_y = bbox[5]; 
	}
	else {
		move_to_x = gameState->blocks[pivot_id_start_offset + pivot_id].model[3][0];
		move_to_y = gameState->blocks[pivot_id_start_offset + pivot_id].model[3][1];
	}

	// Calculate the translation needed to align B with A
	float y_translation_needed = move_to_y - gameState->blocks[block_id].model[3][1];
	float x_translation_needed = move_to_x - gameState->blocks[block_id].model[3][0];

	vec3 world_vec = { x_translation_needed, y_translation_needed, 0.0f };
	mat3 rotationMatrix;
	glm_mat4_pick3(&gameState->blocks[block_id].model, rotationMatrix);

	mat3 inverseRotationMatrix;
	glm_mat3_inv(rotationMatrix, inverseRotationMatrix);

	vec3 localVec;
	glm_mat3_mulv(inverseRotationMatrix, world_vec, localVec);

	glm_translate(&gameState->blocks[block_id].model, localVec);
	glm_rotate(&gameState->blocks[block_id].model, glm_rad(-90.0f), rot);
	glm_vec3_negate(localVec);
	glm_translate(&gameState->blocks[block_id].model, localVec);

}


void translate_block(float x, float y, unsigned int block_id, GameState *gameState) {
	vec3 world_vec = { x, y, 0.0f };
	mat3 rotationMatrix;
	glm_mat4_pick3(&gameState->blocks[block_id].model, rotationMatrix);

	mat3 inverseRotationMatrix;
	glm_mat3_inv(rotationMatrix, inverseRotationMatrix);

	vec3 localVec;
	glm_mat3_mulv(inverseRotationMatrix, world_vec, localVec);

	glm_translate(&gameState->blocks[block_id].model, localVec);
}

void init_and_translate_block(float tile_x, float tile_y, float trans_x, float trans_y, GameState *gameState) {
	SingleBlock block;
	block.currentState = BLOCK_DESCENDING;
	block.renderComponent.tile_x = tile_x;
	block.renderComponent.tile_y = tile_y;
	gameState->blocks[gameState->num_blocks] = block;
	opengl_init_block(gameState->num_blocks, gameState);
	translate_block(trans_x, trans_y, gameState->num_blocks, gameState);
	gameState->num_blocks++;
}

void spawn_block(TetrominoShape shape, GameState *gameState) {
	switch (shape) {
	case TETROMINO_I:
		printf("Processing I-shaped Tetromino\n");
		init_and_translate_block(0.0f, 0.0f, 0.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 0.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 0.0f, 128.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 0.0f, 192.0f, 0.0f, gameState);

		break;
	case TETROMINO_O:
		printf("Processing O-shaped Tetromino\n");
		init_and_translate_block(0.0f, 64.0f, 0.0f, 0.0f, gameState);
		init_and_translate_block(64.0, 64.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 64.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 64.0f, 64.0f, 64.0f, gameState);
		break;
	case TETROMINO_T:
		printf("Processing T-shaped Tetromino\n");
		init_and_translate_block(0.0f, 128.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(64.0, 128.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 128.0f, 64.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 128.0f, 128.0f, 64.0f, gameState);
		break;
	case TETROMINO_J:
		printf("Processing J-shaped Tetromino\n");
		init_and_translate_block(0.0f, 192.0f, 0.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 192.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 192.0f, 64.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 192.0f, 128.0f, 64.0f, gameState);
		break;
	case TETROMINO_L:
		init_and_translate_block(0.0f, 256.0f, 128.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 256.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 256.0f, 64.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 256.0f, 128.0f, 64.0f, gameState);

		printf("Processing L-shaped Tetromino\n");
		break;
	case TETROMINO_S:
		printf("Processing S-shaped Tetromino\n");
		init_and_translate_block(0.0f, 384.0f, 0.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 384.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 384.0f, 64.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 384.0f, 128.0f, 64.0f, gameState);
		break;
	case TETROMINO_Z:
		printf("Processing Z-shaped Tetromino\n");
		init_and_translate_block(0.0f, 320.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 320.0f, 128.0f, 0.0f, gameState);
		init_and_translate_block(64.0f, 320.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(64.0f, 320.0f, 64.0f, 64.0f, gameState);
		break;
	default:
		printf("Unknown Tetromino Shape\n");
	}
}


TetrominoShape get_new_random_shape(TetrominoShape current_shape) {
	TetrominoShape new_shape;
	do {
		new_shape = rand() % 7; // There are 7 different shapes
	} while (new_shape == current_shape);

	return new_shape;
}

int main(void)
{
	GLFWwindow* window;
	GameState gameState;
	gameState.action_queue = IDLE;
	gameState.num_blocks = 0;
	gameState.SCREEN_HEIGHT = 768.0f;
	gameState.SCREEN_WIDTH = 1024.0f;
	gameState.TILE_SIZE = 64.0f;
	window = opengl_create_window(&gameState);
	gameState.current_shape = TETROMINO_I;
	gameState.SHADER_PROGRAM = opengl_init_shaders();
	opengl_setup_camera(&gameState);



	float acceleration = 1.0f;
	GLuint blocks_texture = opengl_load_texture_atlas("assets/atlas.jpg");

	double lastTime = glfwGetTime();
	double deltaTime;
	int frame_counter = 0;

	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowUserPointer(window, &gameState);

	spawn_block(gameState.current_shape, &gameState);

	GLuint bg_texture = opengl_load_texture_atlas("assets/bg.jpg");
	opengl_init_bg(&gameState);

	float half_screen_height = gameState.SCREEN_HEIGHT / 2;

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		double currentTime = glfwGetTime();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		// input
		// -----
		processInput(window);

		float bound_collision_check_y_bottom = -half_screen_height;
		float bound_collision_check_x_left = -(gameState.TILE_SIZE * 5);
		float bound_collision_check_x_right = (gameState.TILE_SIZE * 5);

		if (gameState.action_queue == SPAWN_NEXT_BLOCK) {
			srand(time(NULL));
			TetrominoShape new_shape = get_new_random_shape(gameState.current_shape);
			gameState.current_shape = new_shape;
			spawn_block(gameState.current_shape, &gameState);

			gameState.action_queue = IDLE;
		}


		for (int x = gameState.num_blocks - 4; x < gameState.num_blocks; x++){
			if (gameState.blocks[x].currentState == BLOCK_DESCENDING) {
				if (gameState.blocks[x].model[3][1] - (gameState.TILE_SIZE / 2) <= bound_collision_check_y_bottom) {
					float diff = bound_collision_check_y_bottom + (gameState.TILE_SIZE / 2) - gameState.blocks[x].model[3][1];

					gameState.blocks[x].model[3][1] = bound_collision_check_y_bottom + (gameState.TILE_SIZE / 2);
					for (int j = gameState.num_blocks - 4; j < gameState.num_blocks; j++){
						if (j != x) {
							gameState.blocks[j].model[3][1] = gameState.blocks[j].model[3][1] + diff;
						}
					
						gameState.blocks[j].currentState = BLOCK_COLLIDED;
						gameState.action_queue = SPAWN_NEXT_BLOCK;
					}
				}
			}

			if (gameState.blocks[x].currentState == BLOCK_DESCENDING) {
				printf("bound collision_check %f and xPos %f \n", bound_collision_check_x_left, gameState.blocks[x].model[3][0]);

				if (gameState.blocks[x].model[3][0] - (gameState.TILE_SIZE / 2) <= bound_collision_check_x_left) {
					float diff = bound_collision_check_x_left + (gameState.TILE_SIZE / 2) - gameState.blocks[x].model[3][0];

					gameState.blocks[x].model[3][0] = bound_collision_check_x_left + (gameState.TILE_SIZE / 2);
					for (int j = gameState.num_blocks - 4; j < gameState.num_blocks; j++){
						if (j != x) {
							gameState.blocks[j].model[3][0] = gameState.blocks[j].model[3][0] + diff;
						}
					}
				}
			}



			if (gameState.blocks[x].currentState == BLOCK_DESCENDING) {
				printf("bound collision_check %f and xPos %f \n", bound_collision_check_x_right, gameState.blocks[x].model[3][0]);

				if (bound_collision_check_x_right <= gameState.blocks[x].model[3][0] - (gameState.TILE_SIZE / 2)) {
					float diff = bound_collision_check_x_right + (gameState.TILE_SIZE / 2) - gameState.blocks[x].model[3][0];

					gameState.blocks[x].model[3][0] = bound_collision_check_x_right + (gameState.TILE_SIZE / 2);
					for (int j = gameState.num_blocks - 4; j < gameState.num_blocks; j++){
						if (j != x) {
							gameState.blocks[j].model[3][0] = gameState.blocks[j].model[3][0] - diff;
						}
					}
				}
			}
		}

		if (frame_counter % 60 == 0) {
			for (int x = gameState.num_blocks - 4; x < gameState.num_blocks; x++){
				if (gameState.blocks[x].currentState == BLOCK_DESCENDING) {
					translate_block(0.0f, -64.0f, x, &gameState);
				}
			}
		}

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		opengl_set_current_texture(bg_texture);
		glBindVertexArray(gameState.bg.renderComponent.VAO);
		glUseProgram(gameState.SHADER_PROGRAM);
		GLint model_uniform_location = glGetUniformLocation(gameState.SHADER_PROGRAM, "model");
		GLint projection_uniform_location = glGetUniformLocation(gameState.SHADER_PROGRAM, "projection");

		glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, (float *)gameState.projection);
		glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, (float *)gameState.bg.model);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		opengl_set_current_texture(blocks_texture);
		for (int x = 0; x < gameState.num_blocks; x++){
			glBindVertexArray(gameState.blocks[x].renderComponent.VAO);
			opengl_translate_block(x, &gameState);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();

		frame_counter++;

	
	}

	//glDeleteVertexArrays(1, &VAO);
	//glDeleteBuffers(1, &VBO);
	//glDeleteBuffers(1, &EBO);

	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GameState* gameState = (GameState*)glfwGetWindowUserPointer(window);

	int pivot_block = 0;
	unsigned int pivot_around_center = 0;

	switch (gameState->current_shape) {
	case TETROMINO_I:
		pivot_block = 1;
		break;
	case TETROMINO_O:
		pivot_block = 1;
		pivot_around_center = 1;
		break;
	case TETROMINO_T:
		pivot_block = 2;
		break;
	case TETROMINO_J:
		pivot_block = 2;
		break;
	case TETROMINO_L:
		pivot_block = 2;
		break;
	case TETROMINO_S:
		pivot_block = 1;
		break;
	case TETROMINO_Z:
		pivot_block = 0;
		break;
	default:
		printf("current shape is unknown");
	}

	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		for (int x = gameState->num_blocks - 4; x < gameState->num_blocks; x++){
			if (gameState->blocks[x].currentState != BLOCK_COLLIDED) {
				rotate_block_around_pivot(pivot_block, x, gameState, pivot_around_center);
			}
		}
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		for (int x = gameState->num_blocks - 4; x < gameState->num_blocks; x++){
			if (gameState->blocks[x].currentState != BLOCK_COLLIDED) {
				translate_block(-64.0f, 0.0f, x, gameState);
				printf("block %d xPos %f \n", x, gameState->blocks[x].model[3][0]);
			}
		}
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		for (int x = gameState->num_blocks - 4; x < gameState->num_blocks; x++){
			if (gameState->blocks[x].currentState != BLOCK_COLLIDED) {
				translate_block(64.0f, 0.0f, x, gameState);
			}
		}
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		for (int x = gameState->num_blocks - 4; x < gameState->num_blocks; x++){
			if (gameState->blocks[x].currentState != BLOCK_COLLIDED) {
				translate_block(0.0f, -64.0f, x, gameState);
			}
		}
	}
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
