#include <stdlib.h>
#include <stdio.h>

#include "app_context.h"

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

void load_textures(ApplicationContext *context) {
	unsigned int backgroundTextureId = opengl_load_texture_atlas("assets/bg.jpg");
	unsigned int atlasTextureId = opengl_load_texture_atlas("assets/atlas.jpg");

	context->textureManager->textureIds = create_table();
	insert(context->textureManager->textureIds, "bg", backgroundTextureId);
	insert(context->textureManager->textureIds, "atlas", atlasTextureId);

	context->textureManager->textureWidths = create_table();
	insert(context->textureManager->textureWidths, "bg", 1024);
	insert(context->textureManager->textureWidths, "atlas", 640);

	context->textureManager->textureHeights = create_table();
	insert(context->textureManager->textureHeights, "bg", 960);
	insert(context->textureManager->textureHeights, "atlas", 640);
}

unsigned int get_texture_id_from_path(ApplicationContext * context, char *path) {
	unsigned int result = search(context->textureManager->textureIds, path);

	return result;
}

void get_texture_dimensions_from_id(ApplicationContext * context, char *path, unsigned int *dimensions) {
	dimensions[0] = search(context->textureManager->textureWidths, path);
	dimensions[1] = search(context->textureManager->textureHeights, path);
}