#include <RootTask.h>

#include <rio.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten/emscripten.h>
#endif

static const rio::InitializeArg cInitializeArg = {
    .window = {
        .width = 600,
        .height = 600,
#if RIO_IS_WIN
        .resizable = true
#endif // RIO_IS_WIN
    }
};


#ifdef __EMSCRIPTEN__
void mainLoop() {
    rio::EnterMainLoop();
}
#endif

int main()
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 30, 0);
#endif
    // Initialize RIO with root task
    if (!rio::Initialize<RootTask>(cInitializeArg))
        return -1;
#ifndef __EMSCRIPTEN__
    // Main loop
    rio::EnterMainLoop();

    // Exit RIO
    rio::Exit();
#endif
    return 0;
}
