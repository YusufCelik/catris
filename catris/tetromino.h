#ifndef TETROMINO_H
#define TETROMINO_H

#include <cglm/struct.h>
#include "opengl.h"

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

typedef struct {
	SingleBlock *array;
	size_t size;
	size_t capacity;
} BlocksArray;


void initBlocksArray(BlocksArray *da, size_t initialCapacity);

void addSingleBlock(BlocksArray *da, SingleBlock block);

void freeBlocksArray(BlocksArray *da);

void removeSingleBlock(BlocksArray *da, float x_value, float y_value);

#endif