#include <itu_engine.hpp>

inline void itu_lib_imgui_setup(EngineContext* context, bool intercept_keyboard)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	context->imgui_io = &io; // store imgui contex, to keep aligned rendering scale
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.FontDefault = io.Fonts->AddFontFromFileTTF("data/fonts/CASCADIAMONO.ttf", 16);

	// setup default style
	ImGui::StyleColorsDark();

	if(intercept_keyboard)
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls


	// do some small tweaks
	itu_lib_imgui_set_scale(context->zoom);

	if(context->renderer)
	{
		ImGui_ImplSDL3_InitForSDLRenderer(context->window, context->renderer);
		ImGui_ImplSDLRenderer3_Init(context->renderer);
	}
#ifdef ITU_SYS_RENDER_3D_IMPLEMENTATION
	if(context->ctx_rendering)
	{
		ITU_Renderer3D* ctx_rendering = context->ctx_rendering;		
		SDL_assert(ctx_rendering->device);

		ImGui_ImplSDL3_InitForSDLGPU(context->window);
		ImGui_ImplSDLGPU3_InitInfo init_info = {};
		init_info.Device = ctx_rendering->device;
		init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(ctx_rendering->device, context->window);
		init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
		init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
		init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
		ImGui_ImplSDLGPU3_Init(&init_info);
	}
#endif //ITU_SYS_RENDER_3D_IMPLEMENTATION
	else
	{
		SDL_LogWarn(0, "cannot init imgui, neither SDL_Renderer nor SDL_GPUDevice are initialized!");
	}
}

inline void itu_lib_imgui_set_scale(float zoom)
{
	ImGuiStyle& style = ImGui::GetStyle();


	// imgui has neither a "reset" nor a "set" scale, it allows only to scale from curent one
	// so we need to reset it manually
	style.WindowPadding = ImVec2(8,8);
	style.WindowRounding = 0.0f;
	style.WindowMinSize = ImVec2(32, 32);
	style.WindowBorderHoverPadding = 4.0f;
	style.ChildRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.FramePadding = ImVec2(4,3);
	style.FrameRounding = 0.0f;
	style.ItemSpacing = ImVec2(8,4);
	style.ItemInnerSpacing = ImVec2(4,4);
	style.CellPadding = ImVec2(4,2);
	style.TouchExtraPadding = ImVec2(0,0);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.ScrollbarPadding = 2.0f;
	style.GrabMinSize = 12.0f;
	style.GrabRounding = 0.0f;
	style.LogSliderDeadzone = 4.0f;
	style.ImageBorderSize = 0.0f;
	style.TabRounding = 5.0f;
	style.TabMinWidthBase = 1.0f;
	style.TabMinWidthShrink = 80.0f;
	style.TabCloseButtonMinWidthSelected = -1.0f;
	style.TabCloseButtonMinWidthUnselected = 0.0f;
	style.TabBarOverlineSize = 1.0f;
	style.TreeLinesRounding = 0.0f;
	style.SeparatorTextPadding = ImVec2(20.0f,3.f);
	style.DockingSeparatorSize = 2.0f;
	style.DisplayWindowPadding = ImVec2(19,19);
	style.DisplaySafeAreaPadding = ImVec2(3,3);
	style.MouseCursorScale = 1.0f;


	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	main_scale *= zoom;
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;
}

inline bool itu_lib_imgui_process_sdl_event(SDL_Event* event)
{
	ImGui_ImplSDL3_ProcessEvent(event);

	ImGuiIO& io = ImGui::GetIO();
	switch(event->type)
	{
		case SDL_EVENT_KEY_UP:
		case SDL_EVENT_KEY_DOWN:
			// NOTE: we assume F keys are for debugging, so we'll forward those
			if(event->key.key >= SDLK_F1 && event->key.key <= SDLK_F12)
				return false;
			return io.WantCaptureKeyboard;
		case SDL_EVENT_MOUSE_WHEEL:
		case SDL_EVENT_MOUSE_MOTION:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			return io.WantCaptureMouse;
	}

	return false;
}

inline void itu_lib_imgui_frame_begin(EngineContext* context)
{
	if(context->renderer)
		ImGui_ImplSDLRenderer3_NewFrame();
#ifdef ITU_SYS_RENDER_3D_IMPLEMENTATION
	else if(context->ctx_rendering)
		ImGui_ImplSDLGPU3_NewFrame();
#endif // ITU_SYS_RENDER_3D_IMPLEMENTATION
	else
	{
		// quick check to avoid spamming the console with warnings
		static int print_count = 0;
		if(print_count < 10)
			SDL_LogWarn(0, "cannot begin imgui frame, no valid rendering context");
		return;
	}


	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	if(context->debug_ui_show)
		ImGui::ShowDemoWindow();
}

inline void itu_lib_imgui_frame_end(EngineContext* context)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();

	if(context->renderer)
	{
		// NOTE: currently imgui doesn't handle renderer rescaling (see multiple issues like https://github.com/ocornut/imgui/issues/7433)
		//       current workaround involves resetting the render scale to the appropriate scale for imgui, render iu, then set back to
		//       out own zoom factor
		SDL_SetRenderScale(context->renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		ImGui_ImplSDLRenderer3_RenderDrawData(draw_data, context->renderer);
		SDL_SetRenderScale(context->renderer, context->zoom, context->zoom);
	}
#ifdef ITU_SYS_RENDER_3D_IMPLEMENTATION
	else if(context->ctx_rendering)
	{
		ITU_Renderer3D* ctx_rendering = context->ctx_rendering;		

		SDL_assert(ctx_rendering->device);
		if(ctx_rendering->color_target_info.texture)
		{
			ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, ctx_rendering->command_buffer_render);

			// Setup and start a render pass
			SDL_GPUColorTargetInfo target_info = {};
			target_info.texture = ctx_rendering->color_target_info.texture;
			target_info.load_op = SDL_GPU_LOADOP_LOAD;
			target_info.store_op = SDL_GPU_STOREOP_STORE;
			target_info.mip_level = 0;
			target_info.layer_or_depth_plane = 0;
			target_info.cycle = false;
			SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(ctx_rendering->command_buffer_render, &target_info, 1, NULL);
			ImGui_ImplSDLGPU3_RenderDrawData(draw_data, ctx_rendering->command_buffer_render, render_pass);
			SDL_EndGPURenderPass(render_pass);
		}
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}
#endif // ITU_SYS_RENDER_3D_IMPLEMENTATION
	else
	{
		// quick check to avoid spamming the console with warnings
		static int print_count = 0;
		if(print_count < 10)
			SDL_LogWarn(0, "cannot end imgui frame, no valid rendering context");
		return;
	}
}
