//
// OpenJK touch interface.
//
// Copied from psi/touch_interface_tfe.cpp as a starting point; to be tailored
// to OpenJK's controls/screens later.
//
extern "C"
{
extern int mobile_screen_width;
extern int mobile_screen_height;
}

#include "touch_interface.h"
#include "SDL_keycode.h"
#include <fstream>
#include <GLES/gl.h>

// Local UI code (not a game action) to toggle the force-select panel. Picked to
// not collide with the base KEY_* codes (0x1000-0x10013) or any PORT_ACT_*.
#define KEY_SHOW_FORCE 0x1100

// Alpha multiplier for a force button whose power the player hasn't learned yet
// (fraction of the panel's normal alpha). Available powers use 1.0 (unchanged).
#define FORCE_DIM_ALPHA 0.3f

// The 4x3 grid of force-power buttons, kept so we can dim the ones the player
// can't use. Paired with the PORT_ACT_FORCE_* used to query availability.
static struct
{
    touchcontrols::Button *button;
    int                    portAct;
} s_forceButtons[16];
static int s_forceButtonCount = 0;

// Add a force button to the panel and remember it for availability updates.
static void addForceButton(touchcontrols::TouchControls *tc, touchcontrols::Button *b, int portAct)
{
    tc->addControl(b);
    if(s_forceButtonCount < (int) (sizeof(s_forceButtons) / sizeof(s_forceButtons[0])))
    {
        s_forceButtons[s_forceButtonCount].button  = b;
        s_forceButtons[s_forceButtonCount].portAct = portAct;
        s_forceButtonCount++;
    }
}

// Dim the force buttons whose power the player doesn't currently have. Called
// whenever the force-select panel is shown (and while it stays open).
static void updateForceSelectAvailability()
{
    for(int i = 0; i < s_forceButtonCount; i++)
    {
        bool known = PortableGetForcePowerKnown(s_forceButtons[i].portAct);
        // Available -> full alpha (1.0, unchanged); unavailable -> dimmed.
        s_forceButtons[i].button->setAlpha(known ? 1.0f : FORCE_DIM_ALPHA);
    }
}

// OpenJK (rd-vanilla) runs the fixed-function GLES1 renderer and caches GL state
// in glState (tr_local.h): bound textures, active tmu, texEnv, cull, blend bits.
// GL_Bind/GL_State/GL_Cull skip the actual GL call whenever the cache already
// matches. The touch controls (gl_startRender + the button render) change blend,
// cull, alpha test, texture-2D enable, the client array states, texEnv, the bound
// texture, current colour and the viewport with raw GL calls behind that cache's
// back. The result is a stale cache that corrupts later draws that trust it - most
// visibly cinematics (RE_StretchRaw). gl_endRender only restores the matrices, so
// we snapshot the rest before rendering and put it back afterwards, keeping the
// real GL state consistent with the renderer's cache.
static struct
{
    GLboolean blend, cullFace, alphaTest, depthTest, scissorTest, texture2D;
    GLboolean vertexArray, texCoordArray, colorArray;
    GLint     blendSrc, blendDst;
    GLint     cullFaceMode;
    GLint     texEnvMode;
    GLint     texBinding;
    GLint     activeTexture, clientActiveTexture;
    GLint     viewport[4];
    GLfloat   color[4];
} s_savedGL;

static inline void setEnabled(GLenum cap, GLboolean on)
{
    if(on)
        glEnable(cap);
    else
        glDisable(cap);
}

static inline void setClientState(GLenum cap, GLboolean on)
{
    if(on)
        glEnableClientState(cap);
    else
        glDisableClientState(cap);
}

void TouchInterface::openGLStart()
{
    s_savedGL.blend        = glIsEnabled(GL_BLEND);
    s_savedGL.cullFace     = glIsEnabled(GL_CULL_FACE);
    s_savedGL.alphaTest    = glIsEnabled(GL_ALPHA_TEST);
    s_savedGL.depthTest    = glIsEnabled(GL_DEPTH_TEST);
    s_savedGL.scissorTest  = glIsEnabled(GL_SCISSOR_TEST);
    s_savedGL.texture2D    = glIsEnabled(GL_TEXTURE_2D);
    s_savedGL.vertexArray   = glIsEnabled(GL_VERTEX_ARRAY);
    s_savedGL.texCoordArray = glIsEnabled(GL_TEXTURE_COORD_ARRAY);
    s_savedGL.colorArray    = glIsEnabled(GL_COLOR_ARRAY);

    glGetIntegerv(GL_BLEND_SRC, &s_savedGL.blendSrc);
    glGetIntegerv(GL_BLEND_DST, &s_savedGL.blendDst);
    glGetIntegerv(GL_CULL_FACE_MODE, &s_savedGL.cullFaceMode);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &s_savedGL.texEnvMode);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &s_savedGL.texBinding);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &s_savedGL.activeTexture);
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &s_savedGL.clientActiveTexture);
    glGetIntegerv(GL_VIEWPORT, s_savedGL.viewport);
    glGetFloatv(GL_CURRENT_COLOR, s_savedGL.color);

    touchcontrols::gl_startRender();
    glDisable(GL_DEPTH_TEST);
};

void TouchInterface::openGLEnd()
{
    // Restores the projection/modelview matrices (and the framebuffer).
    touchcontrols::gl_endRender();

    // Put back the active/client-active units first so the texEnv and binding
    // restores below land on the unit they were captured from.
    glActiveTexture(s_savedGL.activeTexture);
    glClientActiveTexture(s_savedGL.clientActiveTexture);

    glBindTexture(GL_TEXTURE_2D, s_savedGL.texBinding);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, s_savedGL.texEnvMode);
    glBlendFunc(s_savedGL.blendSrc, s_savedGL.blendDst);
    glCullFace(s_savedGL.cullFaceMode);
    glViewport(s_savedGL.viewport[0], s_savedGL.viewport[1],
               s_savedGL.viewport[2], s_savedGL.viewport[3]);
    glColor4f(s_savedGL.color[0], s_savedGL.color[1],
              s_savedGL.color[2], s_savedGL.color[3]);

    setEnabled(GL_BLEND, s_savedGL.blend);
    setEnabled(GL_CULL_FACE, s_savedGL.cullFace);
    setEnabled(GL_ALPHA_TEST, s_savedGL.alphaTest);
    setEnabled(GL_DEPTH_TEST, s_savedGL.depthTest);
    setEnabled(GL_SCISSOR_TEST, s_savedGL.scissorTest);
    setEnabled(GL_TEXTURE_2D, s_savedGL.texture2D);

    setClientState(GL_VERTEX_ARRAY, s_savedGL.vertexArray);
    setClientState(GL_TEXTURE_COORD_ARRAY, s_savedGL.texCoordArray);
    setClientState(GL_COLOR_ARRAY, s_savedGL.colorArray);
};

void TouchInterface::mouseMove(int action, float x, float y, float mouse_x, float mouse_y)
{
    // Ignore the top where the buttons are
    if(y < (2.0 / 16.0))
        return;

    if(action == TOUCHMOUSE_MOVE)
    {
        MouseMove(mouse_x * mobile_screen_width, mouse_y * mobile_screen_height);
    }
    else if(action == TOUCHMOUSE_TAP)
    {
        if(0)
        {
            MouseMoveAbsolute(x * mobile_screen_width, y * mobile_screen_height);
        }

        MouseButton(1, BUTTON_PRIMARY);
        waitFrames(3);
        MouseButton(0, BUTTON_PRIMARY);
    }
}

void TouchInterface::createControls(std::string filesPath)
{
    tcMenuMain = new touchcontrols::TouchControls("menu", false, true, 10, true);
    tcYesNo = new touchcontrols::TouchControls("yes_no", false, false);
    tcGameMain = new touchcontrols::TouchControls("game", false, true, 1, true);
    tcGameWeapons = new touchcontrols::TouchControls("weapons", false, true, 1, false);
    tcInventory = new touchcontrols::TouchControls("inventory", false, true, 2, false); //Different edit group
    tcWeaponWheel = new touchcontrols::TouchControls("weapon_wheel", false, true, 1, false);
    // Hide the cog because when using the gamepad and weapon wheel is enabled, the cog will show otherwise
    tcWeaponWheel->hideEditButton = true;

    tcBlank = new touchcontrols::TouchControls("blank", true, false);
    tcCustomButtons = new touchcontrols::TouchControls("custom_buttons", false, true, 1, true);
    tcKeyboard = new touchcontrols::TouchControls("keyboard", false, false);
    //tcDemo = new touchcontrols::TouchControls("demo_playback",false,false);
    tcGamepadUtility = new touchcontrols::TouchControls("gamepad_utility", false, false);
    tcDPadInventory = new touchcontrols::TouchControls("dpad_inventory", false, false);
    tcMouse = new touchcontrols::TouchControls("mouse", false, false);
    tcForceSelect = new touchcontrols::TouchControls("force_select", false, true, 1);

    //Menu -------------------------------------------
    //------------------------------------------------------
    tcMenuMain->setFixAspect(true);
    tcMenuMain->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON));

    tcMenuMain->addControl(new touchcontrols::Button("keyboard", touchcontrols::RectF(2, 0, 4, 2), "keyboard", KEY_SHOW_KBRD));
    tcMenuMain->addControl(new touchcontrols::Button("console", touchcontrols::RectF(6, 0, 8, 2), "tild", PORT_ACT_CONSOLE));
    tcMenuMain->addControl(new touchcontrols::Button("show_custom", touchcontrols::RectF(9, 0, 11, 2), "custom_show", KEY_SHOW_CUSTOM));

    tcMenuMain->addControl(new touchcontrols::Button("gamepad", touchcontrols::RectF(22, 0, 24, 2), "gamepad", KEY_SHOW_GAMEPAD));
    tcMenuMain->addControl(new touchcontrols::Button("gyro", touchcontrols::RectF(24, 0, 26, 2), "gyro", KEY_SHOW_GYRO));
    tcMenuMain->addControl(new touchcontrols::Button("load_save_touch", touchcontrols::RectF(20, 0, 22, 2), "touchscreen_save", KEY_LOAD_SAVE_CONTROLS));

    // Left mouse button
    touchcontrols::Button *b = new touchcontrols::Button("left_mouse", touchcontrols::RectF(23, 4, 26, 7), "left_mouse", PORT_ACT_MOUSE_LEFT);
    b->setAllowPassThrough(false);
    tcMenuMain->addControl(b);

        b = new touchcontrols::Button("up_arrow", touchcontrols::RectF(0, 5, 2, 7), "arrow_up", PORT_ACT_MENU_UP);
        b->setAllowPassThrough(false);
        tcMenuMain->addControl(b);

        b = new touchcontrols::Button("down_arrow", touchcontrols::RectF(0, 8, 2, 10), "arrow_down", PORT_ACT_MENU_DOWN);
        b->setAllowPassThrough(false);
        tcMenuMain->addControl(b);

#if 0
    touchcontrols::Mouse *brightnessSlide = new touchcontrols::Mouse("slide_mouse", touchcontrols::RectF(24, 3, 26, 11), "brightness_slider");
    brightnessSlide->signal_action.connect(sigc::mem_fun(this, &TouchInterface:: brightnessSlideMouse));
    brightnessSlide->setAllowPassThrough(false);
    tcMenuMain->addControl(brightnessSlide);
#endif
    // Mouse at end
    touchcontrols::Mouse *mouse = new touchcontrols::Mouse("mouse", touchcontrols::RectF(0, 0, 26, 16), "");
    mouse->setHideGraphics(true);
    mouse->setEditable(false);
    tcMenuMain->addControl(mouse);
    mouse->signal_action.connect(sigc::mem_fun(this, &TouchInterface::mouseMove));

    tcMenuMain->signal_button.connect(sigc::mem_fun(this, &TouchInterface::menuButton));
    tcMenuMain->setAlpha(0.8);


    //Game -------------------------------------------
    //------------------------------------------------------
    // Jedi Academy in-game layout (ported from jk3_old/code/android/android-jni.cpp).
    tcGameMain->setAlpha(touchSettings.alpha);

    // Top row -----------------------------------------------------------------
    tcGameMain->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON, false, false, "Show menu"));
    tcGameMain->addControl(new touchcontrols::Button("saber_style", touchcontrols::RectF(4, 0, 6, 2), "saber_style", PORT_ACT_SABER_STYLE, false, false, "Saber style"));
    tcGameMain->addControl(new touchcontrols::Button("console", touchcontrols::RectF(6, 0, 8, 2), "tild", PORT_ACT_CONSOLE, false, true, "Console"));
    tcGameMain->addControl(new touchcontrols::Button("keyboard", touchcontrols::RectF(9, 0, 11, 2), "keyboard", KEY_SHOW_KBRD, false, false, "Show keyboard"));
    tcGameMain->addControl(new touchcontrols::Button("datapad", touchcontrols::RectF(14, 0, 16, 2), "gamma", PORT_ACT_DATAPAD, false, false, "Datapad"));
    tcGameMain->addControl(new touchcontrols::Button("use_inventory", touchcontrols::RectF(16, 0, 18, 2), "inv", KEY_SHOW_INV, false, false, "Show inventory"));
    tcGameMain->addControl(new touchcontrols::Button("quick_load", touchcontrols::RectF(20, 0, 22, 2), "load", PORT_ACT_QUICKLOAD, false, false, "Quick load"));
    tcGameMain->addControl(new touchcontrols::Button("quick_save", touchcontrols::RectF(24, 0, 26, 2), "save", PORT_ACT_QUICKSAVE, false, false, "Quick save"));

    // Right-hand action cluster ----------------------------------------------
    tcGameMain->addControl(new touchcontrols::Button("attack", touchcontrols::RectF(23, 7, 26, 10), "shoot", KEY_SHOOT, false, false, "Attack"));
    tcGameMain->addControl(new touchcontrols::Button("alt_attack", touchcontrols::RectF(20, 7, 23, 10), "shoot_alt", PORT_ACT_ALT_ATTACK, false, false, "Alt attack"));
    tcGameMain->addControl(new touchcontrols::Button("use_force", touchcontrols::RectF(23, 5, 25, 7), "shoot_force", PORT_ACT_FORCE_USE, false, false, "Use force"));
    tcGameMain->addControl(new touchcontrols::Button("use", touchcontrols::RectF(21, 5, 23, 7), "use", PORT_ACT_USE, false, false, "Use/Open"));
    tcGameMain->addControl(new touchcontrols::Button("jump", touchcontrols::RectF(24, 3, 26, 5), "jump", PORT_ACT_JUMP, false, false, "Jump"));
    tcGameMain->addControl(new touchcontrols::Button("crouch", touchcontrols::RectF(24, 14, 26, 16), "crouch", PORT_ACT_DOWN, false, false, "Crouch"));

    // Left-hand weapon cycle --------------------------------------------------
    tcGameMain->addControl(new touchcontrols::Button("next_weapon", touchcontrols::RectF(0, 3, 3, 5), "next_weap", PORT_ACT_NEXT_WEP, false, false, "Next weapon"));
    tcGameMain->addControl(new touchcontrols::Button("prev_weapon", touchcontrols::RectF(0, 7, 3, 9), "prev_weap", PORT_ACT_PREV_WEP, false, false, "Prev weapon"));

    // Bottom row: weapon numbers + force cycle + force-select panel toggle -----
    tcGameMain->addControl(new touchcontrols::Button("prev_force", touchcontrols::RectF(8, 14, 10, 16), "prev_force", PORT_ACT_PREV_FORCE, false, false, "Prev force"));
    tcGameMain->addControl(new touchcontrols::Button("force_select", touchcontrols::RectF(10, 14, 12, 16), "quick_force", KEY_SHOW_FORCE, false, false, "Force powers"));
    tcGameMain->addControl(new touchcontrols::Button("show_weapons", touchcontrols::RectF(12, 14, 14, 16), "show_weapons", KEY_SHOW_WEAPONS, false, false, "Show weapons"));
    tcGameMain->addControl(new touchcontrols::Button("next_force", touchcontrols::RectF(16, 14, 18, 16), "next_force", PORT_ACT_NEXT_FORCE, false, false, "Next force"));

    touchJoyRight = new touchcontrols::TouchJoy("touch", touchcontrols::RectF(17, 4, 26, 16), "look_arrow", "fixed_stick_circle");
    tcGameMain->addControl(touchJoyRight);
    touchJoyRight->signal_move.connect(sigc::mem_fun(this, &TouchInterface::rightStick));
    touchJoyRight->signal_double_tap.connect(sigc::mem_fun(this, &TouchInterface::rightDoubleTap));

    touchJoyLeft = new touchcontrols::TouchJoy("stick", touchcontrols::RectF(0, 7, 8, 16), "strafe_arrow", "fixed_stick_circle");
    tcGameMain->addControl(touchJoyLeft);
    touchJoyLeft->signal_move.connect(sigc::mem_fun(this, &TouchInterface::leftStick));
    touchJoyLeft->signal_double_tap.connect(sigc::mem_fun(this, &TouchInterface::leftDoubleTap));

    // SWAPFIX
    touchJoyLeft->registerTouchJoySWAPFIX(touchJoyRight);
    touchJoyRight->registerTouchJoySWAPFIX(touchJoyLeft);

    tcGameMain->signal_button.connect(sigc::mem_fun(this, &TouchInterface::gameButton));
    tcGameMain->signal_settingsButton.connect(sigc::mem_fun(this, &TouchInterface::gameSettingsButton));

    // Also now allow menu to enter the settings
    tcMenuMain->signal_settingsButton.connect(sigc::mem_fun(this, &TouchInterface::gameSettingsButton));


    //Weapons -------------------------------------------
    //------------------------------------------------------
    tcGameWeapons->addControl(new touchcontrols::Button("weapon1", touchcontrols::RectF(1, 14, 3, 16), "key_1", 1));
    tcGameWeapons->addControl(new touchcontrols::Button("weapon2", touchcontrols::RectF(3, 14, 5, 16), "key_2", 2));
    tcGameWeapons->addControl(new touchcontrols::Button("weapon3", touchcontrols::RectF(5, 14, 7, 16), "key_3", 3));
    tcGameWeapons->addControl(new touchcontrols::Button("weapon4", touchcontrols::RectF(7, 14, 9, 16), "key_4", 4));
    tcGameWeapons->addControl(new touchcontrols::Button("weapon5", touchcontrols::RectF(9, 14, 11, 16), "key_5", 5));

    tcGameWeapons->addControl(new touchcontrols::Button("weapon6", touchcontrols::RectF(15, 14, 17, 16), "key_6", 6));
    tcGameWeapons->addControl(new touchcontrols::Button("weapon7", touchcontrols::RectF(17, 14, 19, 16), "key_7", 7));
    tcGameWeapons->addControl(new touchcontrols::Button("weapon8", touchcontrols::RectF(19, 14, 21, 16), "key_8", 8));
    tcGameWeapons->addControl(new touchcontrols::Button("weapon9", touchcontrols::RectF(21, 14, 23, 16), "key_9", 9));
    tcGameWeapons->addControl(new touchcontrols::Button("weapon0", touchcontrols::RectF(23, 14, 25, 16), "key_0", 0));

    tcGameWeapons->signal_button.connect(sigc::mem_fun(this, &TouchInterface::selectWeaponButton));
    tcGameWeapons->setAlpha(0.8);

    //Weapon wheel -------------------------------------------
    //------------------------------------------------------
    wheelSelect = new touchcontrols::WheelSelect("weapon_wheel", touchcontrols::RectF(7, 2, 19, 14), "weapon_wheel_rtcw", 10);
    wheelSelect->signal_selected.connect(sigc::mem_fun(this, &TouchInterface::weaponWheel));
    wheelSelect->signal_enabled.connect(sigc::mem_fun(this, &TouchInterface::weaponWheelSelected));
    tcWeaponWheel->addControl(wheelSelect);

    if(touchSettings.weaponWheelOpaque)
        tcWeaponWheel->setAlpha(0.8);
    else
        tcWeaponWheel->setAlpha(touchSettings.alpha);

    // Force select -------------------------------------------
    //------------------------------------------------------
    // Pop-up grid of every force power (ported from jk3_old/android-jni.cpp).
    // Hidden by default; toggled by the KEY_SHOW_FORCE button on the game screen.
    s_forceButtonCount = 0; // reset in case controls get rebuilt
    // Light side
    addForceButton(tcForceSelect, new touchcontrols::Button("force_absorb",  touchcontrols::RectF(4, 2, 7, 5),   "f_lt_absorb",  PORT_ACT_FORCE_ABSORB),  PORT_ACT_FORCE_ABSORB);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_heal",    touchcontrols::RectF(4, 5, 7, 8),   "f_lt_heal",    PORT_ACT_FORCE_HEAL),    PORT_ACT_FORCE_HEAL);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_mind",    touchcontrols::RectF(4, 8, 7, 11),  "f_lt_mind",    PORT_ACT_FORCE_MIND),    PORT_ACT_FORCE_MIND);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_protect", touchcontrols::RectF(4, 11, 7, 14), "f_lt_protect", PORT_ACT_FORCE_PROTECT), PORT_ACT_FORCE_PROTECT);
    // Neutral
    addForceButton(tcForceSelect, new touchcontrols::Button("force_pull",    touchcontrols::RectF(7, 2, 10, 5),   "force_pull",  PORT_ACT_FORCE_PULL),  PORT_ACT_FORCE_PULL);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_speed",   touchcontrols::RectF(7, 5, 10, 8),   "force_speed", PORT_ACT_FORCE_SPEED), PORT_ACT_FORCE_SPEED);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_push",    touchcontrols::RectF(7, 8, 10, 11),  "force_push",  PORT_ACT_FORCE_PUSH),  PORT_ACT_FORCE_PUSH);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_sight",   touchcontrols::RectF(7, 11, 10, 14), "force_sence", PORT_ACT_FORCE_SIGHT), PORT_ACT_FORCE_SIGHT);
    // Dark side
    addForceButton(tcForceSelect, new touchcontrols::Button("force_drain",     touchcontrols::RectF(10, 2, 13, 5),   "f_dk_drain",     PORT_ACT_FORCE_DRAIN), PORT_ACT_FORCE_DRAIN);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_grip",      touchcontrols::RectF(10, 5, 13, 8),   "f_dk_grip",      PORT_ACT_FORCE_GRIP),  PORT_ACT_FORCE_GRIP);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_lightning", touchcontrols::RectF(10, 8, 13, 11),  "f_dk_lightning", PORT_ACT_FORCE_LIGHT), PORT_ACT_FORCE_LIGHT);
    addForceButton(tcForceSelect, new touchcontrols::Button("force_rage",      touchcontrols::RectF(10, 11, 13, 14), "f_dk_rage",      PORT_ACT_FORCE_RAGE),  PORT_ACT_FORCE_RAGE);

    tcForceSelect->signal_button.connect(sigc::mem_fun(this, &TouchInterface::forceSelectButton));
    tcForceSelect->setAlpha(0.8);

    // Inventory -------------------------------------------
    //------------------------------------------------------

    uiInventoryButtonGrid = new touchcontrols::ButtonGrid("inventory_grid", touchcontrols::RectF(3, 9, 11, 11), "inventory_bg", 4, 1);
    uiInventoryButtonGrid->addCell(0, 0, "inventory_left", PORT_ACT_INVPREV);
    uiInventoryButtonGrid->addCell(1, 0, "inventory_right", PORT_ACT_INVNEXT);
    uiInventoryButtonGrid->addCell(2, 0, "inventory_use", PORT_ACT_INVUSE);
    uiInventoryButtonGrid->addCell(3, 0, "inventory_drop", PORT_ACT_INVDROP);


    uiInventoryButtonGrid->signal_outside.connect(sigc::mem_fun(this, &TouchInterface::inventoryOutside));

    tcInventory->addControl(uiInventoryButtonGrid);
    tcInventory->setPassThroughTouch(touchcontrols::TouchControls::PassThrough::NO_CONTROL);
    tcInventory->signal_button.connect(sigc::mem_fun(this, &TouchInterface::inventoryButton));
    tcInventory->setAlpha(0.9);

    //Blank -------------------------------------------
    //------------------------------------------------------
    tcBlank->addControl(new touchcontrols::Button("enter", touchcontrols::RectF(0, 0, 26, 16), "", 0x123));
    tcBlank->signal_button.connect(sigc::mem_fun(this, &TouchInterface::blankButton));

    //Keyboard -------------------------------------------
    //------------------------------------------------------
    uiKeyboard = new touchcontrols::UI_Keyboard("keyboard", touchcontrols::RectF(0, 8, 26, 16), "font_dual", 0, 0, 0);
    uiKeyboard->signal.connect(sigc::mem_fun(this, &TouchInterface::keyboardKeyPressed));
    tcKeyboard->addControl(uiKeyboard);
    // We want touch to pass through only where there is no keyboard
    tcKeyboard->setPassThroughTouch(touchcontrols::TouchControls::PassThrough::NO_CONTROL);

    //Yes No -------------------------------------------
    //------------------------------------------------------
    tcYesNo->addControl(new touchcontrols::Button("yes", touchcontrols::RectF(8, 12, 11, 15), "key_y", PORT_ACT_MENU_CONFIRM));
    tcYesNo->addControl(new touchcontrols::Button("no", touchcontrols::RectF(15, 12, 18, 15), "key_n", PORT_ACT_MENU_ABORT));
    tcYesNo->signal_button.connect(sigc::mem_fun(this, &TouchInterface::menuButton));
    tcYesNo->setAlpha(0.8);

    //Custom Controls -------------------------------------------
    //------------------------------------------------------
    tcCustomButtons->addControl(new touchcontrols::Button("A", touchcontrols::RectF(5, 5, 7, 7), "Custom_1", PORT_ACT_CUSTOM_0, false, false, "Custom 1 (H)", touchcontrols::COLOUR_RED2));
    tcCustomButtons->addControl(new touchcontrols::Button("B", touchcontrols::RectF(7, 5, 9, 7), "Custom_2", PORT_ACT_CUSTOM_1, false, false, "Custom 2 (I)", touchcontrols::COLOUR_RED2));
    tcCustomButtons->addControl(new touchcontrols::Button("C", touchcontrols::RectF(5, 7, 7, 9), "Custom_3", PORT_ACT_CUSTOM_2, false, false, "Custom 3 (J)", touchcontrols::COLOUR_BLUE1));

    tcCustomButtons->addControl(new touchcontrols::Button("D", touchcontrols::RectF(7, 7, 9, 9), "Custom_4", PORT_ACT_CUSTOM_3, false, false, "Custom 4 (K)", touchcontrols::COLOUR_BLUE1));
    tcCustomButtons->addControl(new touchcontrols::Button("E", touchcontrols::RectF(5, 9, 7, 11), "Custom_5", PORT_ACT_CUSTOM_4, false, false, "Custom 5 (L)", touchcontrols::COLOUR_GREEN2));
    tcCustomButtons->addControl(new touchcontrols::Button("F", touchcontrols::RectF(7, 9, 9, 11), "Custom_6", PORT_ACT_CUSTOM_5, false, false, "Custom 6 (M)", touchcontrols::COLOUR_GREEN2));

    tcCustomButtons->addControl(new touchcontrols::Button("G", touchcontrols::RectF(5, 11, 7, 13), "custom_a", PORT_ACT_CUSTOM_6, false, true, "Custom 7 (N)", touchcontrols::COLOUR_NONE));
    tcCustomButtons->addControl(new touchcontrols::Button("H", touchcontrols::RectF(7, 11, 9, 13), "custom_b", PORT_ACT_CUSTOM_7, false, true, "Custom 8 (O)", touchcontrols::COLOUR_NONE));
    tcCustomButtons->addControl(new touchcontrols::Button("I", touchcontrols::RectF(5, 13, 7, 15), "custom_c", PORT_ACT_CUSTOM_8, false, true, "Custom 9 (P)", touchcontrols::COLOUR_NONE));
    tcCustomButtons->addControl(new touchcontrols::Button("J", touchcontrols::RectF(7, 13, 9, 15), "custom_d", PORT_ACT_CUSTOM_9, false, true, "Custom 10 (Q)", touchcontrols::COLOUR_NONE));

    touchcontrols::QuadSlide *qs1 = new touchcontrols::QuadSlide("quad_slide_1", touchcontrols::RectF(10, 7, 12, 9), "quad_slide", "slide_arrow", PORT_ACT_CUSTOM_10, PORT_ACT_CUSTOM_11,
                                                                 PORT_ACT_CUSTOM_12, PORT_ACT_CUSTOM_13, false, "Quad Slide 1 (R - U)");
    tcCustomButtons->addControl(qs1);

    touchcontrols::QuadSlide *qs2 = new touchcontrols::QuadSlide("quad_slide_2", touchcontrols::RectF(14, 7, 16, 9), "quad_slide", "slide_arrow", PORT_ACT_CUSTOM_14, PORT_ACT_CUSTOM_15,
                                                                 PORT_ACT_CUSTOM_16, PORT_ACT_CUSTOM_17, false, "Quad Slide 2 (V - Y)");
    tcCustomButtons->addControl(qs2);

    //tcCustomButtons->setColor(0.7,0.7,1.f);

    qs1->signal.connect(sigc::mem_fun(this, &TouchInterface::customButton));
    qs2->signal.connect(sigc::mem_fun(this, &TouchInterface::customButton));

    tcCustomButtons->signal_button.connect(sigc::mem_fun(this, &TouchInterface::customButton));
    tcCustomButtons->signal_settingsButton.connect(sigc::mem_fun(this, &TouchInterface::customSettingsButton));
    tcCustomButtons->setAlpha(touchSettings.alpha);

    //Gamepad utility -------------------------------------------
    //------------------------------------------------------
    touchcontrols::ButtonGrid *gamepadUtils = new touchcontrols::ButtonGrid("gamepad_grid", touchcontrols::RectF(8, 5, 18, 11), "gamepad_utils_bg", 3, 2);

    gamepadUtils->addCell(0, 0, "back_button", KEY_BACK_BUTTON);
    gamepadUtils->addCell(0, 1, "scores", PORT_ACT_MP_SCORES);
    gamepadUtils->addCell(1, 0, "keyboard", KEY_SHOW_KBRD);
    gamepadUtils->addCell(1, 1, "tild", PORT_ACT_CONSOLE);
    gamepadUtils->addCell(2, 0, "save", PORT_ACT_QUICKSAVE);
    gamepadUtils->addCell(2, 1, "load", PORT_ACT_QUICKLOAD);

    gamepadUtils->signal_outside.connect(sigc::mem_fun(this, &TouchInterface::gameUtilitiesOutside));

    tcGamepadUtility->addControl(gamepadUtils);
    tcGamepadUtility->setAlpha(0.9);
    tcGamepadUtility->signal_button.connect(sigc::mem_fun(this, &TouchInterface::gameUtilitiesButton));

    // DPad inventory -------------------------------------------
    //------------------------------------------------------
    touchcontrols::DPadSelect *dpadInventory = new touchcontrols::DPadSelect("dpad_inventory", touchcontrols::RectF(9, 4, 17, 12), "dpad", PORT_ACT_INVUSE);

    dpadInventory->addCell(touchcontrols::DPAD_LEFT, "inventory_left", PORT_ACT_INVPREV);
    dpadInventory->addCell(touchcontrols::DPAD_RIGHT, "inventory_right", PORT_ACT_INVNEXT);
    dpadInventory->addCell(touchcontrols::DPAD_DOWN, "inventory_drop", PORT_ACT_INVDROP);

    dpadInventory->signal_button.connect(sigc::mem_fun(this, &TouchInterface::dPadInventoryButton));
    dpadInventory->signal_outside.connect(sigc::mem_fun(this, &TouchInterface::dPadInventoryOutside));

    tcDPadInventory->addControl(dpadInventory);
    tcDPadInventory->setAlpha(0.9);

    // Mouse -------------------------------------------
    //------------------------------------------------------
    touchcontrols::Mouse *mouseMouseMenu = new touchcontrols::Mouse("mouse", touchcontrols::RectF(0, 2, 26, 16), "");
    mouseMouseMenu->setHideGraphics(true);
    mouseMouseMenu->setEditable(false);
    tcMouse->addControl(mouseMouseMenu);
    mouseMouseMenu->signal_action.connect(sigc::mem_fun(this, &TouchInterface::mouseMove));

    tcMouse->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON, false, false, "Back"));
    tcMouse->addControl(new touchcontrols::Button("hide_mouse", touchcontrols::RectF(4, 0, 6, 2), "mouse2", KEY_USE_MOUSE, false, false, "Hide mouse"));
    tcMouse->setAlpha(0.9);
    tcMouse->signal_button.connect(sigc::mem_fun(this, &TouchInterface::mouseButton));


    //---------------------------------------------------------------
    //---------------------------------------------------------------
    controlsContainer.addControlGroup(tcKeyboard);
    controlsContainer.addControlGroup(tcInventory); // before gamemain so touches don't go through
    controlsContainer.addControlGroup(tcGamepadUtility); // before gamemain so touches don't go through
    controlsContainer.addControlGroup(tcDPadInventory);
    controlsContainer.addControlGroup(tcCustomButtons);
    controlsContainer.addControlGroup(tcGameMain);
    controlsContainer.addControlGroup(tcYesNo);
    controlsContainer.addControlGroup(tcGameWeapons);

    controlsContainer.addControlGroup(tcMenuMain);
    controlsContainer.addControlGroup(tcWeaponWheel);
    controlsContainer.addControlGroup(tcForceSelect);
    controlsContainer.addControlGroup(tcBlank);
    controlsContainer.addControlGroup(tcMouse);

    std::string
    newSettings = (std::string) filesPath + "/touch_settings_"
    ENGINE_NAME
    ".xml";

    UI_tc = touchcontrols::createDefaultSettingsUI(&controlsContainer, newSettings);
    UI_tc->setAlpha(1);

    tcMenuMain->setXMLFile((std::string) filesPath + "/menu_rtcw.xml");
    tcGameMain->setXMLFile((std::string) filesPath + "/game_"
    ENGINE_NAME
    ".xml");
    tcInventory->setXMLFile((std::string) filesPath + "/inventory_"
    ENGINE_NAME
    ".xml");
    tcWeaponWheel->setXMLFile((std::string) filesPath + "/weaponwheel_"
    ENGINE_NAME
    ".xml");
    tcGameWeapons->setXMLFile((std::string) filesPath + "/weapons_"
    ENGINE_NAME
    ".xml");
    tcCustomButtons->setXMLFile((std::string) filesPath + "/custom_buttons_0_"
    ENGINE_NAME
    ".xml");
    tcForceSelect->setXMLFile((std::string) filesPath + "/forceselect_"
    ENGINE_NAME
    ".xml");

    enableReloadSniperMode = true; // Enable reload/sniper mode
}


void TouchInterface::blankButton(int state, int code)
{
    // Shown during cinematics (TS_BLANK): a tap sends Enter to advance/skip.
    PortableKeyEvent(state, SDL_SCANCODE_RETURN, 0);
}

void TouchInterface::automapButton(int state, int code)
{

}

void TouchInterface::newFrame()
{
    touchscreemode_t screenMode = PortableGetScreenMode();

    // Hack to show custom buttons while in the menu to bind keys
    if(screenMode == TS_MENU && showCustomMenu == true)
    {
        screenMode = TS_CUSTOM;
    }
/*

        if((screenMode == TS_MENU) && (useMouse || gotMouseMove))
            controlsContainer.showMouse(true);
        else
        {
            useMouse = false;
            gotMouseMove = false;
            controlsContainer.showMouse(false);
        }
*/

    // The base mode logic doesn't know about our extra force-select panel, so
    // make sure it never lingers once we leave the game screen.
    if(screenMode != TS_GAME && tcForceSelect && tcForceSelect->enabled)
        tcForceSelect->setEnabled(false);

    // Keep the force buttons' dim state current while the panel is open (the
    // player may learn a power, e.g. via a level-up, with the panel showing).
    if(screenMode == TS_GAME && tcForceSelect && tcForceSelect->enabled)
        updateForceSelectAvailability();

    updateTouchScreenModeOut(screenMode);
    updateTouchScreenModeIn(screenMode);

    currentScreenMode = screenMode;
}

void TouchInterface::newGLContext()
{

}

// Adds the force-select pop-up toggle on top of the base game-button handling.
void TouchInterface::gameButton(int state, int code)
{
    if(code == KEY_SHOW_FORCE)
    {
        if(state == 1 && tcForceSelect)
        {
            if(!tcForceSelect->enabled)
            {
                // Dim the powers the player doesn't have before showing the panel.
                updateForceSelectAvailability();
                tcForceSelect->animateIn(5);
            }
            else
                tcForceSelect->animateOut(5);
        }
        return;
    }

    TouchInterfaceBase::gameButton(state, code);
}

// A force power was chosen from the pop-up: fire it, then close the panel when
// the button is released.
void TouchInterface::forceSelectButton(int state, int code)
{
    PortableAction(state, code);

    if(!state && tcForceSelect)
        tcForceSelect->animateOut(5);
}
