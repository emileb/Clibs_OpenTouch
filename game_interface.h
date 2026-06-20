#ifndef game_interface_h
#define game_interface_h

#include "port_act_defs.h"
#include <stdbool.h>

#ifndef LOGI
#ifdef __ANDROID__
#ifdef LOGI
undef LOGI
#endif
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
    TS_PDA, // Doom 3
} touchscreemode_t;


void PortableInit(int argc, const char **argv);

void PortableBackButton(void);

int PortableKeyEvent(int state, int code, int unitcode);

void PortableAction(int state, int action);

void PortableMove(float fwd, float strafe);

void PortableMoveFwd(float fwd);

void PortableMoveSide(float strafe);

void PortableLookPitch(int mode, float pitch);

void PortableLookYaw(int mode, float pitch);

void PortableMouse(float dx, float dy);

void PortableMouseAbs(float x, float y);

void PortableMouseButton(int state, int button, float dx, float dy);

void PortableCommand(const char *cmd);

void PortableAutomapControl(float zoom, float x, float y);

// TFE-only: drain the accumulated PortableAutomapControl() pan/zoom and apply it to
// the DarkForces automap. Must be called on the engine main thread (TFE calls it from
// pda_handleInput()). Only implemented by the TFE build.
void PortableApplyMapTouch(void);

int PortableShowKeyboard(void);

bool PortableSetAlwaysRun(bool run);

touchscreemode_t PortableGetScreenMode();

// TFE-only: returns non-zero while a DarkForces-rendered in-mission menu (escape
// menu, agent menu, mission briefing, PDA) is showing, as opposed to the ImGui
// frontend menus (main menu / config). Both report TS_MENU, so the touch layer
// needs this to tell them apart and switch the on-screen mouse from relative-drag
// (good for the small ImGui widgets) to absolute tap-to-position (good for the
// large DOS-style menu buttons). When non-null, *mouseOffsetX receives the menu's
// horizontal pillarbox offset in device pixels: these menus render in a centred
// box, so the touch layer subtracts it from the absolute mouse X to land the tap
// under the finger. Only implemented by the TFE build.
int PortableInGameMenu(float *mouseOffsetX);

// TFE-only: returns non-zero while the ImGui frontend's top-level main menu (the
// large Start/Settings/... image buttons) is showing — not its config/mods/manual
// sub-screens. The touch layer uses this to enable absolute tap-to-position on the
// big main-menu buttons (which are drawn in full window pixels, so no offset),
// while the small-widget sub-screens stay on relative drag. Only implemented by
// the TFE build.
int PortableFrontendMenu(void);

// TFE-only: select the in-game-menu mouse style. enable != 0 → tap-to-position
// (teleport the cursor onto the tapped button and hide the cursor); 0 → relative
// drag of a visible cursor. PortableGetMouseTapMode() reads the current state.
void PortableSetMouseTapMode(int enable);

int PortableGetMouseTapMode(void);

// OpenJK-only: in tap-to-select mode, teleport the menu cursor straight onto a
// tapped point (normalized screen fraction [0,1]) before the click. The engine
// maps it into its resolution-independent UI canvas. Engines that don't implement
// it simply never call it.
void PortableSetMenuCursorPos(float fracX, float fracY);

// Whether the player currently has the given force power available, keyed by the
// PORT_ACT_FORCE_* action code. Engine-specific (implemented by OpenJK); lets the
// touch UI dim force buttons for powers the player hasn't learned. Engines that
// don't implement it simply never call it.
bool PortableGetForcePowerKnown(int forceAction);

int isPlayerRunning();

#define ACTION_DOWN 0
#define ACTION_UP 1
#define ACTION_MOVE 2
#define BUTTON_PRIMARY 1
#define BUTTON_SECONDARY 2

extern void MouseButton(int state, int button);

extern void MouseMove(float dx, float dy);

extern void MouseMoveAbsolute(float dx, float dy);

#ifdef __cplusplus
}
#endif


#endif