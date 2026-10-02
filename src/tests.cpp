/* 02_hello_sdl.cpp
 * 
 * Basic test to check that SDL is working correctly on your machine
 * 
 * author: chris
 */


#include <itu_engine.hpp>
#include <glm/glm.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

int main(void)
{
    glm::mat4 m;
    



    EngineConfig config;
    EngineContext context = { 0 };

    itu_lib_context_init(&config, &context);

    const char* window_title = "Hello SDL";
    SDL_Log("Starting %s\n", window_title);

    SDL_Window*   window;
    SDL_Renderer* renderer;

    SDL_CreateWindowAndRenderer(
        window_title,
        800, 600, 0,
        &window,
        &renderer
    );

    SDL_VALIDATE_PANIC(MIX_Init());
    SDL_VALIDATE_PANIC(MIX_CreateMixer(NULL));
    SDL_VALIDATE_PANIC(TTF_Init());
    SDL_VALIDATE_PANIC(TTF_CreateRendererTextEngine(context.renderer));

    rapidjson::Document d;



    bool quit = false;

    SDL_Event event;
    while(!quit)
    {
        // stop game when `x` button is pressed on the window border UI
        while(SDL_PollEvent(&event))
            if(event.type == SDL_EVENT_QUIT)
                quit = true;

        // clear the drawing surface to a dark blue
        SDL_SetRenderDrawColor(renderer, 0x0C, 0x42, 0xA1, 0x00);
        SDL_RenderClear(renderer);

        // write debug text
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderDebugText(renderer, 370, 290, "Hello SDL!");

        // show frame
        SDL_RenderPresent(renderer);

        // wait some time
        SDL_Delay(16);
    }
}
