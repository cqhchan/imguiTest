#include <SDL.h>
#include "imgui.h"
#include <string>

#include "logger.h"

#ifdef __ANDROID__
#include <GLES2/gl2.h>
#include "imgui_impl_sdl_es2.h"
#include "imgui_impl_sdl_es3.h"
#else
#include "gl_glcore_3_3.h"
#include "imgui_impl_sdl_gl3.h"
#endif
#include "calculator.h"

#include <unistd.h>
#include <dirent.h>

/**
 * A convenience function to create a context for the specified window
 * @param w Pointer to SDL_Window
 * @return An SDL_Context value
 */

typedef bool(initImgui_t)(SDL_Window*);
typedef bool(processEvent_t)(SDL_Event*);
typedef void(newFrame_t)(SDL_Window*);
typedef void(shutdown_t)();

static initImgui_t *initImgui;
static processEvent_t *processEvent;
static newFrame_t *newFrame;
static shutdown_t *shutdown;


static int historySize = 0;
static std::string result[100];

static std::string equation[100];

static std::string currentEquation = "";
static std::string currentResult = "";

static void addStrToEquation(std::string toAdd){


    currentEquation += toAdd;
}


static SDL_GLContext createCtx(SDL_Window *w)
{
    // Prepare and create context
#ifdef __ANDROID__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GLContext ctx = SDL_GL_CreateContext(w);

    if (!ctx)
    {
        Log(LOG_ERROR) << "Could not create context! SDL reports error: " << SDL_GetError();
        return ctx;
    }

    int major, minor, mask;
    int r, g, b, a, depth;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

    SDL_GL_GetAttribute(SDL_GL_RED_SIZE,   &r);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE,  &b);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);

    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);

    const char* mask_desc;

    if (mask & SDL_GL_CONTEXT_PROFILE_CORE) {
        mask_desc = "core";
    } else if (mask & SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) {
        mask_desc = "compatibility";
    } else if (mask & SDL_GL_CONTEXT_PROFILE_ES) {
        mask_desc = "es";
    } else {
        mask_desc = "?";
    }

    Log(LOG_INFO) << "Got context: " << major << "." << minor << mask_desc
                  << ", R" << r << "G" << g << "B" << b << "A" << a << ", depth bits: " << depth;

    SDL_GL_MakeCurrent(w, ctx);
#ifdef __ANDROID__
    if (major == 3)
    {
        Log(LOG_INFO) << "Initializing ImGui for GLES3";
        initImgui = ImGui_ImplSdlGLES3_Init;
        Log(LOG_INFO) << "Setting processEvent and newFrame functions appropriately";
        processEvent = ImGui_ImplSdlGLES3_ProcessEvent;
        newFrame = ImGui_ImplSdlGLES3_NewFrame;
        shutdown = ImGui_ImplSdlGLES3_Shutdown;
    }
    else
    {
        Log(LOG_INFO) << "Initializing ImGui for GLES2";
        initImgui = ImGui_ImplSdlGLES2_Init;
        Log(LOG_INFO) << "Setting processEvent and newFrame functions appropriately";
        processEvent = ImGui_ImplSdlGLES2_ProcessEvent;
        newFrame = ImGui_ImplSdlGLES2_NewFrame;
        shutdown = ImGui_ImplSdlGLES2_Shutdown;
    }
#else
    initImgui = ImGui_ImplSdlGL3_Init;
    processEvent = ImGui_ImplSdlGL3_ProcessEvent;
    newFrame = ImGui_ImplSdlGL3_NewFrame;
    shutdown = ImGui_ImplSdlGL3_Shutdown;
#endif
    Log(LOG_INFO) << "Finished initialization";
    return ctx;
}


int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    if (argc < 2)
    {
        Log(LOG_FATAL) << "Not enough arguments! Usage: " << argv[0] << " path_to_data_dir";
        SDL_Quit();
        return 1;
    }
    if (chdir(argv[1])) {
        Log(LOG_ERROR) << "Could not change directory properly!";
    } else {
        dirent **namelist;
        int numdirs = scandir(".", &namelist, NULL, alphasort);
        if (numdirs < 0) {
            Log(LOG_ERROR) << "Could not list directory";
        } else {
            for (int dirid = 0; dirid < numdirs; ++dirid) {
                Log(LOG_INFO) << "Got file: " << namelist[dirid]->d_name;
            }
            free(namelist);
        }
    }

    // Create window
    Log(LOG_INFO) << "Creating SDL_Window";
    SDL_Window *window = SDL_CreateWindow("Demo App", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext ctx = createCtx(window);
    initImgui(window);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 32.0f);

    ImVec4 clear_color = ImColor(114, 144, 154);
    ImVec4 white = ImColor(255, 255, 255);
    ImVec4 darkRed = ImColor(0, 128, 0);

    Log(LOG_INFO) << "Entering main loop";
    {

        bool done = false;

        int deltaX = 0, deltaY = 0;
        int prevX , prevY;
        SDL_GetMouseState(&prevX, &prevY);

        while (!done) {
            SDL_Event e;

            newFrame(window);
            deltaX = 0;
            deltaY = 0;



            while (SDL_PollEvent(&e)) {
                bool handledByImGui = processEvent(&e);
                {
                    switch (e.type) {
                        case SDL_QUIT:
                            done = true;
                            break;
                        case SDL_MOUSEBUTTONDOWN:
                            prevX = e.button.x;
                            prevY = e.button.y;
                            break;
                        case SDL_MOUSEMOTION:

                            if (e.motion.y < ImGui::GetIO()
                                                     .DisplaySize.y/2 ) {
                                if (e.motion.state & SDL_BUTTON_LMASK) {
                                    deltaX += prevX - e.motion.x;
                                    deltaY += prevY - e.motion.y;
                                    prevX = e.motion.x;
                                    prevY = e.motion.y;
                                }
                            }

                            break;
                        case SDL_MULTIGESTURE:

                            break;
                        case SDL_MOUSEWHEEL:
                            break;
                        default:
                            break;
                    }
                }
            }


            ImGui::GetStateStorage()->SetInt(ImGui::GetID("Calculator"), 1);

            {

                ImGui::Begin("History");
                ImGui::SetWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
                ImGui::SetWindowCollapsed(false, ImGuiSetCond_Always);
                ImGui::SetWindowSize(ImVec2((int) ImGui::GetIO()
                        .DisplaySize.x, (int) ImGui::GetIO()
                        .DisplaySize.y - 150*7), ImGuiSetCond_Always);
                ImGui::BeginChild("scrolling", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing())
                        , true,
                                  ImGuiWindowFlags_AlwaysVerticalScrollbar);

                ImGui::SetWindowFontScale(3);
                ImGui::SetWindowPos(ImVec2(0, 50), ImGuiSetCond_Always);
                ImGui::SetWindowCollapsed(false, ImGuiSetCond_Always);
                ImGui::SetWindowSize(ImVec2((int) ImGui::GetIO()
                        .DisplaySize.x, (int) ImGui::GetIO()
                        .DisplaySize.y - 150*7), ImGuiSetCond_Always);


                if ((deltaY != 0)){

                    Log(LOG_INFO) << "Got moved: " << deltaY;
                    ImGui::SetScrollY(ImGui::GetScrollY() + deltaY);


                }



                const char* cstr = &currentEquation[0u];
                ImGui::Text("%s", cstr);
                const char* resultStr = &currentResult[0u];
                ImGui::Text("%s", resultStr);

                for (int i = historySize -1 ; i >= 0 ; i--){

                    if (equation[i].empty()) {

                        break;

                    }
                    if (result[i].empty()) {

                        break;

                    }
                    const char* tempEquation = &equation[i][0u];
                    const char* tempResult = &result[i][0u];


                    ImGui::TextColored(white, "%s", tempEquation);
                    ImGui::TextColored(darkRed, "%s", tempResult);


                }
                ImGui::EndChild();

                ImGui::End();

                ImGui::Begin("Calculator");
                ImGui::SetWindowFontScale(3);
                ImGui::SetWindowPos(ImVec2(0, (int) ImGui::GetIO()
                        .DisplaySize.y - 150*7), ImGuiSetCond_Always);
                ImGui::SetWindowCollapsed(false, ImGuiSetCond_Always);
                ImGui::SetWindowSize(ImVec2((int) ImGui::GetIO()
                        .DisplaySize.x, 150*7), ImGuiSetCond_Always);

                if(ImGui::Button("sin",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("sin");
                }; ImGui::SameLine();

                if(ImGui::Button("cos",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("cos");
                }; ImGui::SameLine();

                if(ImGui::Button("tan",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("tan");
                }; ImGui::SameLine();

                if(ImGui::Button("del",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    currentEquation.pop_back();
                }; ImGui::SameLine();

                ImGui::NewLine();

                if(ImGui::Button("c",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    currentEquation = "";
                }; ImGui::SameLine();
                ImGui::Button("(",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 )); ImGui::SameLine();
                ImGui::Button(")",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 )); ImGui::SameLine();
                if(ImGui::Button("/",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("/");
                };ImGui::SameLine();

                ImGui::NewLine();

                if(ImGui::Button("7",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("7");
                };
                ImGui::SameLine();
                if (ImGui::Button("8",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("8");
                }; ImGui::SameLine();
                if (ImGui::Button("9",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("9");
                }; ImGui::SameLine();
                if (ImGui::Button("X",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("X");
                }; ImGui::SameLine();

                ImGui::NewLine();

                if(ImGui::Button("4",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("4");
                };
                ImGui::SameLine();
                if (ImGui::Button("5",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("5");
                }; ImGui::SameLine();
                if (ImGui::Button("6",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("6");
                }; ImGui::SameLine();
                if (ImGui::Button("-",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("-");
                }; ImGui::SameLine();
                ImGui::NewLine();

                if(ImGui::Button("1",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("1");
                };
                ImGui::SameLine();
                if (ImGui::Button("2",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("2");
                }; ImGui::SameLine();
                if (ImGui::Button("3",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("3");
                }; ImGui::SameLine();
                if (ImGui::Button("+",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation("+");
                }; ImGui::SameLine();

                ImGui::NewLine();

                if (ImGui::Button("0",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/2 , 150 ))){
                    addStrToEquation("0");
                }; ImGui::SameLine();
                if (ImGui::Button(".",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){
                    addStrToEquation(".");
                }; ImGui::SameLine();
                if (ImGui::Button("=",ImVec2( (int) ImGui::GetIO()
                        .DisplaySize.x/4 , 150 ))){

                    std::ostringstream strs;
                    strs << calculator::calEverything(currentEquation);
                    currentResult = strs.str();

                    equation[historySize] = currentEquation;
                    currentEquation = "";
                    result[historySize] = currentResult;
                    currentResult = "";
                    historySize++;


                }; ImGui::SameLine();



                ImGui::End();
            }


            // Rendering
            glViewport(0, 0, (int) ImGui::GetIO().DisplaySize.x , (int) ImGui::GetIO()
                    .DisplaySize.y);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, (int) ImGui::GetIO().DisplaySize.x /2, (int) ImGui::GetIO()
                    .DisplaySize.y/2 );
            glClearColor(white.x, white.y, white.z, white.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ImGui::Render();
            SDL_GL_SwapWindow(window);
        }
    }
    shutdown();
    SDL_GL_DeleteContext(ctx);
    SDL_Quit();
    return 0;
}

