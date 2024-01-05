#include "tetromino.h"
#include "logging.h"

void printBlocks(BlocksArray *blocks) {
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
