#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android/log.h>
#include "android_native_app_glue.h"

struct user_data {
    char app_name[64];
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

    uint program;
    uint a_pos_id;
    uint a_tex_coord_id;
    uint texture_id;
    uint sampler_id;

    uint8_t *texture_buffer;
};

char *cmd_names[] = {
    "APP_CMD_INPUT_CHANGED",
    "APP_CMD_INIT_WINDOW",
    "APP_CMD_TERM_WINDOW",
    "APP_CMD_WINDOW_RESIZED",
    "APP_CMD_WINDOW_REDRAW_NEEDED",
    "APP_CMD_CONTENT_RECT_CHANGED",
    "APP_CMD_GAINED_FOCUS",
    "APP_CMD_LOST_FOCUS",
    "APP_CMD_CONFIG_CHANGED",
    "APP_CMD_LOW_MEMORY",
    "APP_CMD_START",
    "APP_CMD_RESUME",
    "APP_CMD_SAVE_STATE",
    "APP_CMD_PAUSE",
    "APP_CMD_STOP",
    "APP_CMD_DESTROY",
};

void init(android_app *app)
{
    user_data *p = (user_data *)app->userData;

    p->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(p->display, 0, 0);

    int attrib_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig config;
    int num_config;
    eglChooseConfig(p->display, attrib_list, &config, 1, &num_config);

    int format;
    eglGetConfigAttrib(p->display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);
    p->surface = eglCreateWindowSurface(p->display, config, app->window, 0);

    const int context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    eglBindAPI(EGL_OPENGL_ES_API);
    p->context = eglCreateContext(p->display, config, EGL_NO_CONTEXT, context_attribs);

    eglMakeCurrent(p->display, p->surface, p->surface, p->context);

    p->program = glCreateProgram();
    char *vertex_shader_source =
        "attribute vec2 a_pos; \n"
        "attribute vec2 a_tex_coord; \n"
        "varying vec2 v_tex_coord; \n"
        "void main() \n"
        "{ \n"
        " gl_Position = vec4(a_pos, 0, 1); \n"
        " v_tex_coord = a_tex_coord; \n"
        "} \n";


    char *fragment_shader_source =
        "precision mediump float;\n"
        "varying vec2 v_tex_coord;\n"
        "uniform sampler2D tex;\n"
        "void main() \n"
        "{ \n"
        " gl_FragColor = texture2D( tex, v_tex_coord );\n"
        "} \n";

    uint vertex_shader_id;
    uint fragment_shader_id;

    {
        uint shader = vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        int compiled;
        glShaderSource(shader, 1, (const char* const *)&vertex_shader_source, 0);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char info_log[1024];
            glGetShaderInfoLog(shader, sizeof(info_log), 0, info_log);
            __android_log_print(ANDROID_LOG_INFO, p->app_name, "vertex shader failed to compile: %s", info_log);
        }
    }
    {
        uint shader = fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        int compiled;
        glShaderSource(shader, 1, (const char* const *)&fragment_shader_source, 0);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char info_log[1024];
            glGetShaderInfoLog(shader, sizeof(info_log), 0, info_log);
            __android_log_print(ANDROID_LOG_INFO, p->app_name, "fragment shader failed to compile: %s", info_log);
        }
    }
    glAttachShader(p->program, vertex_shader_id);
    glAttachShader(p->program, fragment_shader_id);
    glLinkProgram(p->program);

    int linked;
    glGetProgramiv(p->program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char info_log[1024];
        glGetProgramInfoLog(p->program, sizeof(info_log), 0, info_log);
        __android_log_print(ANDROID_LOG_INFO, p->app_name, "program failed to link: %s", info_log);
    }

    glUseProgram(p->program);
    p->a_pos_id = glGetAttribLocation(p->program, "a_pos");
    p->a_tex_coord_id = glGetAttribLocation(p->program, "a_tex_coord");
    p->sampler_id = glGetAttribLocation(p->program, "tex");
    glEnableVertexAttribArray(p->a_pos_id);
    glEnableVertexAttribArray(p->a_tex_coord_id);

    glGenTextures(1, &p->texture_id);
    glBindTexture(GL_TEXTURE_2D, p->texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        960, 540, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, p->texture_buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
}

void term(android_app *app)
{
    user_data *p = (user_data *)app->userData;
    eglMakeCurrent(p->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(p->display, p->context);
    eglDestroySurface(p->display, p->surface);
    eglTerminate(p->display);
}

void on_app_cmd(android_app *app, int32_t cmd) {
    user_data *p = (user_data *)app->userData;
    if (cmd < sizeof(cmd_names))
    {
        __android_log_print(ANDROID_LOG_INFO, p->app_name, "cmd is %s", cmd_names[cmd]);
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, p->app_name, "unknown cmd is %d", cmd);
    }
    if (cmd == APP_CMD_INIT_WINDOW)
    {
        init(app);
    }
    if (cmd == APP_CMD_TERM_WINDOW)
    {
        term(app);
    }
    if (cmd == APP_CMD_DESTROY)
    {
        exit(0);
    }
}

int32_t on_input_event(android_app *app, AInputEvent *event) {
    user_data *p = (user_data *)app->userData;
    int event_type = AInputEvent_getType(event);

    switch (event_type)
    {
        case AINPUT_EVENT_TYPE_KEY:
        {
            __android_log_print(ANDROID_LOG_INFO, p->app_name, "event_type was AINPUT_EVENT_TYPE_KEY");
            break;
        }
        case AINPUT_EVENT_TYPE_MOTION:
        {
            __android_log_print(ANDROID_LOG_INFO, p->app_name, "event_type was AINPUT_EVENT_TYPE_MOTION");
            break;
        }
        default:
        {
            __android_log_print(ANDROID_LOG_INFO, p->app_name, "unknown event_type was %d", event_type);
            break;
        }
    }
    return 0;
}

void draw(android_app *app)
{
    user_data *p = (user_data *)app->userData;
    static uint8_t grey_value = 0;
    grey_value += 1;

    glClearColor(grey_value / 255.0, grey_value / 255.0, grey_value / 255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    float vertexCoords[] =
    {
        -.75, .75,
        -.75, -.75,
        .75, .75,
        .75, -.75,
    };
    float texCoords[] =
    {
        0.0, 0.0,
        0.0, 1.0,
        1.0, 0.0,
        1.0, 1.0,
    };

    uint16_t indices[] = { 0, 1, 2, 1, 2, 3 };

    glUseProgram(p->program);
    glVertexAttribPointer(p->a_pos_id, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertexCoords);
    glVertexAttribPointer(p->a_tex_coord_id, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), texCoords);
    glEnableVertexAttribArray(p->a_pos_id);
    glEnableVertexAttribArray(p->a_tex_coord_id);
    glBindTexture(GL_TEXTURE_2D, p->texture_id);
    glTexSubImage2D(GL_TEXTURE_2D,
        0,
        0,
        0,
        960,
        540,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        p->texture_buffer);

    glUniform1i(p->sampler_id, 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

    eglSwapBuffers(p->display, p->surface);
}

void android_main(android_app *app) {
    app_dummy();

    user_data p = {};
    p.texture_buffer = (uint8_t *)malloc(4 * 960 * 540);
    strcpy(p.app_name, "org.nxsy.ndk_handmade");
    app->userData = &p;

    app->onAppCmd = on_app_cmd;
    app->onInputEvent = on_input_event;
    uint64_t counter;
    uint start_row = 0;
    uint start_col = 0;
    while (++counter) {
        timespec start_time = {};
        clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

        int poll_result, events;
        android_poll_source *source;

        while((poll_result = ALooper_pollAll(0, 0, &events, (void**)&source)) >= 0)
        {
            source->process(app, source);
        }

        switch (poll_result)
        {
            case ALOOPER_POLL_WAKE:
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_WAKE");
                break;
            }
            case ALOOPER_POLL_CALLBACK:
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_CALLBACK");
                break;
            }
            case ALOOPER_POLL_TIMEOUT:
            {
                //__android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_TIMEOUT");
                break;
            }
            case ALOOPER_POLL_ERROR:
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_ERROR");
                break;
            }
            default:
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was %d", poll_result);
                break;
            }
        }

        bzero(p.texture_buffer, 4 * 960 * 540);

        for (uint row = start_row;
            row < start_row + 100;
            ++row)
        {
            uint8_t *buffer_row = p.texture_buffer + (row * 4 * 960);
            for (uint col = start_col;
                col < start_col + 20;
                ++col)
            {
                uint8_t *pixel = buffer_row + (col * 4);
                *pixel = 255;
            }
        }

        start_row += 40;
        start_row %= 400;
        start_col += 3;
        start_col %= 900;

        draw(app);

        timespec end_time = {};
        clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

        end_time.tv_sec -= start_time.tv_sec;
        start_time.tv_sec -= start_time.tv_sec;

        int64_t time_taken = ((end_time.tv_sec * 1000000000 + end_time.tv_nsec) -
            (start_time.tv_sec * 1000000000 + start_time.tv_nsec));

        int64_t time_to_sleep = 33 * 1000000;
        if (time_taken <= time_to_sleep)
        {
            timespec sleep_time = {};
            sleep_time.tv_nsec = time_to_sleep - time_taken;
            timespec remainder = {};
            nanosleep(&sleep_time, &remainder);
        }
        else
        {
            if (counter % 10 == 0)
            {
                __android_log_print(ANDROID_LOG_INFO, p.app_name, "Skipped frame!  Took %" PRId64 " ns total", time_taken);
            }
        }

    }
}