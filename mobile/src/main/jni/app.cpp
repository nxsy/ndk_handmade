#include <stdlib.h>
#include <string.h>

#include <android/log.h>
#include "android_native_app_glue.h"

struct user_data {
    char app_name[64];
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

void android_main(android_app *app) {
    app_dummy();

    user_data p = {};
    strcpy(p.app_name, "org.nxsy.ndk_handmade");
    app->userData = &p;

    app->onAppCmd = on_app_cmd;
    app->onInputEvent = on_input_event;

    while (1) {
        int poll_result, events;
        android_poll_source *source;
        poll_result = ALooper_pollAll(-1, 0, &events, (void**)&source);
        if (poll_result >= 0)
        {
            source->process(app, source);
        }
        else
        {
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
                    __android_log_print(ANDROID_LOG_INFO, p.app_name, "poll_result was ALOOPER_POLL_TIMEOUT");
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
        }
    }
}