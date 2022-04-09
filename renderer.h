// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include "XTime.h"
#include <vector>
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif
// Simple Vertex Shader
const char* vertexShaderSource = R"(
// an ultra simple hlsl vertex shader

// TODO: Part 2b
[[vk::push_constant]] 
cbuffer SHADER_VARS
{
    matrix World;
	matrix View;
}

// TODO: Part 2f, Part 3b
// TODO: Part 1c

struct VS_INPUT
{
    float4 Pos : POSITION;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
};

PS_INPUT main(VS_INPUT input)
{

	// TODO: Part 2d
	// TODO: Part 2f, Part 3b
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = mul(World, input.Pos);
    output.Pos = mul(View, output.Pos);
	
    //output.Pos = mul(output.Pos, Projection);
    //output.Norm = mul(input.Norm, (float3x3) World);
    //output.Tex = input.Tex;
    return output;
}
)";
// Simple Pixel Shader
const char* pixelShaderSource = R"(
// an ultra simple hlsl pixel shader
float4 main() : SV_TARGET 
{	
	return float4(0.25f, 0.75f, 0.75f, 0); // TODO: Part	
}
)";
// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	// TODO: Part 4a
	GW::INPUT::GInput PROXY_input;
	GW::INPUT::GController PROXY_controller;
	// TODO: Part 2a
	GW::MATH::GMATRIXF MATRIX_World;
	GW::MATH::GMatrix PROXY_matrix;
	// TODO: Part 3d
	GW::MATH::GMATRIXF MATRIX_Wall_North;
	GW::MATH::GMATRIXF MATRIX_Wall_East;
	GW::MATH::GMATRIXF MATRIX_Wall_South;
	GW::MATH::GMATRIXF MATRIX_Wall_West;
	GW::MATH::GMATRIXF MATRIX_Ceiling;	
	GW::MATH::GMATRIXF MATRIX_Floor;
	// TODO: Part 2e
	GW::MATH::GMATRIXF MATRIX_View;
	// TODO: Part 3a
	GW::MATH::GMATRIXF MATRIX_Projection;
	// 
	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
public:
	// TODO: Part 1c

	XTime timer;

	struct ColorVertex
	{
		GW::MATH::GVECTORF pos;
		GW::MATH::GVECTORF clr;
	};

	std::vector<ColorVertex> vertexList;

	void AddLine(GW::MATH::GVECTORF pos1, GW::MATH::GVECTORF pos2, GW::MATH::GVECTORF color)
	{
		vertexList.push_back({ pos1, color });
		vertexList.push_back({ pos2, color });
	}

	void CreateGrid() {
		float size = 1.0f;
		float spacing = 0.04f;
		int lineCount = (int)(size / spacing);

		float x = -size / 2.0f;
		float y = -size / 2.0f;
		float xS = spacing, yS = spacing;

		y = -size / 2.0f;
		for (int i = 0; i <= lineCount; i++)
		{
			AddLine({ x, y, 0, 1 }, { x + size, y, 0, 1 }, { 1.0f, 1.0f, 1.0f, 1.0f });
			y += yS;
		}
		y = -size / 2.0f;
		x = -size / 2.0f;
		for (int i = 0; i <= lineCount; i++)
		{
			AddLine({ x, y, 0, 1 }, { x, y + size, 0, 1 }, { 1.0f, 1.0f, 1.0f, 1.0f });
			x += xS;
		}
	}

	// TODO: Part 2b	
	struct SHADER_VARS
	{
		GW::MATH::GMATRIXF world;
		GW::MATH::GMATRIXF view_projection;
	};

	SHADER_VARS shader_vars;

	// TODO: Part 2f
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		
		// TODO: Part 4a
		timer = XTime();
		PROXY_input.Create(win);
		//PROXY_controller.Create();
	
#pragma region WORLD VIEW PROJ MATRIX

		// TODO: Part 2a
		PROXY_matrix.Create();

		// TODO: Part 3d		
		
		//Floor Matrix
		PROXY_matrix.RotateXGlobalF(GW::MATH::GIdentityMatrixF, 1.5708, MATRIX_Floor);
		PROXY_matrix.TranslateGlobalF(MATRIX_Floor, GW::MATH::GVECTORF{ 0.0f, 0.5f, 0.0f , 0.0f }, MATRIX_Floor);

		//Ceiling Matrix
		PROXY_matrix.RotateXGlobalF(GW::MATH::GIdentityMatrixF, 1.5708, MATRIX_Ceiling);
		PROXY_matrix.TranslateGlobalF(MATRIX_Ceiling, GW::MATH::GVECTORF{ 0.0f, -0.5f, 0.0f , 0.0f }, MATRIX_Ceiling);

		//North Wall Matrix
		PROXY_matrix.RotateYGlobalF(GW::MATH::GIdentityMatrixF, 1.5708, MATRIX_Wall_North);
		PROXY_matrix.TranslateGlobalF(MATRIX_Wall_North, GW::MATH::GVECTORF{ 0.5f, 0.0f, 0.0f, 0.0f }, MATRIX_Wall_North);

		//South Wall Matrix
		PROXY_matrix.RotateYGlobalF(GW::MATH::GIdentityMatrixF, 1.5708, MATRIX_Wall_South);
		PROXY_matrix.TranslateGlobalF(MATRIX_Wall_South, GW::MATH::GVECTORF{ -0.5f, 0.0f, 0.0f, 0.0f }, MATRIX_Wall_South);

		//East Wall Matrix
		PROXY_matrix.RotateYGlobalF(GW::MATH::GIdentityMatrixF, 3.14159, MATRIX_Wall_East);
		PROXY_matrix.TranslateGlobalF(MATRIX_Wall_East, GW::MATH::GVECTORF{ 0.0f, 0.0f, 0.5f, 0.0f }, MATRIX_Wall_East);

		////West Wall Matrix
		PROXY_matrix.RotateYGlobalF(GW::MATH::GIdentityMatrixF, 3.14159, MATRIX_Wall_West);
		PROXY_matrix.TranslateGlobalF(MATRIX_Wall_West, GW::MATH::GVECTORF{ 0.0f, 0.0f, -0.5f, 0.0f }, MATRIX_Wall_West);
		 //TODO: Part 2e

#pragma endregion

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		//// TODO: Part 1b
		//// TODO: Part 1c
		//// Create Vertex Buffer
		// TODO: Part 1d
		CreateGrid();
		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		GvkHelper::create_buffer(physicalDevice, device, sizeof(vertexList[0]) * vertexList.size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, vertexList.data(), sizeof(vertexList[0]) * vertexList.size());

		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		// TODO: Part 3C
		shaderc_compile_options_set_invert_y(options, false);
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource, strlen(vertexShaderSource),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, pixelShaderSource, strlen(pixelShaderSource),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &pixelShader);
		shaderc_result_release(result); // done
		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = pixelShader;
		stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		assembly_create_info.primitiveRestartEnable = false;
		// Vertex Input State
		// TODO: Part 1c
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(ColorVertex);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		// TODO: Part 1c
		VkVertexInputAttributeDescription vertex_attribute_description[] = {
			{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 } //uv, normal, etc....
			//{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 16 }
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 1;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport = {
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};
		VkRect2D scissor = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount = 1;
		viewport_create_info.pViewports = &viewport;
		viewport_create_info.scissorCount = 1;
		viewport_create_info.pScissors = &scissor;
		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth = 1.0f;
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable = VK_FALSE;
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;
		// Multisampling State
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.0f;
		multisample_create_info.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info.alphaToOneEnable = VK_FALSE;
		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info.minDepthBounds = 0.0f;
		depth_stencil_create_info.maxDepthBounds = 1.0f;
		depth_stencil_create_info.stencilTestEnable = VK_FALSE;
		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0] = 0.0f;
		color_blend_create_info.blendConstants[1] = 0.0f;
		color_blend_create_info.blendConstants[2] = 0.0f;
		color_blend_create_info.blendConstants[3] = 0.0f;
		// Dynamic State 
		VkDynamicState dynamic_state[2] = {
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_state;
		// TODO: Part 2c
		VkPushConstantRange constant_range = {};
		constant_range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		constant_range.offset = 0;
		constant_range.size = sizeof(SHADER_VARS);


		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 0;
		pipeline_layout_create_info.pSetLayouts = VK_NULL_HANDLE;
		pipeline_layout_create_info.pushConstantRangeCount = 1; // TODO: Part 2d 
		pipeline_layout_create_info.pPushConstantRanges = &constant_range; // TODO: Part 2d
		vkCreatePipelineLayout(device, &pipeline_layout_create_info,
			nullptr, &pipelineLayout);
		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
			&pipeline_create_info, nullptr, &pipeline);

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
			});
	}
	void Render()
	{
		// grab the current Vulkan commandBuffer
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		// what is the current client area dimensions?
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// setup the pipeline's dynamic settings
		VkViewport viewport = {
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};
		VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		
		// TODO: Part 3a
		GW::MATH::GVECTORF Eye = { 0.25f, -0.125, -0.25f };
		GW::MATH::GVECTORF At = { 0.0f, 0.0f, 0.0f };
		GW::MATH::GVECTORF Up = { 0.0f, 1.0f, 0.0f };

		PROXY_matrix.LookAtLHF(Eye, At, Up, MATRIX_View);

		float aspect;
		vlk.GetAspectRatio(aspect);

		PROXY_matrix.ProjectionDirectXLHF(1.13446f, aspect, 0.1f, 100.0f, MATRIX_Projection);
		
		// TODO: Part 3b
		//Combine view and projection mat and save into shader var struct
		PROXY_matrix.MultiplyMatrixF(MATRIX_View, MATRIX_Projection, shader_vars.view_projection);

		// TODO: Part 2b
			// TODO: Part 2f, Part 3b
		// TODO: Part 2d

		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);

		// TODO: Part 3e
#pragma region Draw Grid Cube

		shader_vars.world = MATRIX_Floor;
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(SHADER_VARS), &shader_vars);
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertexList.size()), 1, 0, 0);

		shader_vars.world = MATRIX_Ceiling;
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(SHADER_VARS), &shader_vars);
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertexList.size()), 1, 0, 0);

		shader_vars.world = MATRIX_Wall_North;
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(SHADER_VARS), &shader_vars);
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertexList.size()), 1, 0, 0);

		shader_vars.world = MATRIX_Wall_South;
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(SHADER_VARS), &shader_vars);
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertexList.size()), 1, 0, 0);

		shader_vars.world = MATRIX_Wall_East;
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(SHADER_VARS), &shader_vars);
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertexList.size()), 1, 0, 0); 

		shader_vars.world = MATRIX_Wall_West;
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(SHADER_VARS), &shader_vars);
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertexList.size()), 1, 0, 0); 

#pragma endregion

	}
	// TODO: Part 4b
	void UpdateCamera() {

		// TODO: Part 4c
		GW::MATH::GMATRIXF Camera;

		PROXY_matrix.InverseF(shader_vars.view_projection, Camera);

		// TODO: Part 4d
		float KEY_spc = 0;
		float KEY_lShift = 0;
		float KEY_rTrigger = 0;
		float KEY_lTrigger = 0;

		const float Camera_Speed = 0.3f;
		float Seconds_Passed_Since_Last_Frame = timer.Delta();


		PROXY_input.GetState(G_KEY_SPACE, KEY_spc);
		PROXY_input.GetState(G_KEY_LEFTSHIFT, KEY_lShift);

		PROXY_input.GetState(G_RIGHT_TRIGGER_AXIS, KEY_rTrigger);
		PROXY_input.GetState(G_LEFT_TRIGGER_AXIS, KEY_lTrigger);

		float Total_Y_Change = KEY_spc - KEY_lShift + KEY_rTrigger - KEY_lTrigger;

		Camera.row4.y += Total_Y_Change * Camera_Speed * Seconds_Passed_Since_Last_Frame;


		// TODO: Part 4e
		// TODO: Part 4f
		// TODO: Part 4g
		
		// TODO: Part 4c		
		PROXY_matrix.InverseF(shader_vars.view_projection, shader_vars.view_projection);

	}

private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
	}
};
