// TODO: Part 1b
// With what we want & what we don't defined we can include the API
// Simple basecode showing how to create a window and attatch a vulkansurface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
#define GATEWARE_ENABLE_MATH // Enables all Math
#define GATEWARE_ENABLE_AUDIO // Enables all audio
// TODO: Part 3a
#define GATEWARE_ENABLE_INPUT
// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX11SURFACE // we have another template for this
#define GATEWARE_DISABLE_GDIRECTX12SURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE // we have another template for this
#include "XTime.h"
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include "Gateware/Gateware.h"
#include "vulkan/vulkan_core.h"
#include "defines.h"
#include <vector>
#include <iostream>
#include <fstream>


#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif

// Creation, Rendering & Cleanup
class Renderer
{
	// TODO: Part 2b
#define MAX_SUBMESH_PER_DRAW 1024

	struct SHADER_MODEL_DATA
	{
		GW::MATH::GVECTORF SunDirection, SunColor, SunAmbient, CamPos;
		GW::MATH::GMATRIXF ViewMatrix[2], ProjectionMatrix;

		GW::MATH::GMATRIXF matricies[MAX_SUBMESH_PER_DRAW];
		H2B::ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	};

	struct PUSH_CONSTANTS
	{
		unsigned model_Index, material_Index, camera_Index;
		int padding[29] = {};
	};

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;

	XTime timer;

	//Save all the models into this list
	std::vector<GAMEOBJECT> List_Of_Game_Objects;

	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	//VkBuffer vertexHandle = nullptr;
	//VkDeviceMemory vertexData = nullptr;
	//// TODO: Part 1g
	//VkBuffer indexHandle = nullptr;
	//VkDeviceMemory indexData = nullptr;
	//// TODO: Part 2c
	//std::vector<VkBuffer> SB_Handle;
	//std::vector<VkDeviceMemory> SB_Data;

	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	// TODO: Part 2e
	//VkDescriptorSetLayout SB_SetLayout;
	// TODO: Part 2f
	/*VkDescriptorPool SB_DescriptorPool;*/
	// TODO: Part 2g
	//std::vector<VkDescriptorSet> SB_DescriptorSet;
	// TODO: Part 4f

// TODO: Part 2a
	GW::MATH::GMatrix PROXY_matrix;
	GW::MATH::GVector PROXY_vector;
	GW::INPUT::GInput PROXY_input;
	GW::INPUT::GController PROXY_controller;
	GW::AUDIO::GAudio PROXY_audio;
	GW::AUDIO::GMusic PROXY_music;
	GW::AUDIO::GSound PROXY_sound;

	GW::MATH::GMATRIXF MATRIX_World;
	std::vector <GW::MATH::GMATRIXF> MATRIX_View;
	GW::MATH::GMATRIXF MATRIX_Projection;
	GW::MATH::GVECTORF VECTOR_Light_Direction;
	GW::MATH::GVECTORF VECTOR_Light_Color;
	GW::MATH::GVECTORF VECTOR_Ambient;

	// TODO: Part 2b
	SHADER_MODEL_DATA shader_model_data;
	PUSH_CONSTANTS push_constants;
	INPUT_CONTROLLER input_controller;

	float volume = 1.0f;
	unsigned int max_active_frames;
	// TODO: Part 4g
public:

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk);
	void Render();
	void UpdateCamera(INPUT_CONTROLLER input);
	void SignalTimer();
	INPUT_CONTROLLER GetInput();
	void AudioController(INPUT_CONTROLLER input);
	void LoadNewLevel(INPUT_CONTROLLER input);

private:
	void CleanUp();	
	std::string ShaderAsString(const char* shaderFilePath);
	void InitContent();
	void InitShader();
	void InitPipeline(unsigned int width, unsigned int height);
	void Shutdown();
	void ReadGameLevelFile(const char* file);
	void LoadGameLevel();
	std::string FileOpen();
};
