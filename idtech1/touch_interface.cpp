
#include "TouchControlsInterface.h"
#include "TouchControlsContainer.h"
#include "SDL_beloko_extra.h"
#include "SDL.h"
//#include "ios_interface.h"

#if defined(GZDOOM_GL3)
   #include <GLES3/gl3.h>
#else
    #include <GLES/gl.h>
#endif

#include "UI_TouchDefaultSettings.h"
#include "UI_ButtonListWindow.h"

#include "touch_interface.h"

#include <jni.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"touch_interface", __VA_ARGS__))

// Stupid LOCAL_SHORT_COMMANDS loses quotes on command line
#ifdef GZDOOM_GL3
#define ENGINE_NAME "gzdoom_dev"
#endif

extern "C"
{

#ifdef CHOC_SETUP
    #define DEFAULT_FADE_FRAMES 0
#else
    #define DEFAULT_FADE_FRAMES 10
#endif

#include "game_interface.h"
#include "SDL_keycode.h"

int mobile_screen_width;
int mobile_screen_height;


touchscreemode_t currentScreenMode = TS_BLANK;

#define KEY_SHOW_WEAPONS 0x1000
#define KEY_SHOOT        0x1001

#define KEY_SHOW_INV     0x1006
#define KEY_QUICK_CMD    0x1007
#define KEY_SHOW_KBRD    0x1008
#define KEY_SHOW_CUSTOM  0x1009

#define KEY_F10  0x100A
#define KEY_BACK_BUTTON  0x100B
#define KEY_SHOW_GYRO    0x100C
#define KEY_SHOW_GAMEPAD 0x100D

#define GAME_OPTION_AUTO_HIDE_GAMEPAD   0x1
#define GAME_OPTION_HIDE_MENU_AND_GAME  0x2
#define GAME_OPTION_USE_SYSTEM_KEYBOARD 0x4
#define GAME_OPTION_GLES2               0x8

#define GAME_TYPE_DOOM     1 // Dont use 0 so we can detect serialization
#define GAME_TYPE_HEXEN    2
#define GAME_TYPE_HERETIC  3
#define GAME_TYPE_STRIFE   4

static int gameType;

static float gameControlsAlpha = 0.5;
static bool joystickLookMode = false;
static bool invertLook = false;
static bool precisionShoot = false;
static bool showSticks = false;
static bool gamepadHideTouch = false;
static bool autoHideInventory = true;
static bool autoHideNumbers = true;
static bool weaponWheelEnabled = true;
static bool hideGameAndMenu = false;
static int left_double_action;
static int right_double_action;


//To be set by android
static float strafe_sens = 1;
static float forward_sens = 1;
static float pitch_sens = 1;
static float yaw_sens = 1;

static float precisionSensitivty = 0.5;

static bool m_shooting = false;

// Show buttons in game
static bool showCustomOn = false;

// Show custom buttons in the menu
static bool showCustomMenu = false;

static bool useSystemKeyboard = false;

static int controlsCreated = 0;
static touchcontrols::TouchControlsContainer controlsContainer;

static int mapState = 0;

#define DEMO_ALPHA_RESET      30
#define DEMO_ALPFA_DEC        0.1f
static float demoControlsAlpha; // Used to fade out demo controls

touchcontrols::UI_Controls *UI_tc = 0;

touchcontrols::TouchControls *tcMenuMain=0;
touchcontrols::TouchControls *tcYesNo=0;
touchcontrols::TouchControls *tcGameMain=0;
touchcontrols::TouchControls *tcGameWeapons=0;
touchcontrols::TouchControls *tcWeaponWheel=0;
touchcontrols::TouchControls *tcInventory=0;
touchcontrols::TouchControls *tcAutomap=0;
touchcontrols::TouchControls *tcBlank=0;
touchcontrols::TouchControls *tcKeyboard=0;
touchcontrols::TouchControls *tcCutomButtons=0;
touchcontrols::TouchControls *tcDemo=0;
touchcontrols::TouchControls *tcGamepadUtility=0;
touchcontrols::TouchControls *tcDPadInventory=0;

// So can hide and show these buttons
touchcontrols::TouchJoy *touchJoyLeft;
touchcontrols::TouchJoy *touchJoyRight;

// So gampad can control keyboard
touchcontrols::UI_Keyboard *uiKeyboard;

touchcontrols::ButtonGrid *uiInventoryButtonGrid;

// Send message to JAVA SDL activity
int Android_JNI_SendMessage(int command, int param);

#if defined(PRBOOM_DOOM)
typedef enum {
  VID_MODE8,
  VID_MODE15,
  VID_MODE16,
  VID_MODE32,
  VID_MODEGL,
  VID_MODEMAX
} video_mode_t;
video_mode_t V_GetMode(void);
#endif


#if defined(CHOCOLATE) || defined(RETRO_DOOM)
    #define SW_SDL_RENDER 1
#else
    #define SW_SDL_RENDER 0
#endif

#if defined(GZDOOM) || defined(ZANDRONUM_30)

#ifdef  GZDOOM_DEV // For the dev versions it is always OpenGL, even software mode uses ogl
static int currentrenderer = 1;
#else
extern int currentrenderer;
#endif

static GLint     matrixMode;
static GLfloat   projection[16];
static GLfloat   model[16];
#endif


static void openGLStart()
{
#if 1

touchcontrols::gl_startRender();


#if !defined(GZDOOM_GL3)
    if( touchcontrols::gl_getGLESVersion() == 1 )
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glViewport(0, 0, mobile_screen_width, mobile_screen_height);
        glOrthof(0.0f, mobile_screen_width, mobile_screen_height, 0.0f, -1.0f, 1.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        //-----------------

        glDisableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY );

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable (GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);

    //    glDisable(GL_CULL_FACE);
        glDisable(GL_ALPHA_TEST);
    //    glDisable(GL_DEPTH_TEST);
    }
    else
    {

    }
#endif

#if defined(ZANDRONUM_30)
    //glBindBuffer(GL_ARRAY_BUFFER, 0); //GZDoom binds a buffer, this unbinds it
#endif

#if defined(GZDOOM)  && !defined(GZDOOM_GL3)
    //glBindBuffer(GL_ARRAY_BUFFER, 0); //GZDoom binds a buffer, this unbinds it
#endif


#if defined(GZDOOM_GL3)
//touchcontrols::gl_startGLES3();
     //glBindBuffer(GL_ARRAY_BUFFER, 0);
     //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
     //glBindFramebuffer(GL_FRAMEBUFFER, 0);
            //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
     //        glEnable (GL_BLEND);
     //        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      //       glEnable(GL_TEXTURE_2D);
#endif

#endif
}

static void openGLEnd()
{
#if 1

touchcontrols::gl_endRender();

#if defined(PRBOOM_DOOM)
bool sdlSWMode = (V_GetMode() != VID_MODEGL);
#else
bool sdlSWMode = false;
#endif



#if ( defined(GZDOOM) || defined(ZANDRONUM_30) )&& !defined(GZDOOM_GL3)
    if( touchcontrols::gl_getGLESVersion() == 1 )
    {
        if(currentrenderer == 1) //GL mode
        {
            void jwzgles_restore (void);
            jwzgles_restore();
        }
        else
        {
            sdlSWMode = true;
        }
    }
    else
    {
        touchcontrols::gl_resetGL4ES();
    }
#endif

#if !defined(GZDOOM_GL3)
// Setup for SDL Software rendering again
if( SW_SDL_RENDER || sdlSWMode )
{
     glDisable (GL_BLEND);
     glColor4f(1,1,1,1);
     glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
}
#endif

#if defined(PRBOOM_DOOM)
    if(V_GetMode() == VID_MODEGL)
    {
        void jwzgles_restore (void);
        jwzgles_restore();
    }
#endif

#endif

}

static void gameSettingsButton(int state)
{
    if (state == 1)
    {
        controlsContainer.showUIWindow( UI_tc );
    }
}

static void customSettingsButton(int state)
{
    if (state == 1)
    {
        showButtonListWindow( &controlsContainer );
        //showEditButtons();
    }
}


static void gameButton(int state,int code)
{
    if (code == KEY_SHOOT)
    {
        m_shooting = state;
        PortableAction(state,PORT_ACT_ATTACK);
    }
    else if (code == KEY_SHOW_WEAPONS)
    {
        if (state == 1)
        {
            if (!tcGameWeapons->enabled)
            {
                tcGameWeapons->animateIn(5);
            }
        }
    }
    else if (code == KEY_SHOW_INV)
    {
        if (state == 1)
        {
            if (!tcInventory->enabled)
            {
                tcInventory->animateIn(5);
            }
            else
                tcInventory->animateOut(5);
        }
    }
    else if (code == KEY_SHOW_CUSTOM)
    {
        if (state == 1)
        {
            if (!tcCutomButtons->enabled)
            {
                tcCutomButtons->setEnabled(true);
                tcCutomButtons->setAlpha(gameControlsAlpha);
                tcCutomButtons->fade(touchcontrols::FADE_IN,DEFAULT_FADE_FRAMES);
                showCustomOn = true;
            }
            else
            {
                tcCutomButtons->fade(touchcontrols::FADE_OUT,DEFAULT_FADE_FRAMES);
                showCustomOn = false;
            }
        }
    }
    else if(code == KEY_SHOW_KBRD)
    {
        if( state )
        {
            SDL_StartTextInput();
        }
    }
    else if( code == KEY_BACK_BUTTON )
    {
        if( state )
        {
            mobileBackButton();
        }
    }
    else if( code == PORT_ACT_MAP )
    {
        if (state )
        {
            mapState = 0;
        }

        PortableAction(state, code);
    }
    else
    {
        PortableAction(state, code);
    }
}


static void automapButton(int state,int code)
{
    if( state && code == PORT_ACT_MAP && mapState == 0 )
    {
#ifdef RETRO_DOOM // Turn on follow mode to allow movment in map mode
        void AM_ToggleFollowMode(void);
        AM_ToggleFollowMode();
#endif
        mapState = 1;
    }
}

static void gameUtilitiesButton(int state,int code)
{
    // Auto hide the gamepad utilitie, except if showing consol
    if( (code != PORT_ACT_CONSOLE) && (tcGamepadUtility->isEnabled()) )
    {
        tcGamepadUtility->setEnabled( false );
    }

    gameButton( state, code );
}

static void gameUtilitiesOutside( bool fromGamepad )
{
    if (tcGamepadUtility->isEnabled())
    {
        tcGamepadUtility->setEnabled( false );
    }
}

static void blankButton(int state,int code)
{
#if (PRBOOM_DOOM)
    if( state )
        PortableBackButton();
#else
    PortableAction(state, PORT_ACT_USE);
    PortableKeyEvent(state, SDL_SCANCODE_RETURN, 0);
#endif
}

static void customButton(int state,int code)
{
    LOGI("Cust %d, %d", state, code);
    PortableAction( state, code );

    // If we are binding keys hide the custom buttons again
    if (state == 1)
    	showCustomMenu = false;
}

//Weapon wheel callbacks
static void weaponWheelSelected(int enabled)
{
    if (enabled)
        tcWeaponWheel->fade(touchcontrols::FADE_IN,5); //fade in
}

static void weaponWheel(int segment)
{
    LOGI("weaponWheel %d",segment);
    PortableAction(1,PORT_ACT_WEAP1 + segment);
    PortableAction(0,PORT_ACT_WEAP1 + segment);
}

static void menuButton(int state,int code)
{
    if(code == KEY_SHOW_KBRD)
    {
        if (state)
            SDL_StartTextInput();

        return;
    }
    else if( code == KEY_BACK_BUTTON )
    {
        if( state )
            mobileBackButton();
    }
    else if( code == KEY_SHOW_CUSTOM )
    {
        if( state )
            showCustomMenu = true;
    }
    else if (code == SDL_SCANCODE_F10) //for choch setup f10 to work!
    {
        PortableKeyEvent(state, code, 0);
    }
    else  if(code == KEY_SHOW_GYRO)
    {
        // Show gyro options
        if (state)
            Android_JNI_SendMessage( 0x8002, 0 );
    }
    else  if(code == KEY_SHOW_GAMEPAD)
    {
        // Show gamepad options
        if (state)
            Android_JNI_SendMessage( 0x8004, 0 );
    }
    else if(code == PORT_ACT_CONSOLE)
    {
         PortableKeyEvent(state, SDL_SCANCODE_GRAVE, 0);
    }
    else
    {
        PortableAction(state, code);
    }

     demoControlsAlpha = DEMO_ALPHA_RESET;
}

static void inventoryButton(int state,int code)
{
    PortableAction(state,code);
}

static void inventoryOutside( bool fromGamepad )
{
    if( autoHideInventory || fromGamepad )
    {
        if (tcInventory->enabled)
            tcInventory->animateOut(5);
    }
}

static void dPadInventoryButton(int state,int code)
{
    PortableAction(state,code);
}

static void dPadInventoryOutside( bool fromGamepad )
{
    if (tcDPadInventory->enabled)
        tcDPadInventory->animateOut(5);
}

static void left_double_tap(int state)
{
    //LOGTOUCH("L double %d",state);
    if (left_double_action)
        PortableAction(state,left_double_action);
}

static void right_double_tap(int state)
{
    //LOGTOUCH("R double %d",state);
    if (right_double_action)
        PortableAction(state,right_double_action);
}

static void left_stick(float joy_x, float joy_y,float mouse_x, float mouse_y)
{
    float fwdback = joy_y * 15 * forward_sens;
    float strafe  = -joy_x * 10 * strafe_sens;

    PortableMove( fwdback, strafe );
}

static void right_stick(float joy_x, float joy_y,float mouse_x, float mouse_y)
{
    //LOGI(" mouse x = %f",mouse_x);
    int invert        = invertLook ? -1 : 1;
    float scale       = (m_shooting && precisionShoot) ? precisionSensitivty : 1;
    float pitchMouse       = mouse_y * pitch_sens * invert * scale;
    float pitchJoystick    = joy_y * pitch_sens * invert * scale * -2;

    float yawMouse    = mouse_x * yaw_sens * scale;
    float yawJoystick = joy_x  * yaw_sens * scale * 10;

    if (joystickLookMode)
        PortableLookPitch( LOOK_MODE_JOYSTICK, pitchJoystick );
    else
        PortableLookPitch( LOOK_MODE_MOUSE, pitchMouse );
    
    if (joystickLookMode)
         PortableLookYaw( LOOK_MODE_JOYSTICK, yawJoystick );
    else
        PortableLookYaw( LOOK_MODE_MOUSE, yawMouse );  
}


//Weapon select callbacks
static void selectWeaponButton(int state, int code)
{
    PortableAction(state,PORT_ACT_WEAP0 + code);
    if (autoHideNumbers && state == 0)
        tcGameWeapons->animateOut(5);
}

static void automap_multitouch_mouse_move(int action,float x, float y,float dx, float dy)
{
    if (action == MULTITOUCHMOUSE_MOVE)
    {
        PortableAutomapControl( 0, dx, dy );
    }
    else if (action == MULTITOUCHMOUSE_ZOOM)
    {
        PortableAutomapControl( x, 0, 0 );
    }
}

static void showHideKeyboard( int show )
{
    LOGI( "showHideKeyboard %d", show );

    if( useSystemKeyboard )
    {
        if( show )
            Android_JNI_SendMessage( 0x8003, 0 );
    }
    else
    {
        tcKeyboard->setEnabled( show );
    }
}

static void brightnessSlideMouse( int action, float x, float y, float dx, float dy )
{
    y = 1 - y;
    Android_JNI_SendMessage( 0x8001, y * 255 );
}

static void touchSettings( touchcontrols::tTouchSettings settings )
{
    gameControlsAlpha = settings.alpha;

    invertLook = settings.invertLook;

    strafe_sens = settings.moveSensitivity;
    forward_sens = settings.moveSensitivity;
    pitch_sens = settings.lookSensitivity;
    yaw_sens = settings.turnSensitivity;

    showSticks = settings.showJoysticks;
    precisionShoot = settings.precisionShoot;
    precisionSensitivty = settings.precisionSenitivity;

    joystickLookMode =  settings.joystickLookMode;
    autoHideInventory = settings.autoHideInventory;
	autoHideNumbers = settings.autoHideNumbers;
    weaponWheelEnabled  = settings.weaponWheelEnabled;

    switch( settings.dblTapLeft )
    {
        case 1: left_double_action = PORT_ACT_USE; break;
        case 2: left_double_action = PORT_ACT_JUMP; break;
        case 3: left_double_action = PORT_ACT_ATTACK; break;
        default: left_double_action = 0;
    }

    switch( settings.dblTapRight )
    {
        case 1: right_double_action = PORT_ACT_USE; break;
        case 2: right_double_action = PORT_ACT_JUMP; break;
        case 3: right_double_action = PORT_ACT_ATTACK; break;
        default: right_double_action = 0;
    }

    tcGameMain->setAlpha(gameControlsAlpha);
    touchJoyLeft->setCenterAnchor(settings.fixedMoveStick);
}

extern SDL_Scancode SDLCALL SDL_GetScancodeFromKey(SDL_Keycode key);
extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
extern int SDL_SendEditingText(const char *text, int start, int end);
extern int SDL_SendKeyboardText(const char *text);
extern void SDL_StartTextInput(void);
extern void SDL_StopTextInput(void);
 #include <unistd.h>
static void keyboardKeyPressed( uint32_t key )
{
    LOGI( "Keyboard press: %d , %c", key, (char)key );

    if( key == UI_KEYBOARD_HIDE )
    {
        SDL_StopTextInput();
        return;
    }

    // Only send printable chars
    if( key >= 32 && key <= 125 )
    {
        char text[2];
        text[0] = key;
        text[1] = 0;

        SDL_SendKeyboardText(text);
    }

    // Change upper case to lower case to get scan code
    if( key >= 65 && key <= 90 )
    {
        key = key + 32;
    }

    SDL_Scancode sc = SDL_GetScancodeFromKey( key  );
    if( sc != SDL_SCANCODE_UNKNOWN )
    {
        LOGI( "scan code: %d",sc );
//SDL_SendKeyboardKey( SDL_PRESSED, SDL_SCANCODE_LSHIFT );
//SDL_SetModState( KMOD_LSHIFT );
        SDL_SendKeyboardKey( SDL_PRESSED, sc );
        SDL_SendKeyboardKey( SDL_RELEASED, sc );
//SDL_SetModState( KMOD_NONE );
// SDL_SendKeyboardKey( SDL_RELEASED, SDL_SCANCODE_LSHIFT );
    }
}


static void setHideSticks(bool v)
{
    if (touchJoyLeft) touchJoyLeft->setHideGraphics(v);
    if (touchJoyRight) touchJoyRight->setHideGraphics(v);
}

static void updateTouchScreenMode(touchscreemode_t mode)
{
    // LOGI("updateTouchScreenModeA %d",mode);
    
    if (mode != currentScreenMode){
        
        //first disable the last screen and fade out is necessary
        switch(currentScreenMode){
            case TS_BLANK:
                tcBlank->resetOutput();
                tcBlank->setEnabled(false);
                break;
            case TS_MENU:
                tcMenuMain->resetOutput();
                tcMenuMain->fade(touchcontrols::FADE_OUT,DEFAULT_FADE_FRAMES);
                break;
            case TS_Y_N:
                tcYesNo->resetOutput();
                tcYesNo->fade(touchcontrols::FADE_OUT,DEFAULT_FADE_FRAMES);
                break;
            case TS_GAME:
                tcGameMain->resetOutput();
                tcGameMain->fade(touchcontrols::FADE_OUT,DEFAULT_FADE_FRAMES);
                tcGameWeapons->setEnabled(false);
                tcWeaponWheel->setEnabled(false);
                tcCutomButtons->resetOutput();
                tcCutomButtons->setEnabled(false);
                tcInventory->resetOutput();
                tcInventory->setEnabled(false);
                break;
            case TS_MAP:
                tcAutomap->resetOutput();
                tcAutomap->fade(touchcontrols::FADE_OUT,DEFAULT_FADE_FRAMES);
                break;
            case TS_CUSTOM:
                tcCutomButtons->resetOutput();
                tcCutomButtons->fade(touchcontrols::FADE_OUT,DEFAULT_FADE_FRAMES);
                break;
            case TS_DEMO:
                tcDemo->resetOutput();
                tcDemo->fade(touchcontrols::FADE_OUT,DEFAULT_FADE_FRAMES);
                break;
            case TS_CONSOLE:
                break;
        }
        
        //Enable the current new screen
        switch(mode){
            case TS_BLANK:
                tcBlank->setEnabled(true);
                break;
            case TS_MENU:
                if( !hideGameAndMenu )
                {
                    tcMenuMain->setEnabled(true);
                    tcMenuMain->fade(touchcontrols::FADE_IN,DEFAULT_FADE_FRAMES);
                }
                break;
            case TS_Y_N:
                tcYesNo->setEnabled(true);
                tcYesNo->fade(touchcontrols::FADE_IN,DEFAULT_FADE_FRAMES);
                break;
            case TS_GAME:
                if( !hideGameAndMenu )
                {
                    tcGameMain->setEnabled(true);
                    tcGameMain->fade(touchcontrols::FADE_IN,DEFAULT_FADE_FRAMES);

                    if( weaponWheelEnabled )
                        tcWeaponWheel->setEnabled(true);

                    if( showCustomOn ) // Also remember if custom buttons were shown
                    {
                        tcCutomButtons->setEnabled(true);
                        tcCutomButtons->fade(touchcontrols::FADE_IN,DEFAULT_FADE_FRAMES);
                    }
                }
                break;
            case TS_MAP:
                { // This is a bit of hack. We want the map button to be in the same place as the game map button
                  // So move it
                    touchcontrols::Button* mapGame = (touchcontrols::Button*)tcGameMain->getControl("map");
                    touchcontrols::Button* map = (touchcontrols::Button*)tcAutomap->getControl("map");
                    map->controlPos = mapGame->controlPos;
                    map->updateSize();
                }
                tcAutomap->setEnabled(true);
                tcAutomap->fade(touchcontrols::FADE_IN,DEFAULT_FADE_FRAMES);
                break;
            case TS_CUSTOM:
                tcCutomButtons->setEnabled(true);
                tcCutomButtons->setAlpha(1.0);
                tcCutomButtons->fade(touchcontrols::FADE_IN,DEFAULT_FADE_FRAMES);
                break;
           case TS_DEMO:
                tcDemo->setEnabled(true);
                tcDemo->fade(touchcontrols::FADE_IN,DEFAULT_FADE_FRAMES);
                break;
            case TS_CONSOLE:
                break;
        }
        
        currentScreenMode = mode;
    }
    
}

void frameControls()
{
    //static bool inited = false;
    if( SDL_NewEGLCreated() )
    {
        LOGI("NEW EGL CONTEXT");
        touchcontrols::clearGlTexCache();
        controlsContainer.initGL();
    }

    touchscreemode_t screenMode = PortableGetScreenMode();

    if( (screenMode == TS_MAP) && (mapState == 1) )
    {
       screenMode =  TS_GAME;
    }
    // Hack to show custom buttons while in the menu to bind keys
    if( screenMode == TS_MENU && showCustomMenu == true )
    {
        screenMode = TS_CUSTOM;
    }

    if( screenMode == TS_DEMO ) // Fade out demo buttons if not touched for a while
    {
        if(demoControlsAlpha > 0 )
        {
            demoControlsAlpha -= DEMO_ALPFA_DEC;
        }
        tcDemo->setAlpha(demoControlsAlpha);
    }

    updateTouchScreenMode(screenMode);
    
    setHideSticks(!showSticks);

    controlsContainer.draw();
}

void initControls(int width, int height,const char * graphics_path)
{
    touchcontrols::GLScaleWidth = (float)width;
    touchcontrols::GLScaleHeight = (float)height;
    
    LOGI("initControls %d x %d,x path = %s",width,height,graphics_path);
    
    if (!controlsCreated)
    {
        LOGI("creating controls");
        
        touchcontrols::gl_setGraphicsBasePath(graphics_path);
        //setControlsContainer(&controlsContainer);
        
        controlsContainer.openGL_start.connect( sigc::ptr_fun(&openGLStart));
        controlsContainer.openGL_end.connect( sigc::ptr_fun(&openGLEnd));

        touchcontrols::getSettingsSignal()->connect( sigc::ptr_fun(&touchSettings) );
        
        tcMenuMain = new touchcontrols::TouchControls("menu",false,true,10,false);
        tcYesNo = new touchcontrols::TouchControls("yes_no",false,false);
        tcGameMain = new touchcontrols::TouchControls("game",false,true,1,true);
        tcGameWeapons = new touchcontrols::TouchControls("weapons",false,true,1,false);
        tcInventory  = new touchcontrols::TouchControls("inventory",false,true,2,false); //Different edit group
        tcWeaponWheel = new touchcontrols::TouchControls("weapon_wheel",false,true,1,false);
        tcAutomap = new touchcontrols::TouchControls("automap",false,false);
        tcBlank = new touchcontrols::TouchControls("blank",true,false);
        tcCutomButtons = new touchcontrols::TouchControls("custom_buttons",false,true,1,true);
        tcKeyboard = new touchcontrols::TouchControls("keyboard",false,false);
        tcDemo = new touchcontrols::TouchControls("demo_playback",false,false);
        tcGamepadUtility = new touchcontrols::TouchControls("gamepad_utility",false,false);
        tcDPadInventory = new touchcontrols::TouchControls("dpad_inventory",false,false);

        //Menu -------------------------------------------
        //------------------------------------------------------
        tcMenuMain->addControl(new touchcontrols::Button("back",touchcontrols::RectF(0,0,2,2),"ui_back_arrow",KEY_BACK_BUTTON));
        tcMenuMain->addControl(new touchcontrols::Button("down_arrow",touchcontrols::RectF(20,13,23,16),"arrow_down",PORT_ACT_MENU_DOWN));
        tcMenuMain->addControl(new touchcontrols::Button("up_arrow",touchcontrols::RectF(20,10,23,13),"arrow_up",PORT_ACT_MENU_UP));
        tcMenuMain->addControl(new touchcontrols::Button("left_arrow",touchcontrols::RectF(17,13,20,16),"arrow_left",PORT_ACT_MENU_LEFT));
        tcMenuMain->addControl(new touchcontrols::Button("right_arrow",touchcontrols::RectF(23,13,26,16),"arrow_right",PORT_ACT_MENU_RIGHT));
        tcMenuMain->addControl(new touchcontrols::Button("enter",touchcontrols::RectF(0,10,6,16),"enter",PORT_ACT_MENU_SELECT));
        tcMenuMain->addControl(new touchcontrols::Button("keyboard",touchcontrols::RectF(2,0,4,2),"keyboard",KEY_SHOW_KBRD));
#if defined(GZDOOM) || defined(RETRO_DOOM)
		// Consoel does not work properly in menu
        //tcMenuMain->addControl(new touchcontrols::Button("console",touchcontrols::RectF(6,0,8,2),"tild",PORT_ACT_CONSOLE));
#endif
#ifndef CHOC_SETUP
        tcMenuMain->addControl(new touchcontrols::Button("gamepad",touchcontrols::RectF(22,0,24,2),"gamepad",KEY_SHOW_GAMEPAD));
        tcMenuMain->addControl(new touchcontrols::Button("gyro",touchcontrols::RectF(24,0,26,2),"gyro",KEY_SHOW_GYRO));
#endif
        //tcMenuMain->addControl(new touchcontrols::Button("brightness",touchcontrols::RectF(21,0,23,2),"brightness",KEY_BRIGHTNESS));
#ifdef CHOC_SETUP
        tcMenuMain->addControl(new touchcontrols::Button("f10",touchcontrols::RectF(24,0,26,2),"key_f10",SDL_SCANCODE_F10));
#endif

#ifndef CHOC_SETUP
        touchcontrols::Mouse *brightnessSlide = new touchcontrols::Mouse("slide_mouse", touchcontrols::RectF(24,3,26,11), "brightness_slider" );
        brightnessSlide->signal_action.connect(  sigc::ptr_fun(& brightnessSlideMouse ) );
        tcMenuMain->addControl( brightnessSlide );
#endif

#ifdef GZDOOM
        // Actually this isn't needed now
        //tcMenuMain->addControl(new touchcontrols::Button("show_custom",touchcontrols::RectF(24,0,26,2),"custom_show",KEY_SHOW_CUSTOM,false,false));
#endif
        tcMenuMain->signal_button.connect(  sigc::ptr_fun(&menuButton) );
        tcMenuMain->setAlpha(0.8);
        tcMenuMain->setFixAspect(false);
        
        //Game -------------------------------------------
        //------------------------------------------------------
        tcGameMain->setAlpha(gameControlsAlpha);
        tcGameMain->addControl(new touchcontrols::Button("back",touchcontrols::RectF(0,0,2,2),"ui_back_arrow",KEY_BACK_BUTTON,false,false,"Show menu"));
        tcGameMain->addControl(new touchcontrols::Button("attack",touchcontrols::RectF(20,7,23,10),"shoot",KEY_SHOOT,false,false,"Attack!"));
        tcGameMain->addControl(new touchcontrols::Button("attack2",touchcontrols::RectF(3,5,6,8),"shoot",KEY_SHOOT,false,true,"Attack! (duplicate)"));

        tcGameMain->addControl(new touchcontrols::Button("use",touchcontrols::RectF(23,6,26,9),"use",PORT_ACT_USE,false,false,"Use/Open"));
        tcGameMain->addControl(new touchcontrols::Button("quick_save",touchcontrols::RectF(24,0,26,2),"save",PORT_ACT_QUICKSAVE,false,false,"Quick save"));
        tcGameMain->addControl(new touchcontrols::Button("quick_load",touchcontrols::RectF(20,0,22,2),"load",PORT_ACT_QUICKLOAD,false,false,"Quick load"));
        tcGameMain->addControl(new touchcontrols::Button("map",touchcontrols::RectF(2,0,4,2),"map",PORT_ACT_MAP,false,false,"Show map"));
        tcGameMain->addControl(new touchcontrols::Button("keyboard",touchcontrols::RectF(8,0,10,2),"keyboard",KEY_SHOW_KBRD,false,true,"Show Keyboard"));

#if defined(RETRO_DOOM) || defined(CHOCOLATE) || defined (PRBOOM_DOOM)
        tcGameMain->addControl(new touchcontrols::Button("gamma",touchcontrols::RectF(17,0,19,2),"gamma",PORT_ACT_GAMMA,false,false,"Gamma"));
#endif

        bool hideJump = true;
        if(( gameType == GAME_TYPE_STRIFE ) || (gameType == GAME_TYPE_HEXEN ) || (gameType == GAME_TYPE_HERETIC))
            hideJump = false;

        tcGameMain->addControl(new touchcontrols::Button("jump",touchcontrols::RectF(24,3,26,5),"jump",PORT_ACT_JUMP,false,hideJump,"Jump"));

        bool hideInventory = true;
        if(( gameType == GAME_TYPE_STRIFE ) || (gameType == GAME_TYPE_HEXEN ) || (gameType == GAME_TYPE_HERETIC))
            hideInventory = false;

        tcGameMain->addControl(new touchcontrols::Button("use_inventory",touchcontrols::RectF(0,9,2,11),"inventory",KEY_SHOW_INV,false,hideInventory,"Show Inventory"));

#ifndef CHOCOLATE
        tcGameMain->addControl(new touchcontrols::Button("crouch",touchcontrols::RectF(24,14,26,16),"crouch",PORT_ACT_DOWN,false,true,"Crouch"));
        tcGameMain->addControl(new touchcontrols::Button("attack_alt",touchcontrols::RectF(21,5,23,7),"shoot_alt",PORT_ACT_ALT_ATTACK,false,true,"Alt fire"));
#endif

#if defined(GZDOOM) || defined(ZANDRONUM_30)
        tcGameMain->addControl(new touchcontrols::Button("show_custom",touchcontrols::RectF(0,7,2,9),"custom_show",KEY_SHOW_CUSTOM,false,true,"Show custom"));
#endif
        
        tcGameMain->addControl(new touchcontrols::Button("show_weapons",touchcontrols::RectF(12,14,14,16),"show_weapons",KEY_SHOW_WEAPONS,false,false,"Show numbers"));
        tcGameMain->addControl(new touchcontrols::Button("next_weapon",touchcontrols::RectF(0,3,3,5),"next_weap",PORT_ACT_NEXT_WEP,false,false,"Next weapon"));
        tcGameMain->addControl(new touchcontrols::Button("prev_weapon",touchcontrols::RectF(0,5,3,7),"prev_weap",PORT_ACT_PREV_WEP,false,false,"Prev weapon"));

#if defined(GZDOOM) || defined(RETRO_DOOM) || defined(ZANDRONUM_30)
        tcGameMain->addControl(new touchcontrols::Button("console",touchcontrols::RectF(6,0,8,2),"tild",PORT_ACT_CONSOLE,false,true,"Console"));
#endif


        touchJoyRight = new touchcontrols::TouchJoy("touch",touchcontrols::RectF(17,4,26,16),"look_arrow");
        tcGameMain->addControl(touchJoyRight);
        touchJoyRight->signal_move.connect(sigc::ptr_fun(&right_stick) );
        touchJoyRight->signal_double_tap.connect(sigc::ptr_fun(&right_double_tap) );
        
        touchJoyLeft = new touchcontrols::TouchJoy("stick",touchcontrols::RectF(0,7,8,16),"strafe_arrow");
        tcGameMain->addControl(touchJoyLeft);
        touchJoyLeft->signal_move.connect(sigc::ptr_fun(&left_stick) );
        touchJoyLeft->signal_double_tap.connect(sigc::ptr_fun(&left_double_tap) );

#if 1
        // SWAPFIX
        touchJoyLeft->registerTouchJoySWAPFIX( touchJoyRight );
        touchJoyRight->registerTouchJoySWAPFIX( touchJoyLeft );
#endif

        tcGameMain->signal_button.connect(  sigc::ptr_fun(&gameButton) );
        tcGameMain->signal_settingsButton.connect(  sigc::ptr_fun(&gameSettingsButton) );


        UI_tc = touchcontrols::createDefaultSettingsUI( &controlsContainer, ( std::string )graphics_path +  "/touch_settings.xml" );
        UI_tc->setAlpha( 1 );
        
        //Weapons -------------------------------------------
        //------------------------------------------------------
        tcGameWeapons->addControl(new touchcontrols::Button("weapon1",touchcontrols::RectF(1,14,3,16),"key_1",1));
        tcGameWeapons->addControl(new touchcontrols::Button("weapon2",touchcontrols::RectF(3,14,5,16),"key_2",2));
        tcGameWeapons->addControl(new touchcontrols::Button("weapon3",touchcontrols::RectF(5,14,7,16),"key_3",3));
        tcGameWeapons->addControl(new touchcontrols::Button("weapon4",touchcontrols::RectF(7,14,9,16),"key_4",4));
        tcGameWeapons->addControl(new touchcontrols::Button("weapon5",touchcontrols::RectF(9,14,11,16),"key_5",5));

        tcGameWeapons->addControl(new touchcontrols::Button("weapon6",touchcontrols::RectF(15,14,17,16),"key_6",6));
        tcGameWeapons->addControl(new touchcontrols::Button("weapon7",touchcontrols::RectF(17,14,19,16),"key_7",7));
        tcGameWeapons->addControl(new touchcontrols::Button("weapon8",touchcontrols::RectF(19,14,21,16),"key_8",8));
        tcGameWeapons->addControl(new touchcontrols::Button("weapon9",touchcontrols::RectF(21,14,23,16),"key_9",9));
        tcGameWeapons->addControl(new touchcontrols::Button("weapon0",touchcontrols::RectF(23,14,25,16),"key_0",0));

        tcGameWeapons->signal_button.connect(  sigc::ptr_fun(&selectWeaponButton) );
        tcGameWeapons->setAlpha(0.8);
        
        //Weapon wheel -------------------------------------------
        //------------------------------------------------------
        const char * weapon_wheel_gfx = "weapon_wheel";
        int weaponWheelNbr = 8;
        if( gameType == GAME_TYPE_HERETIC )
        {
            weapon_wheel_gfx = "weapon_wheel_heretic";
        }
        else if( gameType == GAME_TYPE_HEXEN )
        {
            weapon_wheel_gfx = "weapon_wheel_hexen";
            weaponWheelNbr = 4;
        }

        touchcontrols::WheelSelect *wheel = new touchcontrols::WheelSelect("weapon_wheel",touchcontrols::RectF(7,2,19,14),weapon_wheel_gfx,weaponWheelNbr);
        wheel->signal_selected.connect(sigc::ptr_fun(&weaponWheel) );
        wheel->signal_enabled.connect(sigc::ptr_fun(&weaponWheelSelected));
        tcWeaponWheel->addControl(wheel);
        tcWeaponWheel->setAlpha(0.8);

        // Inventory -------------------------------------------
        //------------------------------------------------------
        if( gameType == GAME_TYPE_STRIFE )
        {
            uiInventoryButtonGrid = new touchcontrols::ButtonGrid("inventory_grid", touchcontrols::RectF(3,7,11,11),"inventory_bg", 4, 2 );

            uiInventoryButtonGrid->addCell(0,0,"ammo",PORT_ACT_SHOW_WEAPONS);
            uiInventoryButtonGrid->addCell(1,0,"key",PORT_ACT_SHOW_KEYS);
            uiInventoryButtonGrid->addCell(2,0,"notebook",PORT_ACT_HELPCOMP);

            uiInventoryButtonGrid->addCell(0,1,"inventory_left",PORT_ACT_INVPREV);
            uiInventoryButtonGrid->addCell(1,1,"inventory_right",PORT_ACT_INVNEXT);
            uiInventoryButtonGrid->addCell(2,1,"inventory_use",PORT_ACT_INVUSE);
            uiInventoryButtonGrid->addCell(3,1,"inventory_drop",PORT_ACT_INVDROP);
        }
        else
        {
            uiInventoryButtonGrid = new touchcontrols::ButtonGrid("inventory_grid", touchcontrols::RectF(3,9,11,11),"inventory_bg", 4, 1 );

            uiInventoryButtonGrid->addCell(0,0,"inventory_left",PORT_ACT_INVPREV);
            uiInventoryButtonGrid->addCell(1,0,"inventory_right",PORT_ACT_INVNEXT);
            uiInventoryButtonGrid->addCell(2,0,"inventory_use",PORT_ACT_INVUSE);
            uiInventoryButtonGrid->addCell(3,0,"inventory_drop",PORT_ACT_INVDROP);
        }

        uiInventoryButtonGrid->signal_outside.connect( sigc::ptr_fun(&inventoryOutside) );

        tcInventory->addControl( uiInventoryButtonGrid );
        tcInventory->setPassThroughTouch( touchcontrols::TouchControls::PassThrough::NO_CONTROL );
        tcInventory->signal_button.connect( sigc::ptr_fun(&inventoryButton) );
        tcInventory->setAlpha(0.9);
        
        
        //Auto Map -------------------------------------------
        //------------------------------------------------------
        touchcontrols::MultitouchMouse *multimouse = new touchcontrols::MultitouchMouse("gamemouse",touchcontrols::RectF(0,0,26,16),"");
        multimouse->setHideGraphics(true);
        tcAutomap->addControl(multimouse);
        multimouse->signal_action.connect(sigc::ptr_fun(&automap_multitouch_mouse_move) );
        tcAutomap->addControl(new touchcontrols::Button("map",touchcontrols::RectF(2,0,4,2),"map",PORT_ACT_MAP));
        tcAutomap->signal_button.connect(  sigc::ptr_fun(&automapButton) );
        
        
        //Blank -------------------------------------------
        //------------------------------------------------------
        tcBlank->addControl(new touchcontrols::Button("enter",touchcontrols::RectF(0,0,26,16),"",0x123));
        tcBlank->signal_button.connect(  sigc::ptr_fun(&blankButton) );

        //Demo -------------------------------------------
        //------------------------------------------------------
        tcDemo->addControl(new touchcontrols::Button("pressed",touchcontrols::RectF(0,2,26,16),"",0));
        tcDemo->addControl(new touchcontrols::Button("keyboard",touchcontrols::RectF(2,0,4,2),"keyboard",KEY_SHOW_KBRD));
        tcDemo->addControl(new touchcontrols::Button("speed_down",touchcontrols::RectF(20,14,22,16),"key_-",PORT_ACT_DEMO_SPD_DWN));
        tcDemo->addControl(new touchcontrols::Button("speed_default",touchcontrols::RectF(22,14,24,16),"key_0",PORT_ACT_DEMO_SPD_DEF));
        tcDemo->addControl(new touchcontrols::Button("speed_up",touchcontrols::RectF(24,14,26,16),"key_+",PORT_ACT_DEMO_SPD_UP));
        tcDemo->addControl(new touchcontrols::Button("camera",touchcontrols::RectF(0,14,2,16),"camera",PORT_ACT_DEMO_CAMERA));
        tcDemo->signal_button.connect(  sigc::ptr_fun(&menuButton) );
        tcDemo->setAlpha(1);

        //Keyboard -------------------------------------------
        //------------------------------------------------------
        uiKeyboard = new touchcontrols::UI_Keyboard("keyboard",touchcontrols::RectF(0,8,26,16),"font_dual",0,0,0);
        uiKeyboard->signal.connect(  sigc::ptr_fun(&keyboardKeyPressed) );
        tcKeyboard->addControl( uiKeyboard );
        // We want touch to pass through only where there is no keyboard
        tcKeyboard->setPassThroughTouch( touchcontrols::TouchControls::PassThrough::NO_CONTROL );

        //Yes No -------------------------------------------
        //------------------------------------------------------
        tcYesNo->addControl(new touchcontrols::Button("yes",touchcontrols::RectF(8,12,11,15),"key_y",PORT_ACT_MENU_CONFIRM));
        tcYesNo->addControl(new touchcontrols::Button("no",touchcontrols::RectF(15,12,18,15),"key_n",PORT_ACT_MENU_ABORT));
        tcYesNo->signal_button.connect(  sigc::ptr_fun(&menuButton) );
        tcYesNo->setAlpha(0.8);

        //Custom Controls -------------------------------------------
        //------------------------------------------------------
        tcCutomButtons->addControl(new touchcontrols::Button("A",touchcontrols::RectF(5,5,7,7),"Custom_1",PORT_ACT_CUSTOM_0,false,false,"Custom 1"));
        tcCutomButtons->addControl(new touchcontrols::Button("B",touchcontrols::RectF(7,5,9,7),"Custom_2",PORT_ACT_CUSTOM_1,false,false,"Custom 2"));
        tcCutomButtons->addControl(new touchcontrols::Button("C",touchcontrols::RectF(5,7,7,9),"Custom_3",PORT_ACT_CUSTOM_2,false,false,"Custom 3"));
        
        tcCutomButtons->addControl(new touchcontrols::Button("D",touchcontrols::RectF(7,7,9,9),"Custom_4",PORT_ACT_CUSTOM_3,false,false,"Custom 4"));
        tcCutomButtons->addControl(new touchcontrols::Button("E",touchcontrols::RectF(5,9,7,11),"Custom_5",PORT_ACT_CUSTOM_4,false,false,"Custom 5"));
        tcCutomButtons->addControl(new touchcontrols::Button("F",touchcontrols::RectF(7,9,9,11),"Custom_6",PORT_ACT_CUSTOM_5,false,false,"Custom 6"));

        touchcontrols::QuadSlide *qs1 = new touchcontrols::QuadSlide("quad_slide_1",touchcontrols::RectF(10,7,12,9),"quad_slide","slide_arrow",PORT_ACT_CUSTOM_8,PORT_ACT_CUSTOM_9,PORT_ACT_CUSTOM_10,PORT_ACT_CUSTOM_11,false,"Quad Slide 1");
        tcCutomButtons->addControl(qs1);

        touchcontrols::QuadSlide *qs2 = new touchcontrols::QuadSlide("quad_slide_2",touchcontrols::RectF(14,7,16,9),"quad_slide","slide_arrow",PORT_ACT_CUSTOM_12,PORT_ACT_CUSTOM_13,PORT_ACT_CUSTOM_14,PORT_ACT_CUSTOM_15,false,"Quad Slide 2");
        tcCutomButtons->addControl(qs2);

        //tcCutomButtons->setColor(0.7,0.7,1.f);

        qs1->signal.connect(sigc::ptr_fun(&customButton));
        qs2->signal.connect(sigc::ptr_fun(&customButton));
        tcCutomButtons->signal_button.connect(sigc::ptr_fun(&customButton));
        tcCutomButtons->signal_settingsButton.connect(  sigc::ptr_fun(&customSettingsButton) );
        tcCutomButtons->setAlpha(0.8);

        //Gamepad utility -------------------------------------------
        //------------------------------------------------------
        touchcontrols::ButtonGrid *gamepadUtils = new touchcontrols::ButtonGrid("gamepad_grid", touchcontrols::RectF(8,5,18,11),"gamepad_utils_bg", 3, 2 );

        gamepadUtils->addCell(0,0,"ui_back_arrow",KEY_BACK_BUTTON);
        gamepadUtils->addCell(0,1,"map",PORT_ACT_MAP);
        gamepadUtils->addCell(1,0,"keyboard",KEY_SHOW_KBRD);
        gamepadUtils->addCell(1,1,"tild",PORT_ACT_CONSOLE);
        gamepadUtils->addCell(2,0,"save",PORT_ACT_QUICKSAVE);
        gamepadUtils->addCell(2,1,"load",PORT_ACT_QUICKLOAD);

        gamepadUtils->signal_outside.connect( sigc::ptr_fun(&gameUtilitiesOutside) );

        tcGamepadUtility->addControl( gamepadUtils );
        tcGamepadUtility->setAlpha(0.9);
        tcGamepadUtility->signal_button.connect( sigc::ptr_fun(&gameUtilitiesButton) );

        // DPad inventory -------------------------------------------
        //------------------------------------------------------
        touchcontrols::DPadSelect *dpadInventory = new touchcontrols::DPadSelect("dpad_inventory", touchcontrols::RectF(9,4,17,12),"dpad", PORT_ACT_INVUSE );

        dpadInventory->addCell(touchcontrols::DPAD_LEFT,"inventory_left",PORT_ACT_INVPREV);
        dpadInventory->addCell(touchcontrols::DPAD_RIGHT,"inventory_right",PORT_ACT_INVNEXT);
        dpadInventory->addCell(touchcontrols::DPAD_DOWN,"inventory_drop",PORT_ACT_INVDROP);
        //dpadInventory->addCell(touchcontrols::DPAD_LEFT,"inventory_left",PORT_ACT_INVPREV);
        //
        dpadInventory->signal_button.connect( sigc::ptr_fun( &dPadInventoryButton ));
        dpadInventory->signal_outside.connect( sigc::ptr_fun( &dPadInventoryOutside ));

        tcDPadInventory->addControl( dpadInventory );
        tcDPadInventory->setAlpha(0.9);


        //---------------------------------------------------------------
        //---------------------------------------------------------------
        controlsContainer.addControlGroup(tcKeyboard);
        controlsContainer.addControlGroup(tcInventory); // before gamemain so touches don't go through
        controlsContainer.addControlGroup(tcGamepadUtility); // before gamemain so touches don't go through
        controlsContainer.addControlGroup(tcDPadInventory);
#if defined(GZDOOM) || defined(ZANDRONUM_30)
        controlsContainer.addControlGroup(tcCutomButtons);
#endif
        controlsContainer.addControlGroup(tcGameMain);
        controlsContainer.addControlGroup(tcYesNo);
        controlsContainer.addControlGroup(tcGameWeapons);

        controlsContainer.addControlGroup(tcMenuMain);
        controlsContainer.addControlGroup(tcWeaponWheel);
        controlsContainer.addControlGroup(tcAutomap);
        controlsContainer.addControlGroup(tcBlank);
        controlsContainer.addControlGroup(tcDemo);

        controlsCreated = 1;

        // Doom type stays the same so existing user keep original config
        if( gameType == GAME_TYPE_HEXEN )
            touchcontrols::setGlobalXmlAppend(".hexen");
        else if( gameType == GAME_TYPE_HERETIC )
            touchcontrols::setGlobalXmlAppend(".heretic");
        else if( gameType == GAME_TYPE_STRIFE )
            touchcontrols::setGlobalXmlAppend(".strife");

		tcMenuMain->setXMLFile((std::string)graphics_path +  "/menu.xml");

        tcGameMain->setXMLFile((std::string)graphics_path +  "/game_" ENGINE_NAME ".xml");
        tcInventory->setXMLFile((std::string)graphics_path +  "/inventory_" ENGINE_NAME ".xml");
        tcWeaponWheel->setXMLFile((std::string)graphics_path +  "/weaponwheel_" ENGINE_NAME ".xml");
        tcGameWeapons->setXMLFile((std::string)graphics_path +  "/weapons_" ENGINE_NAME ".xml");
        tcCutomButtons->setXMLFile((std::string)graphics_path +  "/custom_buttons_0_" ENGINE_NAME ".xml");
    }
    else
        LOGI("NOT creating controls");
    
    SDL_SetSwapBufferCallBack(frameControls);

    SDL_SetShowKeyboardCallBack(showHideKeyboard);
}




static std::string graphicpath;
const char * getFilesPath()
{
    return graphicpath.c_str(); //graphics path is the same as files path
}

std::string game_path;
const char * getGamePath()
{
    return game_path.c_str();
}


void mobile_init(int width, int height, const char *pngPath,int options, int game)
{
    if( options & GAME_OPTION_AUTO_HIDE_GAMEPAD )
        gamepadHideTouch = true;

    if( options & GAME_OPTION_HIDE_MENU_AND_GAME )
        hideGameAndMenu = true;

    if( options & GAME_OPTION_USE_SYSTEM_KEYBOARD )
        useSystemKeyboard = true;

    if( options & GAME_OPTION_GLES2 )
    {
        touchcontrols::gl_setGLESVersion( 2 );
        touchcontrols::gl_useGL4ES(); // GLES2 always uses GL4ES library
    }

#ifdef GZDOOM_GL3
    touchcontrols::gl_setGLESVersion( 3 );
#endif

    gameType = game;

    LOGI("Game type = %d", gameType );

#ifdef __ANDROID__
    mobile_screen_width = width;
    mobile_screen_height = height;
    graphicpath = pngPath;
    LOGI("PNG Path = %s\n",graphicpath.c_str());

    putenv((char*)"TIMIDITY_CFG=./audiopack/snd_timidity/timidity.cfg");

    initControls(mobile_screen_width,-mobile_screen_height,graphicpath.c_str());

#endif
#ifdef __IOS__
    mobile_screen_width = 1152;
    mobile_screen_height = 640;
    
    static std::string pngPath = getPngDirectory();
   //pngPath += "/../png/";
    LOGI("PNG Path = %s\n",pngPath.c_str());
    
    initControls(mobile_screen_width,-mobile_screen_height,pngPath.c_str());

    SDL_SetTouchControlsInterface(&controlsContainer);
#endif
}

void mobileBackButton( void )
{
    LOGI("mobileBackButton");
    if( tcKeyboard->isEnabled() )
    {
        SDL_StopTextInput();
    }
    else if( tcInventory->isEnabled() )
    {
         tcInventory->animateOut(5);
    }
    else if( tcGamepadUtility->isEnabled() )
    {
         tcGamepadUtility->animateOut(5);
    }
    else if( tcDPadInventory->isEnabled() )
    {
        tcDPadInventory->animateOut(5);
    }
    else if (controlsContainer.isEditing())
    {
        controlsContainer.finishEditing();
        return;
    }
    else if( showCustomMenu == true )
    {
        showCustomMenu = false;
    }
    else
    {
        PortableBackButton();
    }
}

static void hideControls( void )
{
    if (tcGameMain)
    {
        if (tcGameMain->isEnabled())
            tcGameMain->animateOut(30);

        tcWeaponWheel->animateOut(1);
    }
}

void gamepadAction(int state, int action)
{
    LOGI("gamepadAction, %d  %d", state, action );
    bool used = false;

    switch( action )
    {
        case PORT_ACT_MENU_UP:
            used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::UP );
            break;
        case PORT_ACT_MENU_DOWN:
            used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::DOWN );
            break;
        case PORT_ACT_MENU_LEFT:
             used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::LEFT );
            break;
        case PORT_ACT_MENU_RIGHT:
             used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::RIGHT );
            break;
        case PORT_ACT_MENU_SELECT:
            used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::SELECT );
            break;
        case PORT_ACT_MENU_BACK:
            used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::BACK );
            break;
    }

    if( used )
        return;

    if( tcKeyboard->isEnabled() )
    {
        // Do nothing for now, stops multiple controls opening
    }
    else if( tcInventory->isEnabled() )
    {
        // Do nothing for now, stops multiple controls opening
    }
    else if( tcGamepadUtility->isEnabled() )
    {
     // Do nothing for now, stops multiple controls opening
    }
    else if( tcDPadInventory->isEnabled() )
    {
     // Do nothing for now, stops multiple controls opening
    }
    else if( action == PORT_ACT_SHOW_KBRD )
    {
        if( state )
        {
            SDL_StartTextInput();
        }
    }
    else if( action == PORT_ACT_SHOW_GP_UTILS )
    {
        if( state )
        {
            if( !tcGamepadUtility->isEnabled() )
            {
                tcGamepadUtility->animateIn(5);
            }
        }
    }
    else if( action == PORT_ACT_SHOW_DPAD_INV  && currentScreenMode == TS_GAME )
    {
        if( state )
        {
            if( !tcDPadInventory->isEnabled() )
            {
                //tcDPadInventory->animateIn(5);
                tcDPadInventory->setEnabled(true);
            }
        }
    }
    else if( action == PORT_ACT_SHOW_INV && currentScreenMode == TS_GAME )
    {
        if( state )
        {
            if( !tcInventory->isEnabled() )
            {
                tcInventory->animateIn(5);
            }
        }
    }
    else if( action == PORT_ACT_MENU_SHOW )
    {
        if( state )
        {
            mobileBackButton();
        }
    }
    else
    {
        PortableAction(state,action);
        if( gamepadHideTouch )
            hideControls();
    }
}


TouchControlsInterface* mobileGetTouchInterface()
{
    return &controlsContainer;
}

}