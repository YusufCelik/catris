#include <stdio.h>
#include <stdbool.h>
#include "tetromino.h"
#include "logging.h"

void initBlocksArray(BlocksArray *da, size_t initialCapacity) {
	da->array = (SingleBlock *)malloc(initialCapacity * sizeof(SingleBlock));
	da->size = 0;
	da->capacity = initialCapacity;
}

void addSingleBlock(BlocksArray *da, SingleBlock block) {
	if (da->size == da->capacity) {
		da->capacity *= 2;
		da->array = (SingleBlock *)realloc(da->array, da->capacity * sizeof(SingleBlock));
	}
	da->array[da->size++] = block;
}

void freeBlocksArray(BlocksArray *da) {
	free(da->array);
	da->array = NULL;
	da->size = da->capacity = 0;
}

void removeSingleBlock(BlocksArray *da, float x_value, float y_value) {
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