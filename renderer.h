// minimalistic code to draw a single triangle, this is not part of the API.
// TODO: Part 1b
#include "Assets/FSLogo.h"
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
	#pragma comment(lib, "shaderc_combined.lib") 
#endif
//// Simple Vertex Shader
//const char* vertexShaderSource = R"(
//// TODO: 2i
//// an ultra simple hlsl vertex shader
//// TODO: Part 2b
//// TODO: Part 4g
//// TODO: Part 2i
//// TODO: Part 3e
//// TODO: Part 4a
//// TODO: Part 1f
//
////[[vk::push_constant]] 
////cbuffer SHADER_VARS
////{
////    matrix World;
////    matrix View;
////    //matrix Projection;
////}
//
//struct VS_INPUT
//{
//    float4 Pos : POSITION;
//    float3 Norm : NORMAL;
//    float2 Tex : TEXCOORD0;
//};
//
//struct PS_INPUT
//{
//    float4 Pos : SV_POSITION;
//    float3 Norm : NORMAL;
//    float2 Tex : TEXCOORD1;
//};
//// TODO: Part 4b
//
//PS_INPUT main(VS_INPUT input)
//{
//    PS_INPUT output = (PS_INPUT) 0;
//   /* output.Pos = mul(input.Pos, World);
//    output.Pos = mul(output.Pos, View);
//    output.Pos = mul(output.Pos, Projection);
//    output.Norm = mul(input.Norm, (float3x3) World);
//    output.Tex = input.Tex;*/
//
//	output.Pos = input.Pos;
//	output.Norm = input.Norm;
//	output.Tex = input.Tex;
//
//    return output;
//}
//)";
//// Simple Pixel Shader
//const char* pixelShaderSource = R"(
//// TODO: Part 2b
//// TODO: Part 4g
//// TODO: Part 2i
//// TODO: Part 3e
//// an ultra simple hlsl pixel shader
//// TODO: Part 4b
//float4 main() : SV_TARGET 
//{	
//	return float4(0.75f ,0.75f, 0.25f, 0); // TODO: Part 1a
//	// TODO: Part 3a
//	// TODO: Part 4c
//	// TODO: Part 4g (half-vector or reflect method your choice)
//}
//)";
// Creation, Rendering & Cleanup
class Renderer
{
	// TODO: Part 2b
#define MAX_SUBMESH_PER_DRAW 1024
	struct SHADER_MODEL_DATA
	{
		GW::MATH::GVECTORF SunDirection, SunColor;
		GW::MATH::GMATRIXF ViewMatrix, ProjectionMatrix;

		GW::MATH::GMATRIXF matricies[MAX_SUBMESH_PER_DRAW];
		OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	};
	
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	
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
	VkDescriptorSet SB_DescriptorSet;
		// TODO: Part 4f
		
	// TODO: Part 2a
	GW::MATH::GMATRIXF MATRIX_World;
	GW::MATH::GMatrix PROXY_matrix;

	GW::MATH::GMATRIXF MATRIX_View;
	GW::MATH::GMATRIXF MATRIX_Projection;
	GW::MATH::GVECTORF VECTOR_Light_Direction;
	GW::MATH::GVECTORF VECTOR_Light_Color;

	// TODO: Part 2b
	SHADER_MODEL_DATA shader_model_data;
	// TODO: Part 4g
public:

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// TODO: Part 2a
		PROXY_matrix.Create();

		//TODO ROTATE ON Y OVER TIME
		//PROXY_matrix.RotateYGlobalF(GW::MATH::GIdentityMatrixF, 1.5708, MATRIX_World);

		GW::MATH::GVECTORF Eye = { 0.75f, 0.25, -1.5f };
		GW::MATH::GVECTORF At = { 0.15f, 0.75f, 0.0f };
		GW::MATH::GVECTORF Up = { 0.0f, 1.0f, 0.0f };

		PROXY_matrix.LookAtLHF(Eye, At, Up, MATRIX_View);

		float aspect;
		vlk.GetAspectRatio(aspect);

		PROXY_matrix.ProjectionVulkanLHF(G_DEGREE_TO_RADIAN(65), aspect, 0.1f, 100.0f, MATRIX_Projection);

		GW::MATH::GVECTORF lightDir = {-1.0f, -1.0f, 2.0f};
		GW::MATH::GVector::NormalizeF(lightDir, VECTOR_Light_Direction);

		VECTOR_Light_Color = { 0.9f, 0.9f, 1.0f, 1.0f };

		// TODO: Part 2b
		shader_model_data.ViewMatrix = MATRIX_View;
		shader_model_data.ProjectionMatrix = MATRIX_Projection;

		shader_model_data.SunColor = VECTOR_Light_Color;
		shader_model_data.SunDirection = VECTOR_Light_Direction;

		//shader_model_data.matricies[0] = MATRIX_World;
		shader_model_data.matricies[0] = GW::MATH::GIdentityMatrixF;

		for (int i = 0; i < FSLogo_materialcount; i++)
		{
			shader_model_data.materials [i] = FSLogo_materials[i].attrib;
		}


		// TODO: Part 4g
		// TODO: part 3b

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		// TODO: Part 1c
		// Create Vertex Buffer
		//float verts[] = FSLogo_vertices;
		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		GvkHelper::create_buffer(physicalDevice, device, sizeof(FSLogo_vertices),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, FSLogo_vertices, sizeof(FSLogo_vertices));
		// TODO: Part 1g
		GvkHelper::create_buffer(physicalDevice, device, sizeof(FSLogo_indices),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexHandle, &indexData);
		GvkHelper::write_to_buffer(device, indexData, FSLogo_indices, sizeof(FSLogo_indices));
		
		// TODO: Part 2d
		unsigned int max_active_frames; 
		vlk.GetSwapchainImageCount(max_active_frames);
		SB_Handle.resize(max_active_frames);
		SB_Data.resize(max_active_frames);

		for (int i = 0; i < max_active_frames; i++)
		{
			GvkHelper::create_buffer(physicalDevice, device, sizeof(shader_model_data),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &SB_Handle[i], &SB_Data[i]);
			GvkHelper::write_to_buffer(device, SB_Data[i], &shader_model_data, sizeof(shader_model_data));
		}

		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false); // TODO: Part 2i
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// Create Vertex Shader
		std::string Vertex_Shader_String = Renderer::ShaderAsString("VS.hlsl");

		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, Vertex_Shader_String.c_str(), strlen(Vertex_Shader_String.c_str()),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		shaderc_result_release(result); // done

		// Create Pixel Shader
		std::string Pixel_Shader_String = Renderer::ShaderAsString("PS.hlsl");

		result = shaderc_compile_into_spv( // compile
			compiler, Pixel_Shader_String.c_str(), strlen(Pixel_Shader_String.c_str()),
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
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = false;
		// TODO: Part 1e
		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(OBJ_VERT);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description[3] = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(OBJ_VERT, pos) }, //uv, normal, etc....
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(OBJ_VERT, uvw) }, //uvw
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(OBJ_VERT, nrm) }, //norm
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 3;
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
		
		// TODO: Part 2e
		VkDescriptorSetLayoutBinding sb_discriptor_binding = {};
		sb_discriptor_binding.descriptorCount = 1;
		sb_discriptor_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		sb_discriptor_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		sb_discriptor_binding.binding = 0;
		sb_discriptor_binding.pImmutableSamplers = VK_NULL_HANDLE;

		VkDescriptorSetLayoutCreateInfo sb_discriptor_create_info = {};
		sb_discriptor_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		sb_discriptor_create_info.flags = 0;
		sb_discriptor_create_info.bindingCount = 1;
		sb_discriptor_create_info.pNext = VK_NULL_HANDLE;
		sb_discriptor_create_info.pBindings = &sb_discriptor_binding;

		if(vkCreateDescriptorSetLayout(device, &sb_discriptor_create_info, nullptr, &SB_SetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		// TODO: Part 2f
		VkDescriptorPoolSize discriptor_pool_size = {};
		discriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		discriptor_pool_size.descriptorCount = static_cast<uint32_t>(max_active_frames);

		VkDescriptorPoolCreateInfo sb_discriptor_pool_create_info = {};
		sb_discriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		sb_discriptor_pool_create_info.pNext = VK_NULL_HANDLE;
		sb_discriptor_pool_create_info.flags = 0;
		sb_discriptor_pool_create_info.maxSets = static_cast<uint32_t>(max_active_frames);
		sb_discriptor_pool_create_info.poolSizeCount = 1;
		sb_discriptor_pool_create_info.pPoolSizes = &discriptor_pool_size;

		if(vkCreateDescriptorPool(device, &sb_discriptor_pool_create_info, nullptr, &SB_DescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
			// TODO: Part 4f
		// TODO: Part 2g
		VkDescriptorSetAllocateInfo sb_set_alloc_info = {};
		sb_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		sb_set_alloc_info.pSetLayouts = &SB_SetLayout;
		sb_set_alloc_info.pNext = VK_NULL_HANDLE;
		sb_set_alloc_info.descriptorSetCount = 1;
		sb_set_alloc_info.descriptorPool = SB_DescriptorPool;

		if (vkAllocateDescriptorSets(device, &sb_set_alloc_info, &SB_DescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
			// TODO: Part 4f
		// TODO: Part 2h
		VkDescriptorBufferInfo sb_descriptor_buffer_info{};
		for (int i = 0; i < SB_Handle.size(); i++) {
			sb_descriptor_buffer_info.buffer = SB_Handle[i];
			sb_descriptor_buffer_info.offset = 0;
			sb_descriptor_buffer_info.range = VK_WHOLE_SIZE;
		}

		VkWriteDescriptorSet sb_write_descriptor_set = {};
		sb_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		sb_write_descriptor_set.pNext = VK_NULL_HANDLE;
		sb_write_descriptor_set.pTexelBufferView = VK_NULL_HANDLE;
		sb_write_descriptor_set.dstSet = SB_DescriptorSet;
		sb_write_descriptor_set.pBufferInfo = &sb_descriptor_buffer_info;
		sb_write_descriptor_set.pImageInfo = VK_NULL_HANDLE;
		sb_write_descriptor_set.dstBinding = 0;
		sb_write_descriptor_set.descriptorCount = 1;
		sb_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		sb_write_descriptor_set.dstArrayElement = 0;
		vkUpdateDescriptorSets(device, 1, &sb_write_descriptor_set, 0, VK_NULL_HANDLE);

			// TODO: Part 4f
	
		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		// TODO: Part 2e
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &SB_SetLayout;
		// TODO: Part 3c
		pipeline_layout_create_info.pushConstantRangeCount = 0;
		pipeline_layout_create_info.pPushConstantRanges = nullptr;
		vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipelineLayout);

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
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline);

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
		// TODO: Part 2a


		// TODO: Part 3b

		// TODO: Part 4d
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
		
		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		// TODO: Part 1h
		vkCmdBindIndexBuffer(commandBuffer, indexHandle, *offsets, VK_INDEX_TYPE_UINT32);
		// TODO: Part 4d
		// TODO: Part 2i
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &SB_DescriptorSet, 0, VK_NULL_HANDLE);
		// TODO: Part 3b
			// TODO: Part 3d
		for (int i = 0; i < FSLogo_meshcount; i++) {
			vkCmdDrawIndexed(commandBuffer, FSLogo_meshes[i].indexCount, 1, FSLogo_meshes[i].indexOffset, 0, 0); // TODO: Part 1d, 1h
		}
	}
	
	std::string ShaderAsString(const char* shaderFilePath) {
		std::string output;
		unsigned int stringLength = 0;
		GW::SYSTEM::GFile file; file.Create();
		file.GetFileSize(shaderFilePath, stringLength);
		if (stringLength && +file.OpenBinaryRead(shaderFilePath)) {
			output.resize(stringLength);
			file.Read(&output[0], stringLength);
		}
		else
			std::cout << "ERROR: Shader Source File \"" << shaderFilePath << "\" Not Found!" << std::endl;
		return output;
	}

private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		// TODO: Part 1g
		vkDestroyBuffer(device, indexHandle, nullptr);
		vkFreeMemory(device, indexData, nullptr);
		// TODO: Part 2d
		for (int i = 0; i < SB_Handle.size(); i++) {
			vkDestroyBuffer(device, SB_Handle[i], nullptr);
		}

		for (int i = 0; i < SB_Data.size(); i++) {
			vkFreeMemory(device, SB_Data[i], nullptr);
		}

		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		// TODO: Part 2e
		vkDestroyDescriptorSetLayout(device, SB_SetLayout, nullptr);
		// TODO: part 2f
		vkDestroyDescriptorPool(device, SB_DescriptorPool, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
	}
};
