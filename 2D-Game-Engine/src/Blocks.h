#pragma once

#include "AssetManager.h"
#include "Sprite.h"

#define BLOCK_SIZE 16 // The size of a block in pixels
#define BLOCK_COUNT 2 // The number of different blocks

enum Blocks
{
	AIR,
	DIRT
};

class BlockContainer
{
public:
	BlockContainer();
	~BlockContainer();

	static const RenderData& getBlock(Blocks name);

private:
	static std::unordered_map<Blocks, RenderData> blocks;
};