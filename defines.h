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

struct LIGHT {
	std::string name;

	GW::MATH::GMATRIXF worldMatrix;
};

struct INPUT_CONTROLLER
{
	float INPUT_spc = 0;
	float INPUT_lShift = 0;
	float INPUT_lCtrl = 0;

	float INPUT_w = 0;
	float INPUT_s = 0;
	float INPUT_d = 0;
	float INPUT_a = 0;
	float INPUT_o = 0;

	float CINPUT_rTrigger = 0;
	float CINPUT_lTrigger = 0;
	float CINPUT_lStick_y = 0;
	float CINPUT_lStick_x = 0;
	float CINPUT_rStick_y = 0;
	float CINPUT_rStick_x = 0;
	float CINPUT_start = 0;

	float INPUT_mouse_x = 0;
	float INPUT_mouse_y = 0;
};