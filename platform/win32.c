#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <windows.h>

#include "app.h"
#include "instance.h"
#include "log.h"
#include "object.h"
#include "platform.h"
#include "window.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

struct platform_mmap_ {
    const char* data;
    HANDLE file;
    HANDLE map;
    size_t size;
};

platform_mmap_t* platform_mmap(const char* filename) {
    HANDLE file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        log_error("Could not open file (%lu)", GetLastError());
        return NULL;
    }
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size)) {
        log_error("Could not get file size (%lu)", GetLastError());
        CloseHandle(file);
        return NULL;
    }

    HANDLE map = CreateFileMappingA(file, NULL, PAGE_EXECUTE_READ, 0, 0, NULL);
    if (map == INVALID_HANDLE_VALUE) {
        log_error("Could not mmap file (%lu)", GetLastError());
        CloseHandle(file);
        return NULL;
    }
    OBJECT_ALLOC(platform_mmap);
    platform_mmap->file = file;
    platform_mmap->map = map;
    platform_mmap->data = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    platform_mmap->size = size.QuadPart;
    return platform_mmap;
}

size_t platform_mmap_size(platform_mmap_t* m) {
    return m->size;
}

const char* platform_mmap_data(platform_mmap_t* m) {
    return m->data;
}

void platform_munmap(platform_mmap_t* m) {
    UnmapViewOfFile(m->data);
    CloseHandle(m->map);
    CloseHandle(m->file);
    free(m);
}

int64_t platform_get_time(void) {
    FILETIME t;
    GetSystemTimePreciseAsFileTime(&t);
    LARGE_INTEGER i;
    i.LowPart = t.dwLowDateTime;
    i.HighPart = t.dwHighDateTime;
    return i.QuadPart / 10;
}

void platform_set_terminal_color(FILE* f, platform_terminal_color_t c) {
    (void)f;
    (void)c;
}

void platform_clear_terminal_color(FILE* f) {
    (void)f;
}

struct platform_mutex_ {
    CRITICAL_SECTION data;
};

platform_mutex_t* platform_mutex_new() {
    OBJECT_ALLOC(platform_mutex);
    InitializeCriticalSection(&platform_mutex->data);
    return platform_mutex;
}

void platform_mutex_delete(platform_mutex_t* mutex) {
    DeleteCriticalSection(&mutex->data);
    free(mutex);
}

int platform_mutex_lock(platform_mutex_t* mutex) {
    EnterCriticalSection(&mutex->data);
    return 0;
}

int platform_mutex_unlock(platform_mutex_t* mutex) {
    LeaveCriticalSection(&mutex->data);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

struct platform_cond_ {
    CONDITION_VARIABLE data;
};

platform_cond_t* platform_cond_new() {
    OBJECT_ALLOC(platform_cond);
    InitializeConditionVariable(&platform_cond->data);
    return platform_cond;
}

void platform_cond_delete(platform_cond_t* cond) {
    free(cond);
}

int platform_cond_wait(platform_cond_t* cond,
                       platform_mutex_t* mutex)
{
    return !SleepConditionVariableCS(&cond->data, &mutex->data, INFINITE);
}

int platform_cond_broadcast(platform_cond_t* cond) {
    WakeAllConditionVariable(&cond->data);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

struct platform_thread_ {
    HANDLE data;
};

/* The pthreads API (and thus the platform_thread API) expects a function
 * returning void*, while the win32 API returns DWORD.  We use this small
 * struct to translate from one to the other. */
typedef struct {
    void *(*run)(void *);
    void* data;
} thread_data_t;

DWORD WINAPI thread_runner(LPVOID data_) {
    thread_data_t* data = data_;
    (data->run)(data->data);
    free(data_);
    return 0;
}

platform_thread_t*  platform_thread_new(void *(*run)(void *),
                                        void* data)
{
    OBJECT_ALLOC(platform_thread);

    thread_data_t* d = calloc(1, sizeof(thread_data_t));
    d->run = run;
    d->data = data;

    platform_thread->data = CreateThread(NULL, 0, &thread_runner, d, 0, NULL);
    if (platform_thread->data == NULL) {
        free(platform_thread);
        return NULL;
    }
    return platform_thread;
}

void platform_thread_delete(platform_thread_t* thread) {
    CloseHandle(thread->data);
    free(thread);
}

int platform_thread_join(platform_thread_t* thread) {
    return WaitForSingleObject(thread->data, INFINITE);
}

////////////////////////////////////////////////////////////////////////////////

static app_t* app_handle = NULL;
void platform_init(app_t* app, int argc, char** argv) {
    app_handle = app;
    if (argc == 2) {
        app_open(app, argv[1]);
    }
    if (app->instance_count == 0) {
        //  Construct a dummy window, which triggers GLFW initialization
        //  and may cause the application to open a file (if it was
        //  double-clicked or dragged onto the icon).
        window_new("", 1.0f, 1.0f);

        if (app->instance_count == 0) {
            app_open(app, ":/sphere");
        }
    }
}

#define ID_FILE_OPEN            9001
#define ID_FILE_EXIT            9002
#define ID_VIEW_SHADED          9003
#define ID_VIEW_WIREFRAME       9004
#define ID_VIEW_ORTHOGRAPHIC    9005
#define ID_VIEW_PERSPECTIVE     9006

/*  We hot-swap the WNDPROC pointer from the one defined in GLFW to our
 *  own here, which lets us respond to menu events (ignored in GLFW). */
static WNDPROC glfw_wndproc = NULL;
static LRESULT CALLBACK wndproc(HWND hWnd, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
    if (message == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case ID_FILE_OPEN: {
                OPENFILENAME ofn;       // common dialog box structure
                TCHAR szFile[260] = { 0 };       // if using TCHAR macros

                // Initialize OPENFILENAME
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = ("STL model\0*.stl\0");
                ofn.nFilterIndex = 1;
                ofn.lpstrFileTitle = NULL;
                ofn.nMaxFileTitle = 0;
                ofn.lpstrInitialDir = NULL;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                if (GetOpenFileName(&ofn) == TRUE) {
                    app_open(app_handle, ofn.lpstrFile);
                }
                break;
            }
            case ID_FILE_EXIT:
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                break;

            case ID_VIEW_SHADED:
                CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1),
                        ID_VIEW_SHADED, MF_CHECKED);
                CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1),
                        ID_VIEW_WIREFRAME, MF_UNCHECKED);
                instance_view_shaded(app_get_front(app_handle));
                break;

            case ID_VIEW_WIREFRAME:
                CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1),
                        ID_VIEW_SHADED, MF_UNCHECKED);
                CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1),
                        ID_VIEW_WIREFRAME, MF_CHECKED);
                instance_view_wireframe(app_get_front(app_handle));
                break;

            case ID_VIEW_ORTHOGRAPHIC:
                CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1),
                        ID_VIEW_PERSPECTIVE, MF_UNCHECKED);
                CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1),
                        ID_VIEW_ORTHOGRAPHIC, MF_CHECKED);
                instance_view_orthographic(app_get_front(app_handle));
                break;

            case ID_VIEW_PERSPECTIVE:
                CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1),
                        ID_VIEW_ORTHOGRAPHIC, MF_UNCHECKED);
                CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1),
                        ID_VIEW_PERSPECTIVE, MF_CHECKED);
                instance_view_perspective(app_get_front(app_handle));
                break;
        }
    } else if (message == WM_CHAR && wParam == 'o' - 'a' + 1) {
        PostMessage(hWnd, WM_COMMAND, ID_FILE_OPEN, 0L);
    }
    return (*glfw_wndproc)(hWnd, message, wParam, lParam);
}

void platform_window_bind(GLFWwindow* window) {
    HWND w = glfwGetWin32Window(window);
    if (glfw_wndproc == NULL) {
        glfw_wndproc = (WNDPROC)GetWindowLongPtrW(w, GWLP_WNDPROC);
    }
    SetWindowLongPtrW(w, GWLP_WNDPROC, (LONG_PTR)wndproc);

    HMENU menu = CreateMenu();
    HMENU file = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, ID_FILE_OPEN, L"&Open\tCtrl+O");
    AppendMenuW(file, MF_STRING, ID_FILE_EXIT, L"E&xit");
    AppendMenu(menu, MF_STRING | MF_POPUP, (UINT_PTR)file, "File");

    HMENU view = CreatePopupMenu();
    AppendMenuW(view, MF_STRING, ID_VIEW_SHADED, L"Shaded");
    CheckMenuItem(view, ID_VIEW_SHADED, MF_CHECKED);
    AppendMenuW(view, MF_STRING, ID_VIEW_WIREFRAME, L"Wireframe");

    AppendMenuW(view, MF_SEPARATOR, 0, NULL);

    AppendMenuW(view, MF_STRING, ID_VIEW_ORTHOGRAPHIC, L"Orthographic");
    CheckMenuItem(view, ID_VIEW_ORTHOGRAPHIC, MF_CHECKED);
    AppendMenuW(view, MF_STRING, ID_VIEW_PERSPECTIVE, L"Perspective");

    AppendMenu(menu, MF_STRING | MF_POPUP, (UINT_PTR)view, "View");

    SetMenu(w, menu);
}

/*  Shows a warning dialog with the given text */
void platform_warning(const char* title, const char* text) {
    log_error(text);
}

/*  Returns the filename portion of a full path */
const char* platform_filename(const char* filepath) {
    return filepath;
}
