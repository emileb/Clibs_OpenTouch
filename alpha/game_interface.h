#include "port_act_defs.h"

#ifdef __IOS__
#define LOGI printf
#endif

#ifndef LOGI
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"JNITouchControlsUtils", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "JNITouchControlsUtils", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"JNITouchControlsUtils", __VA_ARGS__))
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
	TS_BLANK,
	TS_MENU,
	TS_GAME,
	TS_MAP,
	TS_CONSOLE,
    TS_Y_N,
	TS_CUSTOM,
	TS_DEMO,
	TS_MOUSE,
} touchscreemode_t;


void PortableInit(int argc,const char ** argv);


void PortableBackButton();

int PortableKeyEvent(int state, int code ,int unitcode);
void PortableAction(int state, int action);

void PortableMove(float fwd, float strafe);
void PortableMoveFwd(float fwd);
void PortableMoveSide(float strafe);
void PortableLookPitch(int mode, float pitch);
void PortableLookYaw(int mode, float pitch);

void PortableMouse(float dx,float dy);
void PortableMouseButton(int state, int button, float dx,float dy);

void PortableCommand(const char * cmd);
void PortableAutomapControl(float zoom, float x, float y);
int PortableShowKeyboard(void);
touchscreemode_t PortableGetScreenMode();


#ifdef __cplusplus
}
#endif
