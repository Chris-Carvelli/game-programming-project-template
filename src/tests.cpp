/* 02_hello_sdl.cpp
 * 
 * Basic test to check that SDL is working correctly on your machine
 * 
 * author: chris
 */


#include <itu_engine.hpp>

int main(void)
{
    glm::mat4 m;

    EngineConfig config;
    EngineContext context = { 0 };

    itu_lib_context_init(&config, &context);

    MIX_Mixer* mixer;
    MIX_Track* track;
    MIX_Audio* audio;

    TTF_TextEngine* engine;
    TTF_Font* font;
    TTF_Text* text;

    SDL_PropertiesID id_audio_props = SDL_CreateProperties();
    SDL_SetNumberProperty(id_audio_props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);

    SDL_VALIDATE_PANIC(MIX_Init());
    SDL_VALIDATE_PANIC(mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL));
    SDL_VALIDATE_PANIC(track = MIX_CreateTrack(mixer));
    SDL_VALIDATE_PANIC(audio = MIX_LoadAudio(mixer, "data/footstep00.ogg", true));
    SDL_VALIDATE_PANIC(MIX_SetTrackAudio(track, audio));
    SDL_VALIDATE_PANIC(MIX_PlayTrack(track, id_audio_props));

    SDL_VALIDATE_PANIC(TTF_Init());
    SDL_VALIDATE_PANIC(engine = TTF_CreateRendererTextEngine(context.renderer));
    SDL_VALIDATE_PANIC(font = TTF_OpenFont("data/CASCADIAMONO.ttf", 12));
    SDL_VALIDATE_PANIC(text = TTF_CreateText(engine, font, "I am Rendering with a font!", 0));

    const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
    rapidjson::Document d;
    d.Parse(json);

    rapidjson::Value& s = d["stars"];
    s.SetInt(s.GetInt() + 1);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);


    bool quit = false;

    SDL_Event event;
    while(!quit)
    {

        quit = itu_lib_input_process_events(&context);

        // clear the drawing surface to a dark blue
        SDL_SetRenderDrawColor(context.renderer, 0x0C, 0x42, 0xA1, 0xFF);
        SDL_RenderClear(context.renderer);

        // write debug text
        SDL_SetRenderDrawColor(context.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderDebugText(context.renderer, 10, 10, "Font string expected: 'I am Rendering with a Font!'");
        SDL_RenderDebugText(context.renderer, 10, 30, "Mont string actual  :");
        SDL_VALIDATE_PANIC(TTF_DrawRendererText(text, 180, 25));

        SDL_RenderDebugText(context.renderer, 10, 70, "Musing playing: footstep00.ogg, in an infinite loop");

        char buf[256];
        sprintf(buf, "                                      %s", json);
        SDL_RenderDebugText(context.renderer, 10,  90, "Content of 'data/test.json' original: ");
        SDL_RenderDebugText(context.renderer, 10,  90, buf);

        sprintf(buf, "                                      %s", buffer.GetString());
        SDL_RenderDebugText(context.renderer, 10, 100, "Content of 'data/test.json' modified: ");
        SDL_RenderDebugText(context.renderer, 10, 100, buf);

        SDL_RenderDebugText(context.renderer, 10, 120, "glm::mat4 identity:");
        SDL_RenderDebugText(context.renderer, 25, 130, glm::to_string(glm::identity<glm::mat4>()).c_str());


        // show frame
        SDL_RenderPresent(context.renderer);

        // wait some time
        // SDL_Delay(16);
    }
}
