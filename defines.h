#pragma once
#include "h2bParser.h"
#include "Gateware/Gateware.h"
#include <vector>;

struct GAMEOBJECT {
	
#pragma region Storage Buffer Stuff

	std::vector<VkBuffer> SB_Handle;
	std::vector<VkDeviceMemory> SB_Data;

	VkDescriptorSetLayout SB_SetLayout;

	VkDescriptorPool SB_DescriptorPool;
	
	std::vector<VkDescriptorSet> SB_DescriptorSet;

#pragma endregion

	std::string name;

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