// TODO: Part 1b
#include "GW_Defines.h"
#include "XTime.h"
#include "Assets/FSLogo/FSLogo.h"
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include "Gateware/Gateware.h"
#include "vulkan/vulkan_core.h"
#include <vector>


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
		GW::MATH::GMATRIXF ViewMatrix, ProjectionMatrix;

		GW::MATH::GMATRIXF matricies[MAX_SUBMESH_PER_DRAW];
		OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	};

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;

	XTime timer;

	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	// TODO: Part 1g
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;
	// TODO: Part 2c
	std::vector<VkBuffer> SB_Handle;
	std::vector<VkDeviceMemory> SB_Data;

	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	// TODO: Part 2e
	VkDescriptorSetLayout SB_SetLayout;
	// TODO: Part 2f
	VkDescriptorPool SB_DescriptorPool;
	// TODO: Part 2g
	std::vector<VkDescriptorSet> SB_DescriptorSet;
	// TODO: Part 4f

// TODO: Part 2a
	GW::MATH::GMatrix PROXY_matrix;
	GW::MATH::GVector PROXY_vector;
	GW::INPUT::GInput PROXY_input;
	GW::INPUT::GController PROXY_controller;

	GW::MATH::GMATRIXF MATRIX_World;
	GW::MATH::GMATRIXF MATRIX_View;
	GW::MATH::GMATRIXF MATRIX_Projection;
	GW::MATH::GVECTORF VECTOR_Light_Direction;
	GW::MATH::GVECTORF VECTOR_Light_Color;
	GW::MATH::GVECTORF VECTOR_Ambient;

	// TODO: Part 2b
	SHADER_MODEL_DATA shader_model_data;

	unsigned int max_active_frames;
	// TODO: Part 4g
public:

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk);
	void Render();
	void UpdateCamera();
	void SignalTimer();

private:
	void CleanUp();	
	std::string ShaderAsString(const char* shaderFilePath);
	void InitContent();
	void InitShader();
	void InitPipeline(unsigned int width, unsigned int height);
	void Shutdown();
};
