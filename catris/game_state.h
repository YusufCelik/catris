#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "config.h"

typedef struct SingleBlock SingleBlock;
typedef struct BG BG;
typedef struct TetrominoShape TetrominoShape;
typedef struct SystemActions SystemActions;
typedef struct Animations Animations;
typedef struct BlocksArray BlocksArray;

typedef struct GameState {
	BG *bg;
	TetrominoShape *current_shape;
	int num_blocks;
	mat4 projection;
	unsigned int SHADER_PROGRAM;
	SystemActions *action_queue;
	unsigned int grid[GRID_ROWS][GRID_COLS];
	BlocksArray *blocks;
	Animations *animations;
} GameState;

#endif