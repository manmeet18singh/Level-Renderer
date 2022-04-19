#include "renderer.h"
#include "h2bParser.h"

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

	InitContent();
	InitShader();
	InitPipeline(width, height);
	Shutdown();
}

void Renderer::InitContent() 
{
	//TODO ROTATE ON Y OVER TIME

	GW::MATH::GVECTORF Eye = { 0.75f, 0.25, -1.5f };
	GW::MATH::GVECTORF At = { 0.15f, 0.75f, 0.0f };
	GW::MATH::GVECTORF Up = { 0.0f, 1.0f, 0.0f };

	PROXY_matrix.LookAtLHF(Eye, At, Up, MATRIX_View);

	float aspect;
	vlk.GetAspectRatio(aspect);

	PROXY_matrix.ProjectionVulkanLHF(G_DEGREE_TO_RADIAN(65), aspect, 0.1f, 100.0f, MATRIX_Projection);

	GW::MATH::GVECTORF lightDir = { -1.0f, -1.0f, 2.0f };
	PROXY_vector.NormalizeF(lightDir, VECTOR_Light_Direction);

	VECTOR_Light_Color = { 0.9f, 0.9f, 1.0f, 1.0f };

	// TODO: Part 2b
	shader_model_data.ViewMatrix = MATRIX_View;
	shader_model_data.ProjectionMatrix = MATRIX_Projection;

	shader_model_data.SunColor = VECTOR_Light_Color;
	shader_model_data.SunDirection = VECTOR_Light_Direction;

	shader_model_data.matricies[0] = GW::MATH::GIdentityMatrixF;

	// TODO: Part 4g
	shader_model_data.CamPos = Eye;

	VECTOR_Ambient = { 0.25f, 0.25f, 0.35f, 1.0f };
	shader_model_data.SunAmbient = VECTOR_Ambient;
	// TODO: part 3b

	for (int i = 0; i < FSLogo_materialcount; i++)
	{
		shader_model_data.materials[i] = FSLogo_materials[i].attrib;
	}

	/***************** GEOMETRY INTIALIZATION ******************/
	// Grab the device & physical device so we can allocate some stuff
	VkPhysicalDevice physicalDevice = nullptr;
	vlk.GetDevice((void**)&device);
	vlk.GetPhysicalDevice((void**)&physicalDevice);

	// TODO: Part 1c
	// Create Vertex Buffer

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
	vlk.GetSwapchainImageCount(max_active_frames);
	SB_Handle.resize(max_active_frames);
	SB_Data.resize(max_active_frames);
	SB_DescriptorSet.resize(max_active_frames);

	for (int i = 0; i < max_active_frames; i++)
	{
		GvkHelper::create_buffer(physicalDevice, device, sizeof(shader_model_data),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &SB_Handle[i], &SB_Data[i]);
		GvkHelper::write_to_buffer(device, SB_Data[i], &shader_model_data, sizeof(shader_model_data));
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
	std::string Vertex_Shader_String = ShaderAsString("VS.hlsl");

	shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
		compiler, Vertex_Shader_String.c_str(), strlen(Vertex_Shader_String.c_str()),
		shaderc_vertex_shader, "main.vert", "main", options);
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
	GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
		(char*)shaderc_result_get_bytes(result), &vertexShader);
	shaderc_result_release(result); // done

	// Create Pixel Shader
	std::string Pixel_Shader_String = ShaderAsString("PS.hlsl");

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
	vertex_binding_description.stride = sizeof(OBJ_VERT);
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

	if (vkCreateDescriptorSetLayout(device, &sb_discriptor_create_info, nullptr, &SB_SetLayout) != VK_SUCCESS) {
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

	if (vkCreateDescriptorPool(device, &sb_discriptor_pool_create_info, nullptr, &SB_DescriptorPool) != VK_SUCCESS) {
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

	for (int i = 0; i < max_active_frames; i++)
	{
		if (vkAllocateDescriptorSets(device, &sb_set_alloc_info, &SB_DescriptorSet[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	// TODO: Part 4f
// TODO: Part 2h
	std::vector <VkDescriptorBufferInfo> sb_descriptor_buffer_info(max_active_frames);
	for (int i = 0; i < max_active_frames; i++) {
		sb_descriptor_buffer_info[i].buffer = SB_Handle[i];
		sb_descriptor_buffer_info[i].offset = 0;
		sb_descriptor_buffer_info[i].range = VK_WHOLE_SIZE;
	}

	std::vector <VkWriteDescriptorSet> sb_write_descriptor_set(max_active_frames);
	for (int i = 0; i < max_active_frames; i++) {
		sb_write_descriptor_set[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		sb_write_descriptor_set[i].pNext = VK_NULL_HANDLE;
		sb_write_descriptor_set[i].pTexelBufferView = VK_NULL_HANDLE;
		sb_write_descriptor_set[i].dstSet = SB_DescriptorSet[i];
		sb_write_descriptor_set[i].pBufferInfo = &sb_descriptor_buffer_info[i];
		sb_write_descriptor_set[i].pImageInfo = VK_NULL_HANDLE;
		sb_write_descriptor_set[i].dstBinding = 0;
		sb_write_descriptor_set[i].descriptorCount = 1;
		sb_write_descriptor_set[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		sb_write_descriptor_set[i].dstArrayElement = 0;
		vkUpdateDescriptorSets(device, 1, &sb_write_descriptor_set[i], 0, VK_NULL_HANDLE);
	}
	// TODO: Part 4f

	VkPushConstantRange push_constant_range = {};
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(unsigned int);
	push_constant_range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

	// Descriptor pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	// TODO: Part 2e
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts = &SB_SetLayout;
	// TODO: Part 3c
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
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
		PROXY_matrix.RotateYGlobalF(GW::MATH::GIdentityMatrixF, timer.TotalTime(), MATRIX_World);

		// TODO: Part 3b

		// TODO: Part 4d
		shader_model_data.matricies[1] = MATRIX_World;
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

		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		// TODO: Part 1h
		vkCmdBindIndexBuffer(commandBuffer, indexHandle, *offsets, VK_INDEX_TYPE_UINT32);
		// TODO: Part 4d
		GvkHelper::write_to_buffer(device, SB_Data[currentBuffer], &shader_model_data, sizeof(shader_model_data));
		// TODO: Part 2i
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &SB_DescriptorSet[currentBuffer], 0, VK_NULL_HANDLE);
		// TODO: Part 3b
			// TODO: Part 3d
		for (int i = 0; i < FSLogo_meshcount; i++) {
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(unsigned int), &FSLogo_meshes[i].materialIndex);
			vkCmdDrawIndexed(commandBuffer, FSLogo_meshes[i].indexCount, 1, FSLogo_meshes[i].indexOffset, 0, 0); // TODO: Part 1d, 1h
		}
}

void Renderer::UpdateCamera() {

	// TODO: Part 4c
	GW::MATH::GMATRIXF Camera;

	PROXY_matrix.InverseF(MATRIX_View, Camera);

	// TODO: Part 4d
	float KEY_spc = 0;
	float KEY_lShift = 0;
	float KEY_rTrigger = 0;
	float KEY_lTrigger = 0;

	float KEY_w = 0;
	float KEY_s = 0;
	float KEY_d = 0;
	float KEY_a = 0;
	float KEY_lStick_y = 0;
	float KEY_lStick_x = 0;
	float KEY_rStick_y = 0;
	float KEY_rStick_x = 0;

	float KEY_mouse_x = 0;
	float KEY_mouse_y = 0;

	const float Camera_Speed = 0.3f;
	float Seconds_Passed_Since_Last_Frame = timer.Delta();

	unsigned int width, height;
	win.GetClientWidth(width);
	win.GetClientHeight(height);

	float aspect;
	vlk.GetAspectRatio(aspect);

	PROXY_input.GetState(G_KEY_SPACE, KEY_spc);
	PROXY_input.GetState(G_KEY_LEFTSHIFT, KEY_lShift);
	
	PROXY_input.GetState(G_KEY_W, KEY_w);
	PROXY_input.GetState(G_KEY_S, KEY_s);
	PROXY_input.GetState(G_KEY_D, KEY_d);
	PROXY_input.GetState(G_KEY_A, KEY_a);

	GW::GReturn result = PROXY_input.GetMouseDelta(KEY_mouse_x, KEY_mouse_y);
	if (!G_PASS(result) || result == GW::GReturn::REDUNDANT)
	{
		KEY_mouse_x = 0;
		KEY_mouse_y = 0;
	}

	PROXY_controller.GetState(0, G_RIGHT_TRIGGER_AXIS, KEY_rTrigger);
	PROXY_controller.GetState(0, G_LEFT_TRIGGER_AXIS, KEY_lTrigger);

	PROXY_controller.GetState(0, G_LY_AXIS, KEY_lStick_y);
	PROXY_controller.GetState(0, G_LX_AXIS, KEY_lStick_x);

	PROXY_controller.GetState(0, G_RY_AXIS, KEY_rStick_y);
	PROXY_controller.GetState(0, G_RX_AXIS, KEY_rStick_x);

	// TODO: Part 4e
	float Per_Frame_Speed = Camera_Speed * Seconds_Passed_Since_Last_Frame;
	
	float Total_Y_Change = KEY_spc - KEY_lShift + KEY_rTrigger - KEY_lTrigger;
	float Total_Z_Change = KEY_w - KEY_s + KEY_lStick_y;
	float Total_X_Change = KEY_d - KEY_a + KEY_lStick_x;

	PROXY_matrix.TranslateLocalF(Camera, GW::MATH::GVECTORF{ Total_X_Change * Per_Frame_Speed , Total_Y_Change * Per_Frame_Speed , Total_Z_Change * Per_Frame_Speed }, Camera);

	float Thumb_Speed = G_PI * Seconds_Passed_Since_Last_Frame;
	
	float Total_Pitch = G_DEGREE_TO_RADIAN(65) * KEY_mouse_y / height + KEY_rStick_y * -Thumb_Speed;
	GW::MATH::GMATRIXF Pitch_Mat = GW::MATH::GIdentityMatrixF;
	PROXY_matrix.RotationYawPitchRollF(0, Total_Pitch, 0, Pitch_Mat);


	float Total_Yaw = G_DEGREE_TO_RADIAN(65) * aspect * KEY_mouse_x / width + KEY_rStick_x * Thumb_Speed;
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

void Renderer::CleanUp()
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