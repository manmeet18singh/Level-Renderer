#pragma once
#include "h2bParser.h"
#include "Gateware/Gateware.h"
#include <vector>;

struct GAMEOBJECT {
	
#pragma region Storage Buffer Stuff

	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	// TODO: Part 1g
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;

	std::vector<VkBuffer> SB_Handle;
	std::vector<VkDeviceMemory> SB_Data;

	VkDescriptorSetLayout SB_SetLayout;

	VkDescriptorPool SB_DescriptorPool;
	
	std::vector<VkDescriptorSet> SB_DescriptorSet;

#pragma endregion

	std::string name;

	unsigned matOffset;

	unsigned vertexCount;
	unsigned indexCount;
	unsigned materialCount;
	unsigned meshCount;

	GW::MATH::GMATRIXF worldMatrix;
	std::vector<H2B::VERTEX> vertices;
	std::vector<unsigned> indices;
	std::vector<H2B::MATERIAL> materials;
	std::vector<H2B::BATCH> batches;
	std::vector<H2B::MESH> meshes;
};