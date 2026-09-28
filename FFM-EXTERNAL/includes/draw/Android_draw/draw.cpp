#include "Android_draw/draw.h"
#include "font/font_data.hpp"
#include "protect/oxorany.hpp"
#include "ui/styles/styles.h"
#include <android/log.h>

EGLDisplay display = EGL_NO_DISPLAY;
EGLConfig config;
EGLSurface surface = EGL_NO_SURFACE;
EGLContext context = EGL_NO_CONTEXT;

ANativeWindow *native_window;
ImFont* fontBold;
ImFont* fontMedium;
ImFont* fontDesc;
ImFont* espFont;

int native_window_screen_x = 0;
int native_window_screen_y = 0;
android::ANativeWindowCreator::DisplayInfo displayInfo{0};
uint32_t orientation = 0;
bool g_Initialized = false;
ImGuiWindow *g_window = nullptr;

bool initGUI_draw(uint32_t _screen_x, uint32_t _screen_y, bool log) {
    orientation = displayInfo.orientation;

    if (!init_egl(_screen_x, _screen_y, log)) {
        return false;
    }

    if (!ImGui_init()) {
        return false;
    }

    return true;
}

bool init_egl(uint32_t _screen_x, uint32_t _screen_y, bool log) {
    ::native_window = android::ANativeWindowCreator::Create("Overlay", _screen_x, _screen_y, false);

    ANativeWindow_acquire(native_window);
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        return false;
    }

    if (eglInitialize(display, 0, 0) != EGL_TRUE) {
        return false;
    }

    EGLint num_config = 0;
    const EGLint attribList[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 5,
        EGL_GREEN_SIZE, 6,
        EGL_RED_SIZE, 5,
        EGL_BUFFER_SIZE, 32,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    const EGLint attrib_list[] = {
        EGL_CONTEXT_CLIENT_VERSION,
        3,
        EGL_NONE
    };

    if (eglChooseConfig(display, attribList, &config, 1, &num_config) != EGL_TRUE) {
        return false;
    }

    EGLint egl_format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &egl_format);
    ANativeWindow_setBuffersGeometry(native_window, 0, 0, egl_format);
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrib_list);
    if (context == EGL_NO_CONTEXT) {
        return false;
    }

    surface = eglCreateWindowSurface(display, config, native_window, nullptr);
    if (surface == EGL_NO_SURFACE) {
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        return false;
    }

    return true;
}

void screen_config() {
    displayInfo = android::ANativeWindowCreator::GetDisplayInfo();
}

bool ImGui_init() {
    if (g_Initialized) {
        return true;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    UI::ApplyGothTheme();
    ImGui_ImplAndroid_Init(native_window);
    ImGui_ImplOpenGL3_Init(oxorany("#version 300 es"));

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;

    ImFontConfig config;
    config.FontDataOwnedByAtlas  = false;
    config.OversampleH           = 3;      // subpixel horiz
    config.OversampleV           = 2;      // subpixel vert
    config.PixelSnapH            = false;  // deixa o glyph em posição sub-pixel
    config.RasterizerMultiply    = 1.15f;  // um toque mais sólido
    config.GlyphExtraSpacing.x   = 0.2f;

    // Wide glyph range: Basic Latin + Latin Ext-A/B + Cyrillic + Vietnamese +
    // punctuation + Turkish/German diacritics — covers 99% of FF nicknames.
    static const ImWchar kRange[] = {
        0x0020, 0x00FF,   // Basic Latin + Latin-1 Supplement
        0x0100, 0x017F,   // Latin Extended-A
        0x0180, 0x024F,   // Latin Extended-B
        0x0300, 0x036F,   // Combining Diacritics
        0x0400, 0x04FF,   // Cyrillic
        0x1E00, 0x1EFF,   // Latin Extended Additional (Vietnamese)
        0x2000, 0x206F,   // General Punctuation
        0x2070, 0x209F,   // Super/Subscripts
        0x20A0, 0x20CF,   // Currency
        0x2100, 0x214F,   // Letter-like symbols
        0x2190, 0x21FF,   // Arrows
        0x2500, 0x257F,   // Box drawing
        0x25A0, 0x25FF,   // Geometric shapes
        0
    };

    config.SizePixels = 40.0f;
    fontBold = io.Fonts->AddFontFromMemoryTTF((void*)font_ttf, font_ttf_len, config.SizePixels, &config, kRange);

    config.SizePixels = 32.0f;
    fontMedium = io.Fonts->AddFontFromMemoryTTF((void*)font_ttf, font_ttf_len, config.SizePixels, &config, kRange);

    config.SizePixels = 26.0f;
    fontDesc = io.Fonts->AddFontFromMemoryTTF((void*)font_ttf, font_ttf_len, config.SizePixels, &config, kRange);

    // ESP font: menor, mais leve (RasterizerMultiply neutro pra ficar fino)
    config.RasterizerMultiply    = 1.00f;
    config.OversampleH           = 4;
    config.SizePixels = 18.0f;
    espFont = io.Fonts->AddFontFromMemoryTTF((void*)font_ttf, font_ttf_len, config.SizePixels, &config);

    io.FontDefault = fontMedium;
    io.Fonts->Build();
    ImGui::GetStyle().ScaleAllSizes(4.0f);
    
    ::g_Initialized = true;
    return true;
}

void drawBegin() {
    screen_config();
    if (::orientation != displayInfo.orientation) {
        ::orientation = displayInfo.orientation;
        touch::update(displayInfo.width, displayInfo.height, displayInfo.orientation);
        if (g_window) {
            g_window->Pos.x = 100;
            g_window->Pos.y = 125;
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(native_window_screen_x, native_window_screen_y);
    ImGui::NewFrame();
}

void drawEnd() {
    ImGui::Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    static int _fr=0, _err=0;
    EGLBoolean ok = eglSwapBuffers(display, surface);
    if (!ok) _err++;
    if ((_fr++ % 300)==0) {
        EGLint e = eglGetError();
        __android_log_print(ANDROID_LOG_INFO, "ZqwDraw",
            "frame %d swap=%d egl_err=0x%x total_swap_err=%d surf=%p disp=%p",
            _fr, (int)ok, e, _err, surface, display);
    }
}

void shutdown() {
    if (!g_Initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        eglTerminate(display);
    }
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;

    ANativeWindow_release(native_window);
    android::ANativeWindowCreator::Destroy(native_window);
    ::g_Initialized = false;
}