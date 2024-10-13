#include <RootTask.h>

#include <rio.h>
#include <gfx/rio_Window.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten/emscripten.h>
    #include <gfx/lyr/rio_Renderer.h>
    #include <gfx/rio_Window.h>
    #include <task/rio_TaskMgr.h>
#endif

static rio::InitializeArg initializeArg = {
    .window = {
        .width = 600,
        .height = 600,
#if RIO_IS_WIN
        .resizable = true
#endif // RIO_IS_WIN
    }
};


#ifdef __EMSCRIPTEN__
rio::Window* window;
void mainLoop() {
    // Main loop iteration
    // Update the task manager
    rio::TaskMgr::instance()->calc();

    // Render
    rio::lyr::Renderer::instance()->render();

    // Swap the front and back buffers
    window->swapBuffers();
}
#endif

int main()
{
    char* isServerOnly = getenv("SERVER_ONLY");
    // use invisible window with server only
    if (isServerOnly)
        initializeArg.window.invisible = true;

#ifdef __EMSCRIPTEN__
    // Get window instance
    window = rio::Window::instance();
    emscripten_set_main_loop(mainLoop, 0, 1);
#endif

    // Initialize RIO, make window...
    if (!rio::Initialize<RootTask>(initializeArg))
        return -1;
#ifndef __EMSCRIPTEN__
    // do not draw/present to window when server only
    // may avoid an issue where, when window hasn't been updated
    // for a while (which it won't be if SERVER_ONLY is enabled
    // , since the program blocks on accept() all day)...
    // , the program may hang next time it calls swapBuffers()
    // due to a call to wl_display_dispatch_queue_pending() hanging
    if (isServerOnly)
    {
        rio::Window* window = rio::Window::instance();
        // Main loop
        while (window->isRunning())
        {
            // Update the task manager
            rio::TaskMgr::instance()->calc();

            // Render
            //lyr::Renderer::instance()->render();

            // Swap the front and back buffers
            //window->swapBuffers();
        }
    } else {
        // Main loop
        rio::EnterMainLoop();
    }

    // Exit RIO
    rio::Exit();
#endif // __EMSCRIPTEN__
    return 0;
}
