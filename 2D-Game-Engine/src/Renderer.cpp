#include "stdafx.h"
#include "Renderer.h"

#include "Window.h"

static const Vertex quadVertices[4] =
{
	glm::vec2(-0.5f, -0.5f), glm::vec2(0.0f, 0.0f),
	glm::vec2(0.5f, -0.5f), glm::vec2(1.0f, 0.0f),
	glm::vec2(-0.5f, 0.5f), glm::vec2(0.0f, 1.0f),
	glm::vec2(0.5f, 0.5f), glm::vec2(1.0f, 1.0f)
};

static const unsigned char quadIndices[6] =
{
	0, 1, 2,
	2, 1, 3
};

Renderer::Renderer()
{
	// Create the VAO to use for most sprites
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	// Vertex buffer
	glGenBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

	// Index buffer
	glGenBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

	// Populate the vertex and index buffers. These are static, since all quads are the same.
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, quadVertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned char) * 6, quadIndices, GL_STATIC_DRAW);

	// Setup vertex attribute pointers
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));

	// We don't need these since we're only working with 2D
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// Enable blending for transparent sprites
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Cornflower blue
	glClearColor(0.392157f, 0.584314f, 0.929412f, 1);

	AssetManager::getInstance()->loadShader("defaultShader", "shaders/vertexShader.glsl", "shaders/fragmentShader.glsl");
}

Renderer::~Renderer()
{
	glDeleteBuffers(1, &m_vertexBuffer);
	glDeleteBuffers(1, &m_indexBuffer);
	glDeleteVertexArrays(1, &m_vao);
}

unsigned int Renderer::getVertexBufferID() const
{
	return m_vertexBuffer;
}

unsigned int Renderer::getIndexBufferID() const
{
	return m_indexBuffer;
}

void Renderer::render(const Camera& camera, const Sprite* sprites, size_t spriteCount)
{
	const glm::mat4& projection = camera.getProjectionMatrix();
	const glm::mat4& view = camera.getViewMatrix();

	glBindVertexArray(m_vao);

	glUniform4f(5, 1.0f, 1.0f, 1.0f, 1.0f);

	for (size_t i = 0; i < spriteCount; i++)
	{
		// Don't render sprites with no shader
		if (sprites[i].renderData.shaderID == 0) continue;

		// Use the shader
		glUseProgram(sprites[i].renderData.shaderID);

		// Build the sprite's world matrix
		glm::mat4 world = glm::mat4();
		world = glm::translate(world, glm::vec3(sprites[i].transform.position, 0.0f));
		world = glm::scale(world, glm::vec3(sprites[i].transform.size, 0.0f));

		// Upload uniforms
		glUniformMatrix4fv(0, 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(2, 1, GL_FALSE, &world[0][0]);

		glm::vec2 uvOffsetScaleFactor = sprites[i].renderData.texture.dimensions / (sprites[i].renderData.tileDimensions - glm::vec2(TEXTURE_SHRINK_FACTOR));
		glUniform2fv(4, 1, &uvOffsetScaleFactor[0]);
		glUniform2fv(6, 1, &sprites[i].renderData.uvOffsets[sprites[i].renderData.uvOffsetIndex][0]);

		// Bind the texture
		glBindTexture(GL_TEXTURE_2D, sprites[i].renderData.texture.id);

		// Draw the quad
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, (void*)0);
	}
}

void Renderer::render(const Camera& camera, const Terrain& terrain)
{
	const glm::mat4& projection = camera.getProjectionMatrix();
	const glm::mat4& view = camera.getViewMatrix();

	const Chunk* chunks = terrain.getVisibleChunks();

	AssetManager* assetManager = AssetManager::getInstance();
	unsigned int terrainShaderID = assetManager->getShader("terrainShader");
	unsigned int blockSpritesheet = assetManager->getTexture("blockSpritesheet");

	// Use the shader
	glUseProgram(terrainShaderID);

	// Upload the matrices
	glUniformMatrix4fv(0, 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);

	glUniform4f(5, 1.0f, 1.0f, 1.0f, 1.0f);

	// Bind the texture
	glBindTexture(GL_TEXTURE_2D, blockSpritesheet);

	for (size_t i = 0; i < CHUNK_VIEW_SIZE; i++)
	{
		// Bind the chunk's VAO
		glBindVertexArray(chunks[i].vao);

		unsigned int blockCountSum = 0;
		for (size_t j = 1; j < BLOCK_COUNT; j++)
		{
			// Add to the block sum so the instance offset is consistent
			blockCountSum += chunks[i].blockCount[j - 1];

			// Don't render this block type if there aren't any present in the chunk
			if (chunks[i].blockCount[j] == 0) continue;

			// Get the pointer to the start of the blocks we want to render
			// Offset the blocks array by the previous block's count
			const Block* blocks = chunks[i].blocks + blockCountSum;

			// Upload uniforms
			glm::vec2 uvOffsetScaleFactor = blocks[0].sprite.renderData.texture.dimensions / (blocks[0].sprite.renderData.tileDimensions - glm::vec2(TEXTURE_SHRINK_FACTOR));
			glUniform2fv(4, 1, &uvOffsetScaleFactor[0]);
			glUniform2fv(6, MAX_ANIMATION_LENGTH, &blocks[0].sprite.renderData.uvOffsets[0][0]);

			// Draw the block using instanced rendering
			glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, (void*)0, chunks[i].blockCount[j], blockCountSum);
		}
	}
}
