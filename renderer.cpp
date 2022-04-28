#include "renderer.h"
#include "h2bParser.h"
#include <windows.h>
#include <iostream>
#include <fstream>

Renderer::Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
{
	win = _win;
	vlk = _vlk;
	unsigned int width, height;
	win.GetClientWidth(width);
	win.GetClientHeight(height);
	// TODO: Part 2a
	timer = XTime();
	timer.Restart();

	PROXY_matrix.Create();
	PROXY_vector.Create();
	PROXY_input.Create(win);
	PROXY_controller.Create();

	//Load in default level
	ReadGameLevelFile("../Assets/GameLevel.txt");
	LoadGameLevel();

	InitContent();
	InitShader();
	InitPipeline(width, height);
	Shutdown();
}

void Renderer::InitContent()
{
	//TODO ROTATE ON Y OVER TIME

	GW::MATH::GVECTORF Eye = {-30.0f, 20.0f, 10.0f };
	GW::MATH::GVECTORF At = { 0.0f, 0.0f, 0.0f };
	GW::MATH::GVECTORF Up = { 0.0f, 1.0f, 0.0f };

	PROXY_matrix.LookAtLHF(Eye, At, Up, MATRIX_View);

	float aspect;
	vlk.GetAspectRatio(aspect);

	PROXY_matrix.ProjectionVulkanLHF(G_DEGREE_TO_RADIAN(65), aspect, 0.1f, 100.0f, MATRIX_Projection);

	VECTOR_Light_Color = { 0.9f, 0.9f, 1.0f, 1.0f };

	// TODO: Part 2b
	shader_model_data.ViewMatrix = MATRIX_View;
	shader_model_data.ProjectionMatrix = MATRIX_Projection;

	shader_model_data.SunColor = VECTOR_Light_Color;
	shader_model_data.SunDirection = VECTOR_Light_Direction;

	// TODO: Part 4g
	shader_model_data.CamPos = Eye;

	VECTOR_Ambient = { 0.15f, 0.15f, 0.25f, 1.0f };
	shader_model_data.SunAmbient = VECTOR_Ambient;
	// TODO: part 3b

	unsigned offset = 0;
	int pos = 0;
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{

		for (int i = 0; i < List_Of_Game_Objects[j].materialCount; i++)
		{
			shader_model_data.materials[pos] = List_Of_Game_Objects[j].materials[i].attrib;
			pos++;
		}
		List_Of_Game_Objects[j].matOffset = offset;
		offset += List_Of_Game_Objects[j].materialCount;
	}

	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
		shader_model_data.matricies[j] = List_Of_Game_Objects[j].worldMatrix;
	}

	/***************** GEOMETRY INTIALIZATION ******************/
	// Grab the device & physical device so we can allocate some stuff
	VkPhysicalDevice physicalDevice = nullptr;
	vlk.GetDevice((void**)&device);
	vlk.GetPhysicalDevice((void**)&physicalDevice);

	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		GvkHelper::create_buffer(physicalDevice, device, (sizeof(List_Of_Game_Objects[j].vertices[0]) * List_Of_Game_Objects[j].vertices.size()),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &List_Of_Game_Objects[j].vertexHandle, &List_Of_Game_Objects[j].vertexData);
		GvkHelper::write_to_buffer(device, List_Of_Game_Objects[j].vertexData, List_Of_Game_Objects[j].vertices.data(), (sizeof(List_Of_Game_Objects[j].vertices[0]) * List_Of_Game_Objects[j].vertices.size()));
		// TODO: Part 1g
		GvkHelper::create_buffer(physicalDevice, device, (sizeof(List_Of_Game_Objects[j].indices[0]) * List_Of_Game_Objects[j].indices.size()),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &List_Of_Game_Objects[j].indexHandle, &List_Of_Game_Objects[j].indexData);
		GvkHelper::write_to_buffer(device, List_Of_Game_Objects[j].indexData, List_Of_Game_Objects[j].indices.data(), (sizeof(List_Of_Game_Objects[j].indices[0]) * List_Of_Game_Objects[j].indices.size()));
	}

	// TODO: Part 2d
	vlk.GetSwapchainImageCount(max_active_frames);
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
		List_Of_Game_Objects[j].SB_Handle.resize(max_active_frames);
		List_Of_Game_Objects[j].SB_Data.resize(max_active_frames);
		List_Of_Game_Objects[j].SB_DescriptorSet.resize(max_active_frames);

		for (int i = 0; i < max_active_frames; i++)
		{
			GvkHelper::create_buffer(physicalDevice, device, sizeof(shader_model_data),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &List_Of_Game_Objects[j].SB_Handle[i], &List_Of_Game_Objects[j].SB_Data[i]);
			GvkHelper::write_to_buffer(device, List_Of_Game_Objects[j].SB_Data[i], &shader_model_data, sizeof(shader_model_data));
		}
	}
}

void Renderer::InitShader()
{
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
	std::string Vertex_Shader_String = ShaderAsString("../Shaders/VS.hlsl");

	shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
		compiler, Vertex_Shader_String.c_str(), strlen(Vertex_Shader_String.c_str()),
		shaderc_vertex_shader, "main.vert", "main", options);
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
	GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
		(char*)shaderc_result_get_bytes(result), &vertexShader);
	shaderc_result_release(result); // done

	// Create Pixel Shader
	std::string Pixel_Shader_String = ShaderAsString("../Shaders/PS.hlsl");

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
}

void Renderer::InitPipeline(unsigned int width, unsigned int height)
{
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
	vertex_binding_description.stride = sizeof(H2B::VERTEX);
	vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	VkVertexInputAttributeDescription vertex_attribute_description[3] = {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, //xyz
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3}, //uvw
		{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 6}, //nrm
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
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
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

		if (vkCreateDescriptorSetLayout(device, &sb_discriptor_create_info, nullptr, &List_Of_Game_Objects[j].SB_SetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// TODO: Part 2f
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
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

		if (vkCreateDescriptorPool(device, &sb_discriptor_pool_create_info, nullptr, &List_Of_Game_Objects[j].SB_DescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}
	// TODO: Part 4f
// TODO: Part 2g
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
		VkDescriptorSetAllocateInfo sb_set_alloc_info = {};
		sb_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		sb_set_alloc_info.pSetLayouts = &List_Of_Game_Objects[j].SB_SetLayout;
		sb_set_alloc_info.pNext = VK_NULL_HANDLE;
		sb_set_alloc_info.descriptorSetCount = 1;
		sb_set_alloc_info.descriptorPool = List_Of_Game_Objects[j].SB_DescriptorPool;

		for (int i = 0; i < max_active_frames; i++)
		{
			if (vkAllocateDescriptorSets(device, &sb_set_alloc_info, &List_Of_Game_Objects[j].SB_DescriptorSet[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor sets!");
			}
		}
	}
	// TODO: Part 4f
// TODO: Part 2h
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
		std::vector <VkDescriptorBufferInfo> sb_descriptor_buffer_info(max_active_frames);
		for (int i = 0; i < max_active_frames; i++) {
			sb_descriptor_buffer_info[i].buffer = List_Of_Game_Objects[j].SB_Handle[i];
			sb_descriptor_buffer_info[i].offset = 0;
			sb_descriptor_buffer_info[i].range = VK_WHOLE_SIZE;
		}

		std::vector <VkWriteDescriptorSet> sb_write_descriptor_set(max_active_frames);
		for (int i = 0; i < max_active_frames; i++) {
			sb_write_descriptor_set[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			sb_write_descriptor_set[i].pNext = VK_NULL_HANDLE;
			sb_write_descriptor_set[i].pTexelBufferView = VK_NULL_HANDLE;
			sb_write_descriptor_set[i].dstSet = List_Of_Game_Objects[j].SB_DescriptorSet[i];
			sb_write_descriptor_set[i].pBufferInfo = &sb_descriptor_buffer_info[i];
			sb_write_descriptor_set[i].pImageInfo = VK_NULL_HANDLE;
			sb_write_descriptor_set[i].dstBinding = 0;
			sb_write_descriptor_set[i].descriptorCount = 1;
			sb_write_descriptor_set[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			sb_write_descriptor_set[i].dstArrayElement = 0;
			vkUpdateDescriptorSets(device, 1, &sb_write_descriptor_set[i], 0, VK_NULL_HANDLE);
		}
	}
	// TODO: Part 4f

	VkPushConstantRange push_constant_range = {};
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(PUSH_CONSTANTS);
	push_constant_range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

	// Descriptor pipeline layout
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{

		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		// TODO: Part 2e
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &List_Of_Game_Objects[j].SB_SetLayout;
		// TODO: Part 3c
		pipeline_layout_create_info.pushConstantRangeCount = 1;
		pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
		vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipelineLayout);
	}

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
}

void Renderer::Shutdown()
{
	/***************** CLEANUP / SHUTDOWN ******************/
// GVulkanSurface will inform us when to release any allocated resources
	shutdown.Create(vlk, [&]() {
		if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
			CleanUp(); // unlike D3D we must be careful about destroy timing
		}
		});
}

void Renderer::Render() {
	// TODO: Part 2a

	shader_model_data.ViewMatrix = MATRIX_View;

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



	// TODO: Part 3b
		// TODO: Part 3d
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &List_Of_Game_Objects[j].vertexHandle, offsets);
		// TODO: Part 1h
		vkCmdBindIndexBuffer(commandBuffer, List_Of_Game_Objects[j].indexHandle, *offsets, VK_INDEX_TYPE_UINT32);
		// TODO: Part 2i
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &List_Of_Game_Objects[j].SB_DescriptorSet[currentBuffer], 0, VK_NULL_HANDLE);

		// TODO: Part 4d
		GvkHelper::write_to_buffer(device, List_Of_Game_Objects[j].SB_Data[currentBuffer], &shader_model_data, sizeof(shader_model_data));
		for (int i = 0; i < List_Of_Game_Objects[j].meshCount; i++) {

			push_constants.material_Index = List_Of_Game_Objects[j].matOffset + List_Of_Game_Objects[j].meshes[i].materialIndex;
			push_constants.model_Index = j;

			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(push_constants), &push_constants);
			vkCmdDrawIndexed(commandBuffer, List_Of_Game_Objects[j].meshes[i].drawInfo.indexCount, 1, List_Of_Game_Objects[j].meshes[i].drawInfo.indexOffset, 0, 0); // TODO: Part 1d, 1h
		}
	}

}

INPUT_CONTROLLER Renderer::WaitForInput() {
	INPUT_CONTROLLER input = {};

	PROXY_input.GetState(G_KEY_SPACE, input.INPUT_spc);
	PROXY_input.GetState(G_KEY_LEFTSHIFT, input.INPUT_lShift);

	PROXY_input.GetState(G_KEY_W, input.INPUT_w);
	PROXY_input.GetState(G_KEY_S, input.INPUT_s);
	PROXY_input.GetState(G_KEY_D, input.INPUT_d);
	PROXY_input.GetState(G_KEY_A, input.INPUT_a);

	//New File
	PROXY_input.GetState(G_KEY_O, input.INPUT_o);
	PROXY_input.GetState(G_KEY_LEFTCONTROL, input.INPUT_lCtrl);
	PROXY_controller.GetState(0, G_START_BTN, input.CINPUT_start);

	GW::GReturn result = PROXY_input.GetMouseDelta(input.INPUT_mouse_x, input.INPUT_mouse_y);
	if (!G_PASS(result) || result == GW::GReturn::REDUNDANT)
	{
		input.INPUT_mouse_x = 0;
		input.INPUT_mouse_y = 0;
	}

	PROXY_controller.GetState(0, G_RIGHT_TRIGGER_AXIS, input.CINPUT_rTrigger);
	PROXY_controller.GetState(0, G_LEFT_TRIGGER_AXIS, input.CINPUT_lTrigger);

	PROXY_controller.GetState(0, G_LY_AXIS, input.CINPUT_lStick_y);
	PROXY_controller.GetState(0, G_LX_AXIS, input.CINPUT_lStick_x);

	PROXY_controller.GetState(0, G_RY_AXIS, input.CINPUT_rStick_y);
	PROXY_controller.GetState(0, G_RX_AXIS, input.CINPUT_rStick_x);

	return input;
}

std::string Renderer::FileOpen()
{
	OPENFILENAMEW open;
	memset(&open, 0, sizeof(open));
	
	wchar_t filename[MAX_PATH];
	filename[0] = L'\0';

	open.lStructSize = sizeof(OPENFILENAMEW);
	open.lpstrFile = filename;
	open.nMaxFile = MAX_PATH;
	open.lpstrFilter = L"All\0*.*\0Text\0*.txt\0";
	open.nFilterIndex = 2;
	open.lpstrFileTitle = L"Load New Level From .txt File";
	open.lpstrInitialDir = NULL;
	open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&open) == TRUE)
	{
		std::wstring file = open.lpstrFile;
		return std::string(file.begin(), file.end());
	}
	return "";
}

void Renderer::LoadNewLevel(INPUT_CONTROLLER input) {
	
	//push lctrl and o to open file dialog
	if (input.INPUT_o == 0 || input.INPUT_lCtrl == 0) {
		return;
	}

	//if (input.CINPUT_start == 0 ) {
	//	return;
	//}

	std::string newPath = FileOpen();

	if (newPath == "") return;

	CleanUp();

	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
		List_Of_Game_Objects[j].SB_Handle.clear();
		List_Of_Game_Objects[j].SB_Data.clear();
		List_Of_Game_Objects[j].SB_DescriptorSet.clear();
		List_Of_Game_Objects[j].vertices.clear();
		List_Of_Game_Objects[j].indices.clear();
		List_Of_Game_Objects[j].materials.clear();
		List_Of_Game_Objects[j].batches.clear();
		List_Of_Game_Objects[j].meshes.clear();

	}

	List_Of_Game_Objects.clear();

	unsigned int width, height;
	win.GetClientWidth(width);
	win.GetClientHeight(height);

	ReadGameLevelFile(newPath.c_str());
	LoadGameLevel();

	InitContent();
	InitShader();
	InitPipeline(width, height);
}

void Renderer::UpdateCamera(INPUT_CONTROLLER input) {

	// TODO: Part 4c
	GW::MATH::GMATRIXF Camera;

	PROXY_matrix.InverseF(MATRIX_View, Camera);

	// TODO: Part 4d


	const float Camera_Speed = 10.0f;
	float Seconds_Passed_Since_Last_Frame = timer.Delta();

	unsigned int width, height;
	win.GetClientWidth(width);
	win.GetClientHeight(height);

	float aspect;
	vlk.GetAspectRatio(aspect);

	// TODO: Part 4e
	float Per_Frame_Speed = Camera_Speed * Seconds_Passed_Since_Last_Frame;

	float Total_Y_Change = input.INPUT_spc - input.INPUT_lShift + input.CINPUT_rTrigger - input.CINPUT_lTrigger;
	float Total_Z_Change = input.INPUT_w - input.INPUT_s + input.CINPUT_lStick_y;
	float Total_X_Change = input.INPUT_d - input.INPUT_a + input.CINPUT_lStick_x;

	PROXY_matrix.TranslateLocalF(Camera, GW::MATH::GVECTORF{ Total_X_Change * Per_Frame_Speed , Total_Y_Change * Per_Frame_Speed , Total_Z_Change * Per_Frame_Speed }, Camera);

	float Thumb_Speed = G_PI * Seconds_Passed_Since_Last_Frame;

	float Total_Pitch = G_DEGREE_TO_RADIAN(65) * input.INPUT_mouse_y / height + input.CINPUT_rStick_y * -Thumb_Speed;
	GW::MATH::GMATRIXF Pitch_Mat = GW::MATH::GIdentityMatrixF;
	PROXY_matrix.RotationYawPitchRollF(0, Total_Pitch, 0, Pitch_Mat);


	float Total_Yaw = G_DEGREE_TO_RADIAN(65) * aspect * input.INPUT_mouse_x / width + input.CINPUT_rStick_x * Thumb_Speed;
	GW::MATH::GMATRIXF Yaw_Mat = GW::MATH::GIdentityMatrixF;
	PROXY_matrix.RotationYawPitchRollF(Total_Yaw, 0, 0, Yaw_Mat);

	PROXY_matrix.MultiplyMatrixF(Pitch_Mat, Camera, Camera);
	GW::MATH::GVECTORF Save_Pos = Camera.row4;
	PROXY_matrix.MultiplyMatrixF(Camera, Yaw_Mat, Camera);
	Camera.row4 = Save_Pos;

	// TODO: Part 4c		
	PROXY_matrix.InverseF(Camera, MATRIX_View);
}

void Renderer::SignalTimer()
{
	timer.Signal();
}

std::string Renderer::ShaderAsString(const char* shaderFilePath) {
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

void Renderer::ReadGameLevelFile(const char* levelFilePath) {
	std::ifstream level;

	level.open(levelFilePath, std::ifstream::in);

	if (level.is_open() == false) {
		std::cout << "ERROR: Level Source File \"" << levelFilePath << "\" Not Found!" << std::endl;
		throw std::runtime_error("");
		return;
	}

	std::string currLine;
	GAMEOBJECT obj2save;
	LIGHT light2save;

	while (!level.eof()) {
		std::getline(level, currLine);
		if (currLine == "MESH") {

			std::getline(level, currLine); //Grab the next line
			std::string objName = currLine.substr(0, currLine.find('.')); //grab the name of the mesh, ignore .xxx number for duplicates
			obj2save.name = objName;

			GW::MATH::GMATRIXF objPos;

			for (int i = 0; i < 4; i++)
			{
				std::getline(level, currLine); //Grab the next line
				std::string posVal = currLine.substr(currLine.find('('), currLine.find(')'));

				float x, y, z, w;

				int scan = std::sscanf(posVal.c_str(), "(%f, %f, %f, %f)", &x, &y, &z, &w);

				if (i == 0) {
					objPos.row1 = { x, y, z, w };
				}
				else if (i == 1) {
					objPos.row2 = { x, y, z, w };
				}
				else if (i == 2) {
					objPos.row3 = { x, y, z, w };
				}
				else if (i == 3) {
					objPos.row4 = { x, y, z, w };
				}
				else if (scan == 0 || scan == EOF) {
					std::cout << "error reading matrix";
				}
			}
			obj2save.worldMatrix = objPos;
			List_Of_Game_Objects.push_back(obj2save);
		}
		if (currLine == "LIGHT") {

			std::getline(level, currLine); //Grab the next line
			std::string lightName = currLine.substr(0, currLine.find('.')); //grab the name of the mesh, ignore .xxx number for duplicates
			
			GW::MATH::GVECTORF lightDir;

			GW::MATH::GMATRIXF objPos;

			for (int i = 0; i < 4; i++)
			{
				std::getline(level, currLine); //Grab the next line
				std::string posVal = currLine.substr(currLine.find('('), currLine.find(')'));

				float x, y, z, w;

				int scan = std::sscanf(posVal.c_str(), "(%f, %f, %f, %f)", &x, &y, &z, &w);

				if (i == 3) {

					VECTOR_Light_Direction = { x, y, z, w };
				}
				else if (scan == 0 || scan == EOF) {
					std::cout << "error reading matrix";
				}
			}
			
		}
	}

}

void Renderer::LoadGameLevel() {
	H2B::Parser currObj;

	for (int i = 0; i < List_Of_Game_Objects.size(); i++)
	{
		if (currObj.Parse(std::string("../Assets/H2B Models/" + List_Of_Game_Objects[i].name + ".h2b").c_str())) {

			List_Of_Game_Objects[i].vertexCount = currObj.vertexCount;
			List_Of_Game_Objects[i].indexCount = currObj.indexCount;
			List_Of_Game_Objects[i].materialCount = currObj.materialCount;
			List_Of_Game_Objects[i].meshCount = currObj.meshCount;

			List_Of_Game_Objects[i].vertices = currObj.vertices;
			List_Of_Game_Objects[i].indices = currObj.indices;
			List_Of_Game_Objects[i].materials = currObj.materials;
			List_Of_Game_Objects[i].batches = currObj.batches;
			List_Of_Game_Objects[i].meshes = currObj.meshes;

		}
		else {
			throw std::runtime_error("Error loading model: " + List_Of_Game_Objects[i].name + ".h2b");
		}
	}

}

void Renderer::CleanUp()
{
	// wait till everything has completed
	vkDeviceWaitIdle(device);
	// Release allocated buffers, shaders & pipeline

	// TODO: Part 2d
	for (int j = 0; j < List_Of_Game_Objects.size(); j++)
	{
		// TODO: Part 1g
		vkDestroyBuffer(device, List_Of_Game_Objects[j].indexHandle, nullptr);
		vkFreeMemory(device, List_Of_Game_Objects[j].indexData, nullptr);

		vkDestroyBuffer(device, List_Of_Game_Objects[j].vertexHandle, nullptr);
		vkFreeMemory(device, List_Of_Game_Objects[j].vertexData, nullptr);

		vkDestroyDescriptorSetLayout(device, List_Of_Game_Objects[j].SB_SetLayout, nullptr);

		vkDestroyDescriptorPool(device, List_Of_Game_Objects[j].SB_DescriptorPool, nullptr);

		for (int i = 0; i < List_Of_Game_Objects[j].SB_Handle.size(); i++) {
			vkDestroyBuffer(device, List_Of_Game_Objects[j].SB_Handle[i], nullptr);
		}

		for (int i = 0; i < List_Of_Game_Objects[j].SB_Data.size(); i++) {
			vkFreeMemory(device, List_Of_Game_Objects[j].SB_Data[i], nullptr);
		}
	}

	vkDestroyShaderModule(device, vertexShader, nullptr);
	vkDestroyShaderModule(device, pixelShader, nullptr);

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
}