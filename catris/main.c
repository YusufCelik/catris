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
#include <stdbool.h>

#define GRID_ROWS 15
#define GRID_COLS 16
#define GRID_SURFFACE GRID_COLS * GRID_ROWS
#define SCREEN_WIDTH 1024.0f
#define SCREEN_HEIGHT 960.0f
#define X_MIN -(SCREEN_WIDTH / 2)
#define X_MAX (SCREEN_WIDTH / 2)
#define Y_MIN (SCREEN_HEIGHT /2)
#define Y_MAX -(SCREEN_HEIGHT /2)
#define TILE_SIZE 64.0f

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
	DO_NOT_RENDER,
	BLOCK_DESCENDING,
	BLOCK_COLLIDED
} BlockStates;

typedef enum {
	IDLE,
	PLAYER_FINISHED_MOVE,
	CHECK_ROW_COMPLETION,
	DESTROY_ROW,
	ROW_DESTROYED,
	SPAWN_NEXT_BLOCK,
	START_ROW_DESCENT_ANIMATION,
	START_ROW_REMOVAL_ANIMATION

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
	vec2 velocity;
	float alpha;
	BlockStates currentState;
	RenderComponent renderComponent;
} SingleBlock;

typedef struct BG {
	mat4 model;
	RenderComponent renderComponent;
} BG;

typedef struct BlockNode {
	SingleBlock data;
	struct BlockNode* next;
} BlockNode;

typedef struct {
	SingleBlock *array;
	size_t size;
	size_t capacity;
} DynamicArray;


typedef struct GameState GameState;

typedef struct {
	float startValue;
	float endValue;
	float currentValue;
	float stepValue;
} AnimationProperty;

typedef void(*AnimationStepCallback)(GameState* gameState, SingleBlock **animation_objects, size_t *num_animation_objects, AnimationProperty* properties, size_t numProperties);
typedef void(*AnimationCompleteCallback)(GameState* gameState, SingleBlock **animation_objects, size_t *num_animation_objects, AnimationProperty* properties, size_t numProperties);

typedef enum {
	ANIM_LINEAR,
	ANIM_EASE_OUT_ELASTIC,
	ANIM_EASE_OUT_BOUNCE
} AnimationType;

typedef struct {
	double startTime;
	double duration;
	AnimationProperty* properties;
	size_t numProperties;
	AnimationStepCallback stepCallback;
	AnimationCompleteCallback completeCallback;
	SingleBlock* animation_objects[GRID_SURFFACE];
	size_t num_animation_objects;
	AnimationType type;
} Animation;

typedef struct {
	Animation rowDownwardsAnimation;
	Animation rowDestructionAnimation;
} Animations;

typedef struct GameState {
	SingleBlock current_blocks[4];
	BG bg;
	TetrominoShape current_shape;
	int num_blocks;
	mat4 projection;
	unsigned int SHADER_PROGRAM;
	SystemActions action_queue;
	unsigned int grid[GRID_ROWS][GRID_COLS];
	BlockNode* blockList;
	DynamicArray blocks;
	Animations animations;
} GameState;


typedef struct cVec2 {
	float x;
	float y;
} cVec2;


float easeOutElastic(float progress) {
	// Simplified implementation of easeOutElastic
	float p = 0.3f;
	return pow(2, -10 * progress) * sin((progress - p / 4) * (2 * M_PI) / p) + 1;
}

float easeOutBounce(float progress) {
	// Simplified implementation of easeOutBounce
	if (progress < (1 / 2.75f)) {
		return 7.5625f * progress * progress;
	}
	else if (progress < (2 / 2.75f)) {
		progress -= 1.5f / 2.75f;
		return 7.5625f * progress * progress + 0.75f;
	}
	else if (progress < (2.5 / 2.75)) {
		progress -= 2.25f / 2.75f;
		return 7.5625f * progress * progress + 0.9375f;
	}
	else {
		progress -= 2.625f / 2.75f;
		return 7.5625f * progress * progress + 0.984375f;
	}
}

void updateAnimation(Animation* animation, GameState* gameState, double currentTime) {
	if (!animation || !gameState) return;

	double elapsed = currentTime - animation->startTime;
	if (elapsed < animation->duration) {
		float progress = elapsed / animation->duration;

		// Apply the easing function based on the animation type
		switch (animation->type) {
		case ANIM_EASE_OUT_ELASTIC:
			progress = easeOutElastic(progress);
			break;
		case ANIM_EASE_OUT_BOUNCE:
			progress = easeOutBounce(progress);
			break;
		case ANIM_LINEAR:
		default:
			// Linear progress, no change needed
			break;
		}

		for (size_t i = 0; i < animation->numProperties; i++) {
			float nextValue = animation->properties[i].startValue +
				(animation->properties[i].endValue - animation->properties[i].startValue) * progress;

			// Calculate the step value for this frame
			animation->properties[i].stepValue = nextValue - animation->properties[i].currentValue;
			animation->properties[i].currentValue = nextValue;

			printf("Elapsed: %f, Progress: %f, NextValue: %f, StepValue: %f, CurrentValue: %f \n", elapsed, progress, nextValue, animation->properties[i].stepValue, animation->properties[i].currentValue);
		}


		if (animation->stepCallback) {
			animation->stepCallback(gameState, animation->animation_objects, &animation->num_animation_objects, animation->properties, animation->numProperties);
		}
	}
	else {
		// Ensure we set the exact end values and call the complete callback
		for (size_t i = 0; i < animation->numProperties; i++) {
			animation->properties[i].currentValue = animation->properties[i].endValue;
			animation->properties[i].stepValue = animation->properties[i].endValue - animation->properties[i].currentValue;
		}

		if (animation->completeCallback) {
			animation->completeCallback(gameState, animation->animation_objects, &animation->num_animation_objects, animation->properties, animation->numProperties);
		}
	}
}

void initDynamicArray(DynamicArray *da, size_t initialCapacity) {
	da->array = (SingleBlock *)malloc(initialCapacity * sizeof(SingleBlock));
	da->size = 0;
	da->capacity = initialCapacity;
}

void addSingleBlock(DynamicArray *da, SingleBlock block) {
	if (da->size == da->capacity) {
		da->capacity *= 2;
		da->array = (SingleBlock *)realloc(da->array, da->capacity * sizeof(SingleBlock));
	}
	da->array[da->size++] = block;
}

void freeDynamicArray(DynamicArray *da) {
	free(da->array);
	da->array = NULL;
	da->size = da->capacity = 0;
}

void printBlocks(DynamicArray *blocks) {
	printf("+------+---------+---------+\n");
	printf("|  ID  |    X    |    Y    |\n");
	printf("+------+---------+---------+\n");

	for (unsigned int i = 0; i < blocks->size; i++) {
		float x = blocks->array[i].model[3][0];
		float y = blocks->array[i].model[3][1];

		printf("| %d | %7.2f | %7.2f |\n", i, x, y);
	}

	printf("+------+---------+---------+\n");

}
void removeSingleBlock(DynamicArray *da, float x_value, float y_value) {
	bool found = 0;
	x_value = roundf(x_value);
	y_value = roundf(y_value);

	for (size_t i = 0; i < da->size; i++) {
		if (roundf(da->array[i].model[3][0]) == x_value && roundf(da->array[i].model[3][1]) == y_value) {
			printf("deleting %d \n", i);
			found = 1;
			glDeleteVertexArrays(1, &da->array[i].renderComponent.VAO);
			glDeleteBuffers(1, &da->array[i].renderComponent.VBO);
			glDeleteBuffers(1, &da->array[i].renderComponent.VEO);

			// Move the last element to the current position
			da->array[i] = da->array[da->size - 1];
			da->size--;
			break; // Assuming only one block is to be removed
		}
	}

	if (found == 0) {
		printBlocks(da);
		printf("NOTHING WAS FOUND");
	}
}

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


void copySingleBlock(SingleBlock* dest, const SingleBlock* src) {
	memcpy(dest->model, src->model, sizeof(mat4)); // Copy mat4
	memcpy(&(dest->velocity), &(src->velocity), sizeof(vec2)); // Copy vec2
	dest->currentState = src->currentState;

	// Direct copy of RenderComponent as it contains simple data types
	dest->renderComponent = src->renderComponent;
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

float get_block_absolute_x(mat4 model) {
	return roundf(model[3][0] - 32.0f);
}

float get_block_absolute_y(mat4 model) {
	return roundf(model[3][1] + 32.0f);
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

void initializeGrid(unsigned int grid[GRID_ROWS][GRID_COLS]) {
	for (int row = 0; row < GRID_ROWS; ++row) {
		for (int col = 0; col < GRID_COLS; ++col) {
			if (col < 3 || col > 12) {
				grid[row][col] = 1;
			}
			else {
				grid[row][col] = 0;
			}


		}
	}
}

void initializeAnimObjectsPointerArray(SingleBlock* buffer[GRID_SURFFACE]) {
	for (int x = 0; x < GRID_SURFFACE; x++) {
		buffer[x] = NULL;
	}
}

void findGridPosition(float x, float y, int *row, int *col) {
	// Calculate the size of each cell
	y = roundf(y);
	x = roundf(x);

	int cellWidth = (X_MAX - X_MIN) / GRID_COLS;
	int cellHeight = (Y_MAX - Y_MIN) / GRID_ROWS;

	// Calculate the column and row
	*col = (x - X_MIN) / cellWidth;
	*row = (y - Y_MIN) / cellHeight;
}

void setGridValue(unsigned int grid[GRID_ROWS][GRID_COLS], float x, float y, unsigned int value) {
	int row, col;
	x = roundf(x);
	y = roundf(y);

	findGridPosition(x, y, &row, &col);

	// Check if the calculated position is within the grid bounds
	if (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS) {
		grid[row][col] = value;
	}
	else {
		printf("Error: Position (x: %d, y: %d) is out of grid bounds\n", x, y);
	}
}


void findCoordinatesFromGridPosition(int row, int col, float *x, float *y) {
	*x = roundf(*x);
	*y = roundf(*y);

	int cellWidth = (X_MAX - X_MIN) / GRID_COLS;
	int cellHeight = (Y_MAX - Y_MIN) / GRID_ROWS;

	// Calculate the x and y coordinates
	*x = X_MIN + col * cellWidth + cellWidth / 2;
	*y = Y_MIN + row * cellHeight + cellHeight / 2;
}

void transposeRowsBelowIndex(unsigned int grid[GRID_ROWS][GRID_COLS], int rowIndex) {
	if (rowIndex <= 0 || rowIndex >= GRID_ROWS) {
		printf("Invalid row index. It must be between 1 and %d.\n", GRID_ROWS - 1);
		return;
	}

	for (int row = rowIndex - 1; row >= 0; --row) {
		for (int col = 0; col < GRID_COLS; ++col) {
			grid[row + 1][col] = grid[row][col];
		}
	}
}

void printGrid(unsigned int grid[GRID_ROWS][GRID_COLS]) {
	// Print the top border
	printf("+");
	for (int col = 0; col < GRID_COLS; ++col) {
		printf("---+");
	}
	printf("\n");

	for (int row = 0; row < GRID_ROWS; ++row) {
		// Print row number
		printf("%2d |", row); // Adjust '%2d' for alignment if you have more than 99 rows

		// Print row data
		for (int col = 0; col < GRID_COLS; ++col) {
			printf(" %u |", grid[row][col]);
		}
		printf("\n");

		// Print a separator line after each row
		printf("+");
		for (int col = 0; col < GRID_COLS; ++col) {
			printf("---+");
		}
		printf("\n");
	}
}




int findHighestRowWithAllOnes(unsigned int grid[GRID_ROWS][GRID_COLS]) {
	int highestRow = -1; // Start with -1 to indicate no row found initially
	for (int row = 0; row < GRID_ROWS; ++row) {
		int allOnes = 1; // Flag to check if all values in the row are 1
		for (int col = 0; col < GRID_COLS; ++col) {
			if (grid[row][col] != 1) {
				allOnes = 0; // Set flag to 0 if any column is not 1
				break; // No need to check further columns in this row
			}
		}
		if (allOnes) {
			highestRow = row; // Update the highest row number
		}
	}

	return highestRow;
}

void opengl_init_block(SingleBlock *block, GameState *gameState) {
	float uv_coords[8];
	int texture_atlas_width = 640;
	int texture_atlas_height = 640;
	int tile_width = 64;
	int tile_height = 64;

	calculate_uv_coords(
		texture_atlas_width, texture_atlas_height,
		block->renderComponent.tile_x, block->renderComponent.tile_y, tile_width, tile_height, uv_coords);

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

	glm_mat4_identity(block->model);

	vec3 size = { 64, 64, 1.0f };
	glm_scale(&block->model, &size);

	glUseProgram(gameState->SHADER_PROGRAM);
	GLint model_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "model");
	GLint projection_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "projection");
	GLint alpha_uniform_location = glGetUniformLocation(gameState->SHADER_PROGRAM, "alpha");

	glUniform1f(alpha_uniform_location, 0.0f);
	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, (float *)gameState->projection);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, (float *)block->model);

	glGenVertexArrays(1, &block->renderComponent.VAO);
	glGenBuffers(1, &block->renderComponent.VBO);
	glGenBuffers(1, &block->renderComponent.VEO);

	glBindVertexArray(block->renderComponent.VAO);

	glBindBuffer(GL_ARRAY_BUFFER, block->renderComponent.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block->renderComponent.VEO);
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


void opengl_setup_camera(GameState *gameState) {
	float right = SCREEN_WIDTH / 2;
	float left = -right;
	float top = SCREEN_HEIGHT / 2;
	float bottom = -top;
	float near = -1.0f;    // Near clipping plane
	float far = 1.0f;      // Far clipping plane

	glm_ortho(left, right, bottom, top, near, far, gameState->projection);
}


void calculateBoundingBox(GameState *gameState, float *bbox) {
	int start_block_id = gameState->blocks.size - 4;
	float min_x = 1e6, min_y = 1e6;
	float max_x = -1e6, max_y = -1e6;

	for (int i = start_block_id; i < start_block_id + 4; ++i) {
		float x = gameState->blocks.array[i].model[3][0];
		float y = gameState->blocks.array[i].model[3][1];

		if (x < min_x) min_x = x;
		if (y < min_y) min_y = y;
		if (x > max_x) max_x = x;
		if (y > max_y) max_y = y;
	}

	bbox[0] = min_x;
	bbox[1] = min_y;
	bbox[2] = (max_x - min_x) + TILE_SIZE; // width
	bbox[3] = (max_y - min_y) + TILE_SIZE; // height
	bbox[4] = (min_x + max_x) / 2; // center_x
	bbox[5] = (min_y + max_y) / 2; // center_y
}

void rotate_block_around_pivot(unsigned int pivot_id, unsigned int block_id, GameState *gameState, unsigned int center) {
	vec3 rot = { 0.0f, 0.0f, 1.0f };
	float bbox[6]; // x, y, width, height, center_x, center_y
	calculateBoundingBox(gameState, bbox);

	unsigned int pivot_id_start_offset = gameState->blocks.size - 4;

	float move_to_x;
	float move_to_y;

	if (center == 1) {
		move_to_x = bbox[4];
		move_to_y = bbox[5];
	}
	else {
		move_to_x = gameState->blocks.array[pivot_id_start_offset + pivot_id].model[3][0];
		move_to_y = gameState->blocks.array[pivot_id_start_offset + pivot_id].model[3][1];
	}

	// Calculate the translation needed to align B with A
	float y_translation_needed = move_to_y - gameState->blocks.array[block_id].model[3][1];
	float x_translation_needed = move_to_x - gameState->blocks.array[block_id].model[3][0];

	vec3 world_vec = { x_translation_needed, y_translation_needed, 0.0f };
	mat3 rotationMatrix;
	glm_mat4_pick3(&gameState->blocks.array[block_id].model, rotationMatrix);

	mat3 inverseRotationMatrix;
	glm_mat3_inv(rotationMatrix, inverseRotationMatrix);

	vec3 localVec;
	glm_mat3_mulv(inverseRotationMatrix, world_vec, localVec);

	glm_translate(&gameState->blocks.array[block_id].model, localVec);
	glm_rotate(&gameState->blocks.array[block_id].model, glm_rad(-90.0f), rot);
	glm_vec3_negate(localVec);
	glm_translate(&gameState->blocks.array[block_id].model, localVec);

}


void translate_block(float x, float y, mat4 model) {
	mat4 translationMatrix;
	glm_translate_make(translationMatrix, (vec3){ x, y, 0.0f });
	glm_mul(translationMatrix, model, model);
}

void init_and_translate_block(unsigned int block_id, float tile_x, float tile_y, float trans_x, float trans_y, GameState *gameState) {
	SingleBlock block;
	glm_vec2_zero(block.velocity);
	block.velocity[1] = -64.0f;
	block.alpha = 1.0f;
	block.currentState = BLOCK_DESCENDING;
	block.renderComponent.tile_x = tile_x;
	block.renderComponent.tile_y = tile_y;
	addSingleBlock(&gameState->blocks, block);
	opengl_init_block(&gameState->blocks.array[gameState->blocks.size - 1], gameState);
	translate_block(trans_x - (TILE_SIZE / 2), -trans_y + (-TILE_SIZE / 2) + (SCREEN_HEIGHT / 2), gameState->blocks.array[gameState->blocks.size - 1].model);
	//printf("created block %d relative xPos %f and yPos %f \n", gameState->num_blocks, gameState->blocks[gameState->num_blocks].model[3][0], gameState->blocks[gameState->num_blocks].model[3][1]);
	//printf("created block %d absolute xPos %f and yPos %f \n", gameState->num_blocks, get_block_absolute_x(gameState->blocks[gameState->num_blocks].model), get_block_absolute_y(gameState->blocks[gameState->num_blocks].model));
}

void spawn_block(TetrominoShape shape, GameState *gameState) {
	switch (shape) {
	case TETROMINO_I:
		printf("Processing I-shaped Tetromino\n");
		init_and_translate_block(0, 0.0f, 0.0f, 0.0f, 0.0f, gameState);
		init_and_translate_block(1, 64.0f, 0.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(2, 64.0f, 0.0f, 128.0f, 0.0f, gameState);
		init_and_translate_block(3, 64.0f, 0.0f, 192.0f, 0.0f, gameState);
		break;
	case TETROMINO_O:
		printf("Processing O-shaped Tetromino\n");
		init_and_translate_block(0, 0.0f, 64.0f, 0.0f, 0.0f, gameState);
		init_and_translate_block(1, 64.0, 64.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(2, 64.0f, 64.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(3, 64.0f, 64.0f, 64.0f, 64.0f, gameState);
		break;
	case TETROMINO_T:
		printf("Processing T-shaped Tetromino\n");
		init_and_translate_block(0, 0.0f, 128.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(1, 64.0, 128.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(2, 64.0f, 128.0f, 64.0f, 64.0f, gameState);
		init_and_translate_block(3, 64.0f, 128.0f, 128.0f, 64.0f, gameState);
		break;
	case TETROMINO_J:
		printf("Processing J-shaped Tetromino\n");
		init_and_translate_block(0, 0.0f, 192.0f, 0.0f, 0.0f, gameState);
		init_and_translate_block(1, 64.0f, 192.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(2, 64.0f, 192.0f, 64.0f, 64.0f, gameState);
		init_and_translate_block(3, 64.0f, 192.0f, 128.0f, 64.0f, gameState);
		break;
	case TETROMINO_L:
		init_and_translate_block(0, 0.0f, 256.0f, 128.0f, 0.0f, gameState);
		init_and_translate_block(1, 64.0f, 256.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(2, 64.0f, 256.0f, 64.0f, 64.0f, gameState);
		init_and_translate_block(3, 64.0f, 256.0f, 128.0f, 64.0f, gameState);

		printf("Processing L-shaped Tetromino\n");
		break;
	case TETROMINO_S:
		printf("Processing S-shaped Tetromino\n");
		init_and_translate_block(0, 0.0f, 384.0f, 0.0f, 0.0f, gameState);
		init_and_translate_block(1, 64.0f, 384.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(2, 64.0f, 384.0f, 64.0f, 64.0f, gameState);
		init_and_translate_block(3, 64.0f, 384.0f, 128.0f, 64.0f, gameState);
		break;
	case TETROMINO_Z:
		printf("Processing Z-shaped Tetromino\n");
		init_and_translate_block(0, 0.0f, 320.0f, 64.0f, 0.0f, gameState);
		init_and_translate_block(1, 64.0f, 320.0f, 128.0f, 0.0f, gameState);
		init_and_translate_block(2, 64.0f, 320.0f, 0.0f, 64.0f, gameState);
		init_and_translate_block(3, 64.0f, 320.0f, 64.0f, 64.0f, gameState);
		break;
	default:
		printf("Unknown Tetromino Shape\n");
	}
}


TetrominoShape get_new_random_shape(TetrominoShape current_shape) {
	TetrominoShape new_shape;
	do {
		new_shape = rand() % 2; // There are 7 different shapes
	} while (new_shape == current_shape);

	return new_shape;
}

void animateRowsDownwardStepCallback(GameState* gameState, SingleBlock **animation_objects, size_t *num_animation_objects, AnimationProperty* properties, size_t numProperties) {
	// Implement step logic
	// Example: update the position and alpha of an object
	for (int x = 0; x < *num_animation_objects; x++) {
		SingleBlock *block = animation_objects[x];
		translate_block(0.0f, properties[0].stepValue, *block->model);
	}
}

void animateRowsDownwardCompletedCallback(GameState* gameState, SingleBlock **animation_objects, size_t *num_animation_objects, AnimationProperty* properties, size_t numProperties) {
	// Implement completion logic
	printf("Animation completed!");
	int highestRow = findHighestRowWithAllOnes(gameState->grid);

	gameState->action_queue = ROW_DESTROYED;
	for (size_t i = 0; i < numProperties; i++) {
		properties[i].currentValue = properties[i].startValue;
		properties[i].stepValue = 0;
	}

	for (int i = 0; i < *num_animation_objects; i++) {
		SingleBlock *block = animation_objects[i];

		// Calculate the nearest multiple of 64.0f
		float value = block->model[3][1];
		float nearestMultiple;
		if (value >= 0) {
			nearestMultiple = round(value / 64.0f) * 64.0f;
		}
		else {
			nearestMultiple = round(value / -64.0f) * -64.0f;
		}

		block->model[3][1] = nearestMultiple;
	}

	transposeRowsBelowIndex(gameState->grid, highestRow);
	initializeAnimObjectsPointerArray(animation_objects);
	*num_animation_objects = 0;
}


void animateRowDesctructionStepCallback(GameState* gameState, SingleBlock **animation_objects, size_t *num_animation_objects, AnimationProperty* properties, size_t numProperties) {
	// Implement step logic
	// Example: update the position and alpha of an object
	for (int x = 0; x < *num_animation_objects; x++) {
		SingleBlock *block = animation_objects[x];

		// Calculate a factor that decreases as we move through the array
		float fadeFactor = (float)(x + 0.4f) / *num_animation_objects;

		// Apply the fade factor to the alpha value
		block->alpha = properties[0].currentValue * fadeFactor;

		// Apply the fade factor and the wave effect to the translation
		translate_block(-64.0f * fadeFactor, 0.0f, block->model);
	}
}

void animateRowDestructionCompletedCallback(GameState* gameState, SingleBlock **animation_objects, size_t *num_animation_objects, AnimationProperty* properties, size_t numProperties) {
	// Implement completion logic
	printf("Animation completed!");
	int highestRow = findHighestRowWithAllOnes(gameState->grid);


	for (int x = 0; x < *num_animation_objects; x++) {
		SingleBlock *block = animation_objects[x];
		removeSingleBlock(&gameState->blocks, block->model[3][0] , block->model[3][1]);
	}

	for (size_t i = 0; i < gameState->blocks.size; i++) {
		SingleBlock *block = &gameState->blocks.array[i];
		int row, col;

		findGridPosition(block->model[3][0], block->model[3][1], &row, &col);

		if (row < highestRow) {
			// Apply the translation
			gameState->animations.rowDownwardsAnimation.num_animation_objects += 1;
			gameState->animations.rowDownwardsAnimation.animation_objects[gameState->animations.rowDownwardsAnimation.num_animation_objects - 1] = block;
		}
	}


	gameState->action_queue = START_ROW_DESCENT_ANIMATION;
	gameState->animations.rowDownwardsAnimation.startTime = glfwGetTime();
	initializeAnimObjectsPointerArray(animation_objects);
	*num_animation_objects = 0;
}

int main(void)
{
	GLFWwindow* window;
	GameState gameState;
	gameState.action_queue = IDLE;
	gameState.num_blocks = 0;
	gameState.blockList = NULL;
	gameState.blocks.array = (SingleBlock *)malloc(10 * sizeof(SingleBlock));
	gameState.blocks.size = 0;
	gameState.blocks.capacity = 10;

	window = opengl_create_window(&gameState);
	gameState.current_shape = TETROMINO_I;
	gameState.SHADER_PROGRAM = opengl_init_shaders();
	opengl_setup_camera(&gameState);

	initializeGrid(gameState.grid);

	float acceleration = 1.0f;
	GLuint blocks_texture = opengl_load_texture_atlas("assets/atlas.jpg");

	double lastTime = glfwGetTime();
	int frame_counter = 0;

	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowUserPointer(window, &gameState);

	spawn_block(gameState.current_shape, &gameState);

	GLuint bg_texture = opengl_load_texture_atlas("assets/bg.jpg");
	opengl_init_bg(&gameState);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// animations

	AnimationProperty animatingRowsDownwardsProperties[] = {
		{ 0.0f, -64.0f, 0.0f }, // Y Translation
	};

	AnimationProperty animatingRowDestructionProperties[] = {
		{ 1.0f, 0.0f, 0.0f }, // Alpha
	};

	gameState.animations.rowDownwardsAnimation.duration = 0.1f;
	gameState.animations.rowDownwardsAnimation.properties = animatingRowsDownwardsProperties;
	gameState.animations.rowDownwardsAnimation.numProperties = sizeof(animatingRowsDownwardsProperties) / sizeof(animatingRowsDownwardsProperties[0]);
	gameState.animations.rowDownwardsAnimation.stepCallback = animateRowsDownwardStepCallback;
	gameState.animations.rowDownwardsAnimation.completeCallback = animateRowsDownwardCompletedCallback;
	gameState.animations.rowDownwardsAnimation.num_animation_objects = 0;
	initializeAnimObjectsPointerArray(gameState.animations.rowDownwardsAnimation.animation_objects);
	gameState.animations.rowDownwardsAnimation.type = ANIM_EASE_OUT_ELASTIC;

	gameState.animations.rowDestructionAnimation.duration = 0.1f;
	gameState.animations.rowDestructionAnimation.properties = animatingRowDestructionProperties;
	gameState.animations.rowDestructionAnimation.numProperties = sizeof(animatingRowDestructionProperties) / sizeof(animatingRowDestructionProperties[0]);
	gameState.animations.rowDestructionAnimation.stepCallback = animateRowDesctructionStepCallback;
	gameState.animations.rowDestructionAnimation.completeCallback = animateRowDestructionCompletedCallback;
	gameState.animations.rowDestructionAnimation.num_animation_objects = 0;
	initializeAnimObjectsPointerArray(gameState.animations.rowDestructionAnimation.animation_objects);
	gameState.animations.rowDestructionAnimation.type = ANIM_EASE_OUT_BOUNCE;

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		// float alpha = (cos(totalTime / alphaDuration * 2.0f * M_PI) + 1.0f) / 2.0f;



		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		opengl_set_current_texture(bg_texture);
		glBindVertexArray(gameState.bg.renderComponent.VAO);
		glUseProgram(gameState.SHADER_PROGRAM);
		GLint model_uniform_location = glGetUniformLocation(gameState.SHADER_PROGRAM, "model");
		GLint projection_uniform_location = glGetUniformLocation(gameState.SHADER_PROGRAM, "projection");
		GLint alphaLocation = glGetUniformLocation(gameState.SHADER_PROGRAM, "alpha");

		glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, (float *)gameState.projection);
		glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, (float *)gameState.bg.model);
		glUniform1f(alphaLocation, (float) 1.0f);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		opengl_set_current_texture(blocks_texture);

		for (size_t i = 0; i < gameState.blocks.size; i++) {

			SingleBlock block = gameState.blocks.array[i];
			glBindVertexArray(block.renderComponent.VAO);
			glUniform1f(alphaLocation, (float)block.alpha);
			opengl_translate_block(block.model, &gameState);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();


		if (gameState.action_queue == START_ROW_DESCENT_ANIMATION) {
			updateAnimation(&gameState.animations.rowDownwardsAnimation, &gameState, currentTime);
		}

		if (gameState.action_queue == START_ROW_REMOVAL_ANIMATION) {
			updateAnimation(&gameState.animations.rowDestructionAnimation, &gameState, currentTime);
		}

		frame_counter++;

		if (gameState.action_queue == PLAYER_FINISHED_MOVE) {
			for (unsigned int i = gameState.blocks.size - 4; i < gameState.blocks.size; i++){
				setGridValue(gameState.grid, get_block_absolute_x(gameState.blocks.array[i].model), get_block_absolute_y(gameState.blocks.array[i].model), 1);
				printf("ADDING BLOCK %f %f \n", get_block_absolute_x(gameState.blocks.array[i].model), get_block_absolute_y(gameState.blocks.array[i].model));
			}

			printf("So many blocks %d \n", gameState.blocks.size);

			gameState.action_queue = CHECK_ROW_COMPLETION;
		}

		if (gameState.action_queue == CHECK_ROW_COMPLETION) {
			int highestRow = findHighestRowWithAllOnes(gameState.grid);
			if (highestRow != -1) {
				gameState.action_queue = DESTROY_ROW;
			}

			else {
				gameState.action_queue = SPAWN_NEXT_BLOCK;
			}
		}

		if (gameState.action_queue == DESTROY_ROW) {
			printf("DELETED ROW at %f", glfwGetTime());
			int row_to_be_removed = findHighestRowWithAllOnes(gameState.grid);

			for (size_t i = 0; i < gameState.blocks.size; i++) {
				SingleBlock *block = &gameState.blocks.array[i];
				int row, col;

				findGridPosition(block->model[3][0], block->model[3][1], &row, &col);

				if (row == row_to_be_removed) {
					gameState.animations.rowDestructionAnimation.num_animation_objects += 1;
					gameState.animations.rowDestructionAnimation.animation_objects[gameState.animations.rowDestructionAnimation.num_animation_objects - 1] = block;
				}
			}

			gameState.animations.rowDestructionAnimation.startTime = glfwGetTime();
			gameState.action_queue = START_ROW_REMOVAL_ANIMATION;

		}

		if (gameState.action_queue == ROW_DESTROYED) {
			gameState.action_queue = CHECK_ROW_COMPLETION;
		}


		if (gameState.action_queue == SPAWN_NEXT_BLOCK) {
			srand(time(NULL));
			TetrominoShape new_shape = get_new_random_shape(gameState.current_shape);
			gameState.current_shape = new_shape;
			spawn_block(gameState.current_shape, &gameState);
			gameState.action_queue = IDLE;
		}

		// input
		// -----
		processInput(window);
	}

	free(gameState.blocks.array);
	gameState.blocks.array = NULL;
	gameState.blocks.size = gameState.blocks.capacity = 0;

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

	if (action == GLFW_RELEASE) {
		return;
	}

	if (gameState->action_queue == PLAYER_FINISHED_MOVE || gameState->action_queue == START_ROW_DESCENT_ANIMATION || gameState->action_queue == START_ROW_REMOVAL_ANIMATION) {
		return;
	}



	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
		printGrid(gameState->grid);

		BlockNode* current = gameState->blockList;
		while (current != NULL) {
			float x, y;
			x = current->data.model[3][0];
			y = current->data.model[3][1];
			printf("x: %f y: %f \n", x, y);
			current = current->next;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		for (unsigned int i = gameState->blocks.size - 4; i < gameState->blocks.size; i++){
			if (gameState->blocks.array[i].currentState != BLOCK_COLLIDED) {
				printf("before: %f \n", gameState->blocks.array[i].model[3][1]);
				rotate_block_around_pivot(pivot_block, i, gameState, pivot_around_center);
				printf("after: %f \n", gameState->blocks.array[i].model[3][1]);
			}
		}
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		bool can_move_left = 1;

		for (unsigned int i = gameState->blocks.size - 4; i < gameState->blocks.size; i++){
			unsigned int row, col;
			float x, y;
			x = get_block_absolute_x(gameState->blocks.array[i].model);
			y = get_block_absolute_y(gameState->blocks.array[i].model) - (TILE_SIZE - 1.0f);

			findGridPosition(x, y, &row, &col);

			if (gameState->grid[row][col - 1] == 1) {
				can_move_left = 0;
			}
		}

		if (can_move_left == 1) {
			for (unsigned int i = gameState->blocks.size - 4; i < gameState->blocks.size; i++){
				translate_block(-64.0f, 0.0f, gameState->blocks.array[i].model);
			}
		}
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		bool can_move_right = 1;

		for (unsigned int i = gameState->blocks.size - 4; i < gameState->blocks.size; i++){
			unsigned int row1, row2, col;
			float x, y1, y2;
			x = get_block_absolute_x(gameState->blocks.array[i].model);
			y1 = get_block_absolute_y(gameState->blocks.array[i].model);
			y2 = get_block_absolute_y(gameState->blocks.array[i].model) - (TILE_SIZE - 1.0f);

			findGridPosition(x, y1, &row1, &col);
			findGridPosition(x, y2, &row2, &col);

			printf("current block id %d \n", i);
			printf("current row %d, col %d \n", row1, col);
			printf("current grid block %d, next grid block %d \n", gameState->grid[row1][col], gameState->grid[row1][col + 1]);

			if (gameState->grid[row1][col + 1] == 1) {
				can_move_right = 0;
			}

			if (gameState->grid[row2][col + 1] == 1) {
				can_move_right = 0;
			}
		}

		if (can_move_right == 1) {
			for (unsigned int i = gameState->blocks.size - 4; i < gameState->blocks.size; i++){
				translate_block(64.0f, 0.0f, gameState->blocks.array[i].model);
			}
		}
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		printf("Pressing down \n");
		bool can_move_down = 1;

		for (unsigned int i = gameState->blocks.size - 4; i < gameState->blocks.size; i++){
			unsigned int row, col;
			float x, y;
			x = get_block_absolute_x(gameState->blocks.array[i].model);
			y = get_block_absolute_y(gameState->blocks.array[i].model);

			findGridPosition(x, y, &row, &col);

			//printf("current block id %d \n", i);
			//printf("current row %d, col %d \n", row, col);
			//printf("current grid block %d, next grid block %d \n", gameState->grid[row][col], gameState->grid[row + 1][col]);

			if (gameState->grid[row + 1][col] == 1) {
				can_move_down = 0;
			}

			if (row + 1 == GRID_ROWS) {
				can_move_down = 0;
			}
		}

		if (can_move_down == 1) {
			for (unsigned int i = gameState->blocks.size - 4; i < gameState->blocks.size; i++){
				translate_block(0.0f, -64.0f, gameState->blocks.array[i].model);
			}
		}

		else {
			printf("CAN'T MOVE DOWN ANYMORE! \n");

			gameState->action_queue = PLAYER_FINISHED_MOVE;

			for (unsigned int i = gameState->blocks.size - 4; i < gameState->blocks.size; i++){
				if (gameState->blocks.array[i].currentState != BLOCK_COLLIDED) {
					gameState->blocks.array[i].currentState = BLOCK_COLLIDED;
					gameState->blocks.array[i].velocity[1] = 0;
				}
			}

			printGrid(gameState->grid);

			printf("DONE MOVEMENT \n");
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