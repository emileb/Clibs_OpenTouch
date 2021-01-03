
#include "TouchControlsInterface.h"
#include "TouchControlsContainer.h"
#include "SDL_beloko_extra.h"
#include "SDL.h"
//#include "ios_interface.h"

#include <GLES/gl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

#define COMMAND_SET_BACKLIGHT      0x8001
#define COMMAND_SHOW_GYRO_OPTIONS  0x8002
#define COMMAND_SHOW_KEYBOARD      0x8003
#define COMMAND_SHOW_GAMEPAD       0x8004
#define COMMAND_VIBRATE            0x8005
#define COMMAND_LOAD_SAVE_CONTROLS 0x8006

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
#define KEY_USE_MOUSE    0x100E
#define KEY_LEFT_MOUSE   0x100F
#define KEY_LOAD_SAVE_CONTROLS 0x1010


#define GAME_TYPE_DOOM     1 // Dont use 0 so we can detect serialization
#define GAME_TYPE_HEXEN    2
#define GAME_TYPE_HERETIC  3
#define GAME_TYPE_STRIFE   4

	int gameType;

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

	static int volume_up_action;
	static int volume_down_action;

//To be set by android
	static float strafe_sens = 1;
	static float forward_sens = 1;
	static float pitch_sens = 1;
	static float yaw_sens = 1;

	static float precisionSensitivty = 0.5;

	static bool m_shooting = false;

	static bool useMouse = false;
	static bool gameShowMouse = false;

// Show buttons in game
	static bool showCustomAlways = false;
	static bool showCustomOn = false;
	static bool showWeaponNumbersOn = false;

// Show custom buttons in the menu
	static bool showCustomMenu = false;

	static bool useSystemKeyboard = false;


	static bool weaponWheelMoveStick = true;
	static touchcontrols::WheelSelectMode weaponWheelGamepadMode = touchcontrols::WHEELSELECT_GP_MODE_HOLD;
	static int weaponWheelAutoTimout = 0;

	static int controlsCreated = 0;
	static touchcontrols::TouchControlsContainer controlsContainer;

	static int mapState = 0;

#define DEMO_ALPHA_RESET      30
#define DEMO_ALPFA_DEC        0.1f
	static float demoControlsAlpha; // Used to fade out demo controls

	static touchcontrols::UI_Controls *UI_tc = 0;

	static touchcontrols::TouchControls *tcMenuMain = 0;
	static touchcontrols::TouchControls *tcYesNo = 0;
	static touchcontrols::TouchControls *tcGameMain = 0;
	static touchcontrols::TouchControls *tcGameWeapons = 0;
	static touchcontrols::TouchControls *tcWeaponWheel = 0;
	static touchcontrols::TouchControls *tcInventory = 0;
	static touchcontrols::TouchControls *tcAutomap = 0;
	static touchcontrols::TouchControls *tcBlank = 0;
	static touchcontrols::TouchControls *tcKeyboard = 0;
	static touchcontrols::TouchControls *tcCustomButtons = 0;
	static touchcontrols::TouchControls *tcDemo = 0;
	static touchcontrols::TouchControls *tcGamepadUtility = 0;
	static touchcontrols::TouchControls *tcDPadInventory = 0;
	static touchcontrols::TouchControls *tcMouse = 0;
#ifdef D3ES
	static touchcontrols::TouchControls *tcPda = 0;
#endif

// So can hide and show these buttons
	static touchcontrols::TouchJoy *touchJoyLeft;
	static touchcontrols::TouchJoy *touchJoyRight;

// So gampad can control keyboard
	static touchcontrols::UI_Keyboard *uiKeyboard;

// So gamepad can access it
	static touchcontrols::WheelSelect *wheelSelect;

	static touchcontrols::ButtonGrid *uiInventoryButtonGrid;

	static std::string graphicpath;
	static const char * getFilesPath()
	{
		return graphicpath.c_str(); //graphics path is the same as files path
	}

	std::string game_path;
	const char * getGamePath()
	{
		return game_path.c_str();
	}

	static int touchActionToAction(int action)
	{
		int ret = 0;

		switch(action)
		{
		case 1:
			ret = PORT_ACT_USE;
			break;

		case 2:
			ret = PORT_ACT_JUMP;
			break;

		case 3:
			ret = PORT_ACT_ATTACK;
			break;

		case 4:
			ret = PORT_ACT_ALT_ATTACK;
			break;

		default:
			ret = 0;
		}

		return ret;
	}

// Needed for Doom 3
	extern const char *nativeLibsPath;

	extern std::string userFilesPath;

// Send message to JAVA SDL activity
	int Android_JNI_SendMessage(int command, int param);

#if defined(PRBOOM_DOOM)
	typedef enum
	{
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

#endif


	static void openGLStart()
	{
#if 1
		touchcontrols::gl_startRender();

#if !defined(D3ES)

		if(touchcontrols::gl_getGLESVersion() == 1)
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
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_TEXTURE_2D);

			glDisable(GL_ALPHA_TEST);
		}

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



#if ( defined(GZDOOM) || defined(ZANDRONUM_30) ) && !defined(GZDOOM_GL3)

		if(touchcontrols::gl_getGLESVersion() == 1)
		{
			if(currentrenderer == 1) //GL mode
			{
				void jwzgles_restore(void);
				jwzgles_restore();
			}
			else
			{
				sdlSWMode = true;
			}
		}
		else if(touchcontrols::gl_getGLESVersion() == 2)
		{
			touchcontrols::gl_resetGL4ES();
		}

#endif

#if !defined(GZDOOM_GL3) && !defined(D3ES)

// Setup for SDL Software rendering again
		if(SW_SDL_RENDER || sdlSWMode)
		{
			glDisable(GL_BLEND);
			glColor4f(1, 1, 1, 1);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}

#endif

#if defined(PRBOOM_DOOM)

		if(V_GetMode() == VID_MODEGL)
		{
			void jwzgles_restore(void);
			jwzgles_restore();
		}

#endif

#endif
	}

	static void vibrate(int duration)
	{
		Android_JNI_SendMessage(COMMAND_VIBRATE, duration);
	}

	static void gameSettingsButton(int state)
	{
		if(state == 1)
		{
			controlsContainer.showUIWindow(UI_tc);
		}
	}

	static void customSettingsButton(int state)
	{
		if(state == 1)
		{
			showButtonListWindow(&controlsContainer);
			//showEditButtons();
		}
	}


	static int licTest = 0;
	int keyCheck();
	extern char keyGlobal[];

	static void gameButton(int state, int code)
	{
#ifndef NO_SEC

		if(state)
		{
			if(licTest < 0)
				return;

			licTest++;

			if(licTest == 64)
			{
				if(keyCheck() == 0)
				{
					// Failed
					licTest = -1;
				}
				else
				{
					// Now make it fail
					keyGlobal[4] = keyGlobal[4] ^ 0xAA;

					if(keyCheck() == 1)
					{
						// Failed, keyCheck always returns valid!!
						licTest = -1;
					}

					// Put back
					keyGlobal[4] = keyGlobal[4] ^ 0xAA;
				}
			}
		}

#endif

		if(code == KEY_SHOOT)
		{
			m_shooting = state;
			PortableAction(state, PORT_ACT_ATTACK);
		}
		else if(code == KEY_SHOW_WEAPONS)
		{
			if(state == 1)
			{
				if(!tcGameWeapons->enabled)
				{
					showWeaponNumbersOn = true;
					tcGameWeapons->animateIn(5);
				}
				else
				{
					showWeaponNumbersOn = false;
					tcGameWeapons->animateOut(5);
				}
			}
		}
		else if(code == KEY_SHOW_INV)
		{
			if(state == 1)
			{
				if(!tcInventory->enabled)
				{
					tcInventory->animateIn(5);
				}
				else
					tcInventory->animateOut(5);
			}
		}
		else if(code == KEY_SHOW_CUSTOM)
		{
			if(state == 1)
			{
				if(!tcCustomButtons->enabled)
				{
					tcCustomButtons->setEnabled(true);
					tcCustomButtons->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
					showCustomOn = true;
				}
				else
				{
					tcCustomButtons->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
					showCustomOn = false;
				}
			}
		}
		else if(code == KEY_SHOW_KBRD)
		{
			if(state)
			{
				SDL_StartTextInput();
			}
		}
		else if(code == KEY_BACK_BUTTON)
		{
			if(state)
			{
				mobileBackButton();
			}
		}
		else if(code == KEY_USE_MOUSE)
		{
			if(state)
			{
				useMouse = true;
			}
		}
		else if(code == PORT_ACT_MAP)
		{
			if(state)
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


	static void automapButton(int state, int code)
	{
		if(state && code == PORT_ACT_MAP && mapState == 0)
		{
#ifdef RETRO_DOOM // Turn on follow mode to allow movment in map mode
			void AM_ToggleFollowMode(void);
			AM_ToggleFollowMode();
#endif
			mapState = 1;
		}
	}


	static void gameUtilitiesButton(int state, int code)
	{
		// Auto hide the gamepad utilitie, except if showing consol
		if((code != PORT_ACT_CONSOLE) && (tcGamepadUtility->isEnabled()))
		{
			tcGamepadUtility->setEnabled(false);
		}

		gameButton(state, code);
	}

	static void gameUtilitiesOutside(bool fromGamepad)
	{
		if(tcGamepadUtility->isEnabled())
		{
			tcGamepadUtility->setEnabled(false);
		}
	}

	static void blankButton(int state, int code)
	{
#if defined(PRBOOM_DOOM)

		if(state)
			PortableBackButton();

#elif defined(D3ES) // Blank is used for cinematic, allow to skip them

		if(state)
			mobileBackButton();

#else
		PortableAction(state, PORT_ACT_USE);
		PortableKeyEvent(state, SDL_SCANCODE_RETURN, 0);
#endif
	}

	static void customButton(int state, int code)
	{
		LOGI("Cust %d, %d", state, code);
		PortableAction(state, code);

		// If we are binding keys hide the custom buttons again
		if(state == 1)
			showCustomMenu = false;
	}

//Weapon wheel callbacks
	static void weaponWheelSelected(int enabled)
	{
		if(enabled)
			tcWeaponWheel->fade(touchcontrols::FADE_IN, 5); //fade in
	}

	static void weaponWheel(int segment)
	{
		LOGI("weaponWheel %d", segment);
		int number = 0;

		if(segment == 9)   // Press '0'
			number = 0;
		else
			number = 1 + segment;

		PortableAction(1, PORT_ACT_WEAP0 + number);
		PortableAction(0, PORT_ACT_WEAP0 + number);
	}

	static void menuButton(int state, int code)
	{
		if(code == KEY_SHOW_KBRD)
		{
			if(state)
				SDL_StartTextInput();

			return;
		}
		else if(code == KEY_BACK_BUTTON)
		{
			if(state)
				mobileBackButton();
		}
		else if(code == KEY_SHOW_CUSTOM)
		{
			if(state)
				showCustomMenu = true;
		}
		else if(code == SDL_SCANCODE_F10)  //for choch setup f10 to work!
		{
			PortableKeyEvent(state, code, 0);
		}
		else  if(code == KEY_SHOW_GYRO)
		{
			// Show gyro options
			if(state)
				Android_JNI_SendMessage(COMMAND_SHOW_GYRO_OPTIONS, 0);
		}
		else  if(code == KEY_LOAD_SAVE_CONTROLS)
		{
			// Show load save controls
			if(state)
				Android_JNI_SendMessage(COMMAND_LOAD_SAVE_CONTROLS, 0);
		}
		else  if(code == KEY_SHOW_GAMEPAD)
		{
			// Show gamepad options
			if(state)
				Android_JNI_SendMessage(COMMAND_SHOW_GAMEPAD, 0);
		}
		else if(code == KEY_USE_MOUSE)
		{
			if(state)
			{
				useMouse = true;
			}
		}
		else if(code == PORT_ACT_CONSOLE)
		{
			PortableKeyEvent(state, SDL_SCANCODE_GRAVE, 0);
		}
		else if(code == KEY_LEFT_MOUSE)
		{
#ifdef D3ES
			PortableMouseButton(state, 1, 0, 0);
#endif
		}
		else
		{
			PortableAction(state, code);
		}

		demoControlsAlpha = DEMO_ALPHA_RESET;
	}

	static void inventoryButton(int state, int code)
	{
		PortableAction(state, code);
	}

	static void inventoryOutside(bool fromGamepad)
	{
		if(autoHideInventory || fromGamepad)
		{
			if(tcInventory->enabled)
				tcInventory->animateOut(5);
		}
	}

	static void dPadInventoryButton(int state, int code)
	{
		PortableAction(state, code);
	}

	static void dPadInventoryOutside(bool fromGamepad)
	{
		if(tcDPadInventory->enabled)
			tcDPadInventory->animateOut(5);
	}

	static void left_double_tap(int state)
	{
		//LOGTOUCH("L double %d",state);
		if(left_double_action)
			PortableAction(state, left_double_action);
	}

	static void right_double_tap(int state)
	{
		//LOGTOUCH("R double %d",state);
		if(right_double_action)
			PortableAction(state, right_double_action);
	}

	static void left_stick(float joy_x, float joy_y, float mouse_x, float mouse_y)
	{
		float fwdback = joy_y * 15 * forward_sens;
		float strafe  = -joy_x * 10 * strafe_sens;

		PortableMove(fwdback, strafe);
	}

	static void right_stick(float joy_x, float joy_y, float mouse_x, float mouse_y)
	{
		//LOGI(" mouse x = %f",mouse_x);
		int invert        = invertLook ? -1 : 1;
		float scale       = (m_shooting && precisionShoot) ? precisionSensitivty : 1;
		float pitchMouse       = mouse_y * pitch_sens * invert * scale;
		float pitchJoystick    = joy_y * pitch_sens * invert * scale * -2;

		float yawMouse    = mouse_x * yaw_sens * scale;
		float yawJoystick = joy_x  * yaw_sens * scale * 10;

		if(joystickLookMode)
			PortableLookPitch(LOOK_MODE_JOYSTICK, pitchJoystick);
		else
			PortableLookPitch(LOOK_MODE_MOUSE, pitchMouse);

		if(joystickLookMode)
			PortableLookYaw(LOOK_MODE_JOYSTICK, yawJoystick);
		else
			PortableLookYaw(LOOK_MODE_MOUSE, yawMouse);
	}


//Weapon select callbacks
	static void selectWeaponButton(int state, int code)
	{
		PortableAction(state, PORT_ACT_WEAP0 + code);

		if(autoHideNumbers && state == 0)
		{
			showWeaponNumbersOn = false;
			tcGameWeapons->animateOut(5);
		}
	}

	static void automap_multitouch_mouse_move(int action, float x, float y, float dx, float dy)
	{
		if(action == MULTITOUCHMOUSE_MOVE)
		{
			PortableAutomapControl(0, dx, dy);
		}
		else if(action == MULTITOUCHMOUSE_ZOOM)
		{
			PortableAutomapControl(x, 0, 0);
		}
	}

	static void showHideKeyboard(int show)
	{
		LOGI("showHideKeyboard %d", show);

		if(useSystemKeyboard)
		{
			if(show)
				Android_JNI_SendMessage(COMMAND_SHOW_KEYBOARD, 0);
		}
		else
		{
			tcKeyboard->setEnabled(show);
		}
	}

	static void brightnessSlideMouse(int action, float x, float y, float dx, float dy)
	{
		y = 1 - y;
		Android_JNI_SendMessage(COMMAND_SET_BACKLIGHT, y * 255);
	}

	static void touchSettings(touchcontrols::tTouchSettings settings)
	{
		gameControlsAlpha = settings.alpha;

		invertLook = settings.invertLook;

		strafe_sens = settings.strafeSensitivity;
		forward_sens = settings.fwdSensitivity;
		pitch_sens = settings.lookSensitivity;
		yaw_sens = settings.turnSensitivity;

		showSticks = settings.showJoysticks;
		precisionShoot = settings.precisionShoot;
		precisionSensitivty = settings.precisionSenitivity;

		showCustomAlways = settings.alwaysShowCust;

		joystickLookMode =  settings.joystickLookMode;
		autoHideInventory = settings.autoHideInventory;
		autoHideNumbers = settings.autoHideNumbers;
		weaponWheelEnabled  = settings.weaponWheelEnabled;

		left_double_action = touchActionToAction(settings.dblTapLeft);
		right_double_action = touchActionToAction(settings.dblTapRight);

		volume_up_action = touchActionToAction(settings.volumeUp);
		volume_down_action = touchActionToAction(settings.volumeDown);

		controlsContainer.setColour(settings.defaultColor);
		controlsContainer.setAlpha(gameControlsAlpha);

		touchJoyLeft->setCenterAnchor(settings.fixedMoveStick);

		if(tcGameMain) tcGameMain->setAlpha(gameControlsAlpha);

		if(tcCustomButtons)  tcCustomButtons->setAlpha(gameControlsAlpha);

		if(tcYesNo) tcYesNo->setColour(settings.defaultColor);

		if(tcGameMain) tcGameMain->setColour(settings.defaultColor);

		if(tcGameWeapons) tcGameWeapons->setColour(settings.defaultColor);

		if(tcWeaponWheel) tcWeaponWheel->setColour(settings.defaultColor);

		if(tcInventory) tcInventory->setColour(settings.defaultColor);

		if(tcAutomap) tcAutomap->setColour(settings.defaultColor);

		if(tcCustomButtons) tcCustomButtons->setColour(settings.defaultColor);

		if(tcGamepadUtility) tcGamepadUtility->setColour(settings.defaultColor);

		if(tcDPadInventory) tcDPadInventory->setColour(settings.defaultColor);
	}

	extern SDL_Scancode SDLCALL SDL_GetScancodeFromKey(SDL_Keycode key);
	extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
	extern int SDL_SendEditingText(const char *text, int start, int end);
	extern int SDL_SendKeyboardText(const char *text);
	extern void SDL_StartTextInput(void);
	extern void SDL_StopTextInput(void);
#include <unistd.h>
	static void keyboardKeyPressed(uint32_t key)
	{
		LOGI("Keyboard press: %d , %c", key, (char)key);

		if(key == UI_KEYBOARD_HIDE)
		{
			SDL_StopTextInput();
			return;
		}

		char text[2] = {0, 0};

		// Only send printable chars
		if(key >= 32 && key <= 125)
		{
			text[0] = key;
			text[1] = 0;

		}

		// Change upper case to lower case to get scan code
		if(key >= 65 && key <= 90)
		{
			key = key + 32;
		}

		SDL_Scancode sc = SDL_GetScancodeFromKey(key);

		// Send scancode
		if(sc != SDL_SCANCODE_UNKNOWN)
		{
			SDL_SendKeyboardKey(SDL_PRESSED, sc);
			SDL_SendKeyboardKey(SDL_RELEASED, sc);
		}

		// Send text if avaliable
		if(text[0])
			SDL_SendKeyboardText(text);
	}

	static void mouse_move(int action, float x, float y, float mouse_x, float mouse_y)
	{
#if defined(GZDOOM) || defined(ZANDRONUM_30) || defined(D3ES)

		if(action == TOUCHMOUSE_MOVE)
		{
			PortableMouse(mouse_x, mouse_y);
		}
		else if(action == TOUCHMOUSE_TAP)
		{
			PortableMouseButton(1, 1, 0, 0);
			usleep(200 * 1000); // Need this for the PDA to work in D3, needs a frame to react..
			PortableMouseButton(0, 1, 0, 0);
		}

#endif
	}

	static void mouseButton(int state, int code)
	{
#if defined(GZDOOM) || defined(ZANDRONUM_30) || defined(D3ES)

		// Hide the mouse
		if((code == KEY_USE_MOUSE) && state)
		{
			useMouse = false;
		}
		else if(code == KEY_LEFT_MOUSE)
		{
			PortableMouseButton(state, 1, 0, 0);
		}
		else if((code == KEY_BACK_BUTTON) && state)
		{
			mobileBackButton();
		}

		LOGI("useMouse = %d", useMouse);
#endif
	}

	static void showSDLMouseCallback(int show)
	{
		LOGI("showSDLMouseCallback = %d", show);
		gameShowMouse = show;
	}

	static void moveSDLMouseCallback(float x, float y)
	{
		LOGI("moveSDLMouseCallback = %f, %f", x, y);
		controlsContainer.mousePos(x, y);
	}


	static void setHideSticks(bool v)
	{
		if(touchJoyLeft) touchJoyLeft->setHideGraphics(v);

		if(touchJoyRight) touchJoyRight->setHideGraphics(v);
	}

	static void updateTouchScreenMode(touchscreemode_t mode)
	{
		// LOGI("updateTouchScreenModeA %d",mode);

		if(mode != currentScreenMode)
		{
			//first disable the last screen and fade out is necessary
			switch(currentScreenMode)
			{
			case TS_BLANK:
				tcBlank->resetOutput();
				tcBlank->setEnabled(false);
				break;

			case TS_MENU:
				tcMenuMain->resetOutput();
				tcMenuMain->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
				break;

			case TS_Y_N:
				tcYesNo->resetOutput();
				tcYesNo->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
				break;

			case TS_GAME:
				tcGameMain->resetOutput();
				tcGameMain->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
				tcGameWeapons->setEnabled(false);
				tcWeaponWheel->setEnabled(false);

				if(tcCustomButtons)
				{
					tcCustomButtons->resetOutput();
					tcCustomButtons->setEnabled(false);
				}

				if(tcInventory)
				{
					tcInventory->resetOutput();
					tcInventory->setEnabled(false);
				}

				break;

			case TS_MAP:
				tcAutomap->resetOutput();
				tcAutomap->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
				break;

			case TS_CUSTOM:
				tcCustomButtons->resetOutput();
				tcCustomButtons->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
				break;

			case TS_DEMO:
				tcDemo->resetOutput();
				tcDemo->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
				break;

			case TS_MOUSE:
				tcMouse->resetOutput();
				tcMouse->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
				break;
#ifdef D3ES

			case TS_PDA:
				tcPda->resetOutput();
				tcPda->fade(touchcontrols::FADE_OUT, DEFAULT_FADE_FRAMES);
				break;
#else

			case TS_PDA:
				break;
#endif

			case TS_CONSOLE:
				break;
			}

			//Enable the current new screen
			switch(mode)
			{
			case TS_BLANK:
				tcBlank->setEnabled(true);
				break;

			case TS_MENU:
				useMouse = false;

				if(!hideGameAndMenu)
				{
					tcMenuMain->setEnabled(true);
					tcMenuMain->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
				}

				break;

			case TS_Y_N:
				tcYesNo->setEnabled(true);
				tcYesNo->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
				break;

			case TS_GAME:
				useMouse = false;

				// Always set these so they are never wrong
				if(tcGameMain) tcGameMain->setAlpha(gameControlsAlpha);

				if(tcCustomButtons) tcCustomButtons->setAlpha(gameControlsAlpha);

				if(!hideGameAndMenu)
				{
					tcGameMain->setEnabled(true);
					tcGameMain->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);

					if(showCustomOn || showCustomAlways)   // Also remember if custom buttons were shown
					{
						tcCustomButtons->setEnabled(true);
						tcCustomButtons->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
					}

					// Bring back weapon numbers
					if(showWeaponNumbersOn)
					{
						tcGameWeapons->animateIn(5);
					}
				}

				// Always enable wheel
				if(weaponWheelEnabled)
					tcWeaponWheel->setEnabled(true);

				break;

			case TS_MAP:
			{
				// This is a bit of hack. We want the map button to be in the same place as the game map button
				// So move it
				touchcontrols::Button* mapGame = (touchcontrols::Button*)tcGameMain->getControl("map");
				touchcontrols::Button* map = (touchcontrols::Button*)tcAutomap->getControl("map");
				map->controlPos = mapGame->controlPos;
				map->updateSize();
			}

			tcAutomap->setEnabled(true);
			tcAutomap->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
			break;

			case TS_CUSTOM:
				tcCustomButtons->setEnabled(true);
				tcCustomButtons->setAlpha(1.0);
				tcCustomButtons->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
				break;

			case TS_DEMO:
				tcDemo->setEnabled(true);
				tcDemo->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
				break;

			case TS_MOUSE:
				tcMouse->setEnabled(true);
				tcMouse->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
				break;
#ifdef D3ES

			case TS_PDA:
			{
				// Copy position from the game screen
				touchcontrols::Button* pdaGame = (touchcontrols::Button*)tcGameMain->getControl("pda");
				touchcontrols::Button* pda = (touchcontrols::Button*)tcPda->getControl("pda");
				pda->controlPos = pdaGame->controlPos;
				pda->updateSize();

				tcPda->setEnabled(true);
				tcPda->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);
			}
			break;
#else

			case TS_PDA:
				break;
#endif

			case TS_CONSOLE:
				break;
			}

			currentScreenMode = mode;
		}
	}

	void frameControls()
	{
#if !defined(NO_SEC)
		unsigned char sha_data[20] = {0x23, 0x53, 0xff, 0x41, 0x16, 0xd4, 0x43, 0x7f, 0x43, 0xaf, 0x12, 0x19, 0x75, 0xaa, 0xd7, 0xb0, 0x5e, 0xee, 0xf5, 0xde};
#include "./secure/check_include.h"
#endif

		//static bool inited = false;
		if(SDL_NewEGLCreated())
		{
			LOGI("NEW EGL CONTEXT");
			touchcontrols::clearGlTexCache();
			controlsContainer.initGL();
		}

		touchscreemode_t screenMode = PortableGetScreenMode();

		if((screenMode == TS_MAP) && (mapState == 1))
		{
			screenMode =  TS_GAME;
		}

		// Hack to show custom buttons while in the menu to bind keys
		if(screenMode == TS_MENU && showCustomMenu == true)
		{
			screenMode = TS_CUSTOM;
		}

		if(screenMode == TS_DEMO)   // Fade out demo buttons if not touched for a while
		{
			if(demoControlsAlpha > 0)
			{
				demoControlsAlpha -= DEMO_ALPFA_DEC;
			}

			tcDemo->setAlpha(demoControlsAlpha);
		}

		if(((screenMode == TS_GAME) || (screenMode == TS_MENU)) & useMouse)   // Show mouse screen
		{
			screenMode = TS_MOUSE;
		}

		if(gameShowMouse && useMouse)
			controlsContainer.showMouse(true);
		else
			controlsContainer.showMouse(false);

		updateTouchScreenMode(screenMode);

		setHideSticks(!showSticks);

//openGLStart();
//openGLEnd();
		controlsContainer.draw();
	}


	static void createCustomControls(touchcontrols::TouchControls* customControls)
	{
		customControls->addControl(new touchcontrols::Button("A", touchcontrols::RectF(5, 5, 7, 7), "Custom_1", PORT_ACT_CUSTOM_0, false, false, "Custom 1 (KP1)", touchcontrols::COLOUR_RED2));
		customControls->addControl(new touchcontrols::Button("B", touchcontrols::RectF(7, 5, 9, 7), "Custom_2", PORT_ACT_CUSTOM_1, false, false, "Custom 2 (KP2)", touchcontrols::COLOUR_RED2));
		customControls->addControl(new touchcontrols::Button("C", touchcontrols::RectF(5, 7, 7, 9), "Custom_3", PORT_ACT_CUSTOM_2, false, false, "Custom 3 (KP3)", touchcontrols::COLOUR_BLUE1));

		customControls->addControl(new touchcontrols::Button("D", touchcontrols::RectF(7, 7, 9, 9), "Custom_4", PORT_ACT_CUSTOM_3, false, false, "Custom 4 (KP4)", touchcontrols::COLOUR_BLUE1));
		customControls->addControl(new touchcontrols::Button("E", touchcontrols::RectF(5, 9, 7, 11), "Custom_5", PORT_ACT_CUSTOM_4, false, false, "Custom 5 (KP5)", touchcontrols::COLOUR_GREEN2));
		customControls->addControl(new touchcontrols::Button("F", touchcontrols::RectF(7, 9, 9, 11), "Custom_6", PORT_ACT_CUSTOM_5, false, false, "Custom 6 (KP6)", touchcontrols::COLOUR_GREEN2));

		customControls->addControl(new touchcontrols::Button("G", touchcontrols::RectF(5, 11, 7, 13), "custom_a", PORT_ACT_CUSTOM_6, false, true, "Custom 7 (KP7)", touchcontrols::COLOUR_NONE));
		customControls->addControl(new touchcontrols::Button("H", touchcontrols::RectF(7, 11, 9, 13), "custom_b", PORT_ACT_CUSTOM_7, false, true, "Custom 8 (KP8)", touchcontrols::COLOUR_NONE));
		customControls->addControl(new touchcontrols::Button("I", touchcontrols::RectF(5, 13, 7, 15), "custom_c", PORT_ACT_CUSTOM_8, false, true, "Custom 9 (KP9)", touchcontrols::COLOUR_NONE));
		customControls->addControl(new touchcontrols::Button("J", touchcontrols::RectF(7, 13, 9, 15), "custom_d", PORT_ACT_CUSTOM_9, false, true, "Custom 10 (KP0)", touchcontrols::COLOUR_NONE));

		touchcontrols::QuadSlide *qs1 = new touchcontrols::QuadSlide("quad_slide_1", touchcontrols::RectF(10, 7, 12, 9), "quad_slide", "slide_arrow", PORT_ACT_CUSTOM_10, PORT_ACT_CUSTOM_11, PORT_ACT_CUSTOM_12, PORT_ACT_CUSTOM_13, false, "Quad Slide 1 (A - D)");
		customControls->addControl(qs1);

		touchcontrols::QuadSlide *qs2 = new touchcontrols::QuadSlide("quad_slide_2", touchcontrols::RectF(14, 7, 16, 9), "quad_slide", "slide_arrow", PORT_ACT_CUSTOM_14, PORT_ACT_CUSTOM_15, PORT_ACT_CUSTOM_16, PORT_ACT_CUSTOM_17, false, "Quad Slide 2 (E - H)");
		customControls->addControl(qs2);

		touchcontrols::QuadSlide *qs3 = new touchcontrols::QuadSlide("quad_slide_3", touchcontrols::RectF(10, 11, 12, 13), "quad_slide_2", "slide_arrow", PORT_ACT_CUSTOM_18, PORT_ACT_CUSTOM_19, PORT_ACT_CUSTOM_20, PORT_ACT_CUSTOM_21, true, "Quad Slide 3 (I - L)");
		customControls->addControl(qs3);

		touchcontrols::QuadSlide *qs4 = new touchcontrols::QuadSlide("quad_slide_4", touchcontrols::RectF(14, 11, 16, 13), "quad_slide_2", "slide_arrow", PORT_ACT_CUSTOM_22, PORT_ACT_CUSTOM_23, PORT_ACT_CUSTOM_24, PORT_ACT_CUSTOM_25, true, "Quad Slide 4 (M - P)");
		customControls->addControl(qs4);
		//customControls->setColor(0.7,0.7,1.f);

		qs1->signal.connect(sigc::ptr_fun(&customButton));
		qs2->signal.connect(sigc::ptr_fun(&customButton));
		qs3->signal.connect(sigc::ptr_fun(&customButton));
		qs4->signal.connect(sigc::ptr_fun(&customButton));
		customControls->signal_button.connect(sigc::ptr_fun(&customButton));
		customControls->signal_settingsButton.connect(sigc::ptr_fun(&customSettingsButton));
		customControls->setAlpha(gameControlsAlpha);
	}

#ifdef D3ES
	void initControlsDoom3(const char * xmlPath)
	{
		LOGI("initControlsDoom3");

		if(!controlsCreated)
		{
			LOGI("creating controls");

			tcMenuMain = new touchcontrols::TouchControls("menu", false, true, 10, false);
			tcGameMain = new touchcontrols::TouchControls("game", false, true, 1, true);
			tcGameWeapons = new touchcontrols::TouchControls("weapons", false, true, 1, false);
			tcWeaponWheel = new touchcontrols::TouchControls("weapon_wheel", false, true, 1, false);
			tcBlank = new touchcontrols::TouchControls("blank", true, false);
			tcKeyboard = new touchcontrols::TouchControls("keyboard", false, false);
			tcCustomButtons = new touchcontrols::TouchControls("custom_buttons", false, true, 1, true);
			tcPda = new touchcontrols::TouchControls("pda", false, false);
			tcGamepadUtility = new touchcontrols::TouchControls("gamepad_utility", false, false);

			// Hide the cog because when using the gamepad and weapon wheel is enabled, the cog will show otherwise
			tcWeaponWheel->hideEditButton = true;

			touchcontrols::Mouse *mouse = new touchcontrols::Mouse("mouse", touchcontrols::RectF(0, 2, 26, 16), ""); // Leave gap at top so dont click mouse when pressing those buttons
			mouse->setHideGraphics(true);
			mouse->setEditable(false);
			tcMenuMain->addControl(mouse);
			mouse->signal_action.connect(sigc::ptr_fun(&mouse_move));
			tcMenuMain->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON, false, false, "Back"));
			tcMenuMain->addControl(new touchcontrols::Button("keyboard", touchcontrols::RectF(2, 0, 4, 2), "keyboard", KEY_SHOW_KBRD));
			tcMenuMain->addControl(new touchcontrols::Button("gamepad", touchcontrols::RectF(22, 0, 24, 2), "gamepad", KEY_SHOW_GAMEPAD));
			tcMenuMain->addControl(new touchcontrols::Button("gyro", touchcontrols::RectF(24, 0, 26, 2), "gyro", KEY_SHOW_GYRO));
			tcMenuMain->addControl(new touchcontrols::Button("show_custom", touchcontrols::RectF(9, 0, 11, 2), "custom_show", KEY_SHOW_CUSTOM));

			// Hide mouse button, try to use tap for now..
			//tcMenuMain->addControl(new touchcontrols::Button("left_button",touchcontrols::RectF(0,6,3,10),"left_mouse",KEY_LEFT_MOUSE,false,false,"Back"));

			tcMenuMain->signal_button.connect(sigc::ptr_fun(&menuButton));

			// GAME------------------------------------------------------------------------------
			//-----------------------------------------------------------------------------------
			tcGameMain->setAlpha(gameControlsAlpha);
			tcGameMain->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON, false, false, "Show menu"));
			tcGameMain->addControl(new touchcontrols::Button("attack", touchcontrols::RectF(20, 7, 23, 10), "shoot", KEY_SHOOT, false, false, "Attack!"));
			tcGameMain->addControl(new touchcontrols::Button("attack2", touchcontrols::RectF(3, 5, 6, 8), "shoot", KEY_SHOOT, false, true, "Attack! (duplicate)"));
			tcGameMain->addControl(new touchcontrols::Button("quick_save", touchcontrols::RectF(24, 0, 26, 2), "save", PORT_ACT_QUICKSAVE, false, false, "Quick save"));
			tcGameMain->addControl(new touchcontrols::Button("quick_load", touchcontrols::RectF(20, 0, 22, 2), "load", PORT_ACT_QUICKLOAD, false, false, "Quick load"));
			tcGameMain->addControl(new touchcontrols::Button("keyboard", touchcontrols::RectF(8, 0, 10, 2), "keyboard", KEY_SHOW_KBRD, false, false, "Show keyboard"));

			tcGameMain->addControl(new touchcontrols::Button("jump", touchcontrols::RectF(24, 3, 26, 5), "jump", PORT_ACT_JUMP, false, false, "Jump"));
			tcGameMain->addControl(new touchcontrols::Button("crouch", touchcontrols::RectF(24, 14, 26, 16), "crouch", PORT_ACT_DOWN, false, true, "Crouch"));
			tcGameMain->addControl(new touchcontrols::Button("crouch_toggle", touchcontrols::RectF(24, 14, 26, 16), "crouch", PORT_ACT_TOGGLE_CROUCH, false, false, "Crouch (smart toggle)"));
			tcGameMain->addControl(new touchcontrols::Button("show_weapons", touchcontrols::RectF(12, 14, 14, 16), "show_weapons", KEY_SHOW_WEAPONS, false, false, "Show numbers"));
			tcGameMain->addControl(new touchcontrols::Button("next_weapon", touchcontrols::RectF(0, 3, 3, 5), "next_weap", PORT_ACT_NEXT_WEP, false, false, "Next weapon"));
			tcGameMain->addControl(new touchcontrols::Button("prev_weapon", touchcontrols::RectF(0, 5, 3, 7), "prev_weap", PORT_ACT_PREV_WEP, false, false, "Prev weapon"));
			tcGameMain->addControl(new touchcontrols::Button("reload", touchcontrols::RectF(3, 4, 5, 6), "reload", PORT_ACT_RELOAD, false, false, "Reload"));
			tcGameMain->addControl(new touchcontrols::Button("console", touchcontrols::RectF(6, 0, 8, 2), "tild", PORT_ACT_CONSOLE, false, true, "Console"));
			tcGameMain->addControl(new touchcontrols::Button("flashlight", touchcontrols::RectF(21, 3, 23, 5), "flashlight", PORT_ACT_FLASH_LIGHT, false, false, "Flashlight"));
			tcGameMain->addControl(new touchcontrols::Button("pda", touchcontrols::RectF(16, 0, 18, 2), "gamma", PORT_ACT_HELPCOMP, false, false, "Show PDA"));
			tcGameMain->addControl(new touchcontrols::Button("zoom", touchcontrols::RectF(18, 3, 20, 5), "zoom", PORT_ACT_ZOOM_IN, false, false, "Zoom (smart toggle)"));
			tcGameMain->addControl(new touchcontrols::Button("sprint", touchcontrols::RectF(0, 7, 2, 9), "sprint", PORT_ACT_SPRINT, false, false, "Sprint (smart toggle)"));
			tcGameMain->addControl(new touchcontrols::Button("show_custom", touchcontrols::RectF(0, 7, 2, 9), "custom_show", KEY_SHOW_CUSTOM, false, true, "Show custom"));

			touchJoyRight = new touchcontrols::TouchJoy("touch", touchcontrols::RectF(17, 4, 26, 16), "look_arrow", "fixed_stick_circle");
			tcGameMain->addControl(touchJoyRight);
			touchJoyRight->signal_move.connect(sigc::ptr_fun(&right_stick));
			touchJoyRight->signal_double_tap.connect(sigc::ptr_fun(&right_double_tap));

			touchJoyLeft = new touchcontrols::TouchJoy("stick", touchcontrols::RectF(0, 7, 8, 16), "strafe_arrow", "fixed_stick_circle");
			tcGameMain->addControl(touchJoyLeft);
			touchJoyLeft->signal_move.connect(sigc::ptr_fun(&left_stick));
			touchJoyLeft->signal_double_tap.connect(sigc::ptr_fun(&left_double_tap));

			// SWAPFIX
			touchJoyLeft->registerTouchJoySWAPFIX(touchJoyRight);
			touchJoyRight->registerTouchJoySWAPFIX(touchJoyLeft);


			tcGameMain->signal_button.connect(sigc::ptr_fun(&gameButton));
			tcGameMain->signal_settingsButton.connect(sigc::ptr_fun(&gameSettingsButton));

			//CUSTOM BUTTONS -------------------------------------------
			//-------------------------------------------
			createCustomControls(tcCustomButtons);

			//PDA -------------------------------------------
			//------------------------------------------------------
			tcPda->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON, false, false, "Show menu"));
			tcPda->addControl(new touchcontrols::Button("pda", touchcontrols::RectF(16, 0, 18, 2), "gamma", PORT_ACT_HELPCOMP, false, false, "Show PDA"));
			tcPda->addControl(mouse); // Try to add the same mosue object from the main menu.. should work?...
			tcPda->signal_button.connect(sigc::ptr_fun(&gameButton));

			//Blank -------------------------------------------
			//------------------------------------------------------
			tcBlank->addControl(new touchcontrols::Button("enter", touchcontrols::RectF(0, 0, 26, 16), "", 0x123));
			tcBlank->signal_button.connect(sigc::ptr_fun(&blankButton));

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

			tcGameWeapons->signal_button.connect(sigc::ptr_fun(&selectWeaponButton));
			tcGameWeapons->setAlpha(0.8);

			//Weapon wheel -------------------------------------------
			//------------------------------------------------------
			int weaponWheelNbr = 10;

			wheelSelect = new touchcontrols::WheelSelect("weapon_wheel", touchcontrols::RectF(7, 2, 19, 14), "weapon_wheel_%d", weaponWheelNbr);
			wheelSelect->signal_selected.connect(sigc::ptr_fun(&weaponWheel));
			wheelSelect->signal_enabled.connect(sigc::ptr_fun(&weaponWheelSelected));
			tcWeaponWheel->addControl(wheelSelect);
			tcWeaponWheel->setAlpha(0.8);

			//Gamepad utility -------------------------------------------
			//------------------------------------------------------
			touchcontrols::ButtonGrid *gamepadUtils = new touchcontrols::ButtonGrid("gamepad_grid", touchcontrols::RectF(8, 5, 18, 11), "gamepad_utils_bg", 3, 2);

			gamepadUtils->addCell(0, 0, "back_button", KEY_BACK_BUTTON);
			gamepadUtils->addCell(0, 1, "gamma", PORT_ACT_HELPCOMP);
			gamepadUtils->addCell(1, 0, "keyboard", KEY_SHOW_KBRD);
			gamepadUtils->addCell(1, 1, "flashlight", PORT_ACT_FLASH_LIGHT);
			gamepadUtils->addCell(2, 0, "save", PORT_ACT_QUICKSAVE);
			gamepadUtils->addCell(2, 1, "load", PORT_ACT_QUICKLOAD);

			gamepadUtils->signal_outside.connect(sigc::ptr_fun(&gameUtilitiesOutside));

			tcGamepadUtility->addControl(gamepadUtils);
			tcGamepadUtility->setAlpha(0.9);
			tcGamepadUtility->signal_button.connect(sigc::ptr_fun(&gameUtilitiesButton));


			//Keyboard -------------------------------------------
			//------------------------------------------------------
			uiKeyboard = new touchcontrols::UI_Keyboard("keyboard", touchcontrols::RectF(0, 8, 26, 16), "font_dual", 0, 0, 0);
			uiKeyboard->signal.connect(sigc::ptr_fun(&keyboardKeyPressed));
			tcKeyboard->addControl(uiKeyboard);
			// We want touch to pass through only where there is no keyboard
			tcKeyboard->setPassThroughTouch(touchcontrols::TouchControls::PassThrough::NO_CONTROL);

			UI_tc = touchcontrols::createDefaultSettingsUI(&controlsContainer, (std::string)xmlPath +  "/touch_settings_doom3.xml");
			UI_tc->setAlpha(1);

			//---------------------------------------------------------------
			//---------------------------------------------------------------
			controlsContainer.addControlGroup(tcKeyboard);
			controlsContainer.addControlGroup(tcGamepadUtility);
			controlsContainer.addControlGroup(tcMenuMain);
			controlsContainer.addControlGroup(tcCustomButtons);
			controlsContainer.addControlGroup(tcGameMain);
			controlsContainer.addControlGroup(tcGameWeapons);
			controlsContainer.addControlGroup(tcWeaponWheel);
			controlsContainer.addControlGroup(tcPda);
			controlsContainer.addControlGroup(tcBlank);

			controlsCreated = 1;

			tcMenuMain->setXMLFile((std::string)xmlPath +  "/menu_d3es.xml");
			tcGameMain->setXMLFile((std::string)xmlPath +  "/game_d3es.xml");
			tcWeaponWheel->setXMLFile((std::string)xmlPath +  "/weaponwheel_d3es.xml");
			tcGameWeapons->setXMLFile((std::string)xmlPath +  "/weapons_d3es.xml");
			tcCustomButtons->setXMLFile((std::string)xmlPath +  "/custom_buttons_d3es.xml");
		}
		else
			LOGI("NOT creating controls");

		SDL_SetSwapBufferCallBack(frameControls);

		SDL_SetShowKeyboardCallBack(showHideKeyboard);
	}
#endif // D3ES

	void initControls(const char * xmlPath)
	{
		LOGI("initControls");

		if(!controlsCreated)
		{
			LOGI("creating controls");

			tcMenuMain = new touchcontrols::TouchControls("menu", false, true, 10, false);
			tcYesNo = new touchcontrols::TouchControls("yes_no", false, false);
			tcGameMain = new touchcontrols::TouchControls("game", false, true, 1, true);
			tcGameWeapons = new touchcontrols::TouchControls("weapons", false, true, 1, false);
			tcInventory  = new touchcontrols::TouchControls("inventory", false, true, 2, false); //Different edit group
			tcWeaponWheel = new touchcontrols::TouchControls("weapon_wheel", false, true, 1, false);
			tcAutomap = new touchcontrols::TouchControls("automap", false, false);
			tcBlank = new touchcontrols::TouchControls("blank", true, false);
			tcCustomButtons = new touchcontrols::TouchControls("custom_buttons", false, true, 1, true);
			tcKeyboard = new touchcontrols::TouchControls("keyboard", false, false);
			tcDemo = new touchcontrols::TouchControls("demo_playback", false, false);
			tcGamepadUtility = new touchcontrols::TouchControls("gamepad_utility", false, false);
			tcDPadInventory = new touchcontrols::TouchControls("dpad_inventory", false, false);
			tcMouse = new touchcontrols::TouchControls("mouse", false, false);
			// Hide the cog because when using the gamepad and weapon wheel is enabled, the cog will show otherwise
			tcWeaponWheel->hideEditButton = true;

			//Menu -------------------------------------------
			//------------------------------------------------------
			tcMenuMain->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON));
			tcMenuMain->addControl(new touchcontrols::Button("down_arrow", touchcontrols::RectF(20, 13, 23, 16), "arrow_down", PORT_ACT_MENU_DOWN));
			tcMenuMain->addControl(new touchcontrols::Button("up_arrow", touchcontrols::RectF(20, 10, 23, 13), "arrow_up", PORT_ACT_MENU_UP));
			tcMenuMain->addControl(new touchcontrols::Button("left_arrow", touchcontrols::RectF(17, 13, 20, 16), "arrow_left", PORT_ACT_MENU_LEFT));
			tcMenuMain->addControl(new touchcontrols::Button("right_arrow", touchcontrols::RectF(23, 13, 26, 16), "arrow_right", PORT_ACT_MENU_RIGHT));
			tcMenuMain->addControl(new touchcontrols::Button("enter", touchcontrols::RectF(0, 10, 6, 16), "enter", PORT_ACT_MENU_SELECT));
			tcMenuMain->addControl(new touchcontrols::Button("keyboard", touchcontrols::RectF(2, 0, 4, 2), "keyboard", KEY_SHOW_KBRD));
			// Mouse pointer does not work with SDL
#if defined(GZDOOM) || defined(ZANDRONUM_30)
			tcMenuMain->addControl(new touchcontrols::Button("show_mouse", touchcontrols::RectF(4, 0, 6, 2), "mouse2", KEY_USE_MOUSE));
#endif
#ifndef CHOC_SETUP
			tcMenuMain->addControl(new touchcontrols::Button("gamepad", touchcontrols::RectF(22, 0, 24, 2), "gamepad", KEY_SHOW_GAMEPAD));
			tcMenuMain->addControl(new touchcontrols::Button("gyro", touchcontrols::RectF(24, 0, 26, 2), "gyro", KEY_SHOW_GYRO));
			tcMenuMain->addControl(new touchcontrols::Button("load_save_touch", touchcontrols::RectF(20, 0, 22, 2), "touchscreen_save", KEY_LOAD_SAVE_CONTROLS));
#endif
			//tcMenuMain->addControl(new touchcontrols::Button("brightness",touchcontrols::RectF(21,0,23,2),"brightness",KEY_BRIGHTNESS));
#ifdef CHOC_SETUP
			tcMenuMain->addControl(new touchcontrols::Button("f10", touchcontrols::RectF(24, 0, 26, 2), "key_f10", SDL_SCANCODE_F10));
#endif

#ifndef CHOC_SETUP
			touchcontrols::Mouse *brightnessSlide = new touchcontrols::Mouse("slide_mouse", touchcontrols::RectF(24, 3, 26, 11), "brightness_slider");
			brightnessSlide->signal_action.connect(sigc::ptr_fun(& brightnessSlideMouse));
			tcMenuMain->addControl(brightnessSlide);
#endif


			tcMenuMain->signal_button.connect(sigc::ptr_fun(&menuButton));
			tcMenuMain->setAlpha(0.8);
			tcMenuMain->setFixAspect(false);

			//Game -------------------------------------------
			//------------------------------------------------------
			tcGameMain->setAlpha(gameControlsAlpha);
			tcGameMain->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON, false, false, "Show menu"));
			tcGameMain->addControl(new touchcontrols::Button("attack", touchcontrols::RectF(20, 7, 23, 10), "shoot", KEY_SHOOT, false, false, "Attack!"));
			tcGameMain->addControl(new touchcontrols::Button("attack2", touchcontrols::RectF(3, 5, 6, 8), "shoot", KEY_SHOOT, false, true, "Attack! (duplicate)"));

			tcGameMain->addControl(new touchcontrols::Button("use", touchcontrols::RectF(23, 6, 26, 9), "use", PORT_ACT_USE, false, false, "Use/Open"));
			tcGameMain->addControl(new touchcontrols::Button("quick_save", touchcontrols::RectF(24, 0, 26, 2), "save", PORT_ACT_QUICKSAVE, false, false, "Quick save"));
			tcGameMain->addControl(new touchcontrols::Button("quick_load", touchcontrols::RectF(20, 0, 22, 2), "load", PORT_ACT_QUICKLOAD, false, false, "Quick load"));
			tcGameMain->addControl(new touchcontrols::Button("map", touchcontrols::RectF(2, 0, 4, 2), "map", PORT_ACT_MAP, false, false, "Show map"));
			tcGameMain->addControl(new touchcontrols::Button("keyboard", touchcontrols::RectF(8, 0, 10, 2), "keyboard", KEY_SHOW_KBRD, false, false, "Show keyboard"));
			tcGameMain->addControl(new touchcontrols::Button("show_mouse", touchcontrols::RectF(4, 0, 6, 2), "mouse2", KEY_USE_MOUSE, false, true, "Use mouse"));

#if defined(RETRO_DOOM) || defined(CHOCOLATE) || defined (PRBOOM_DOOM)
			tcGameMain->addControl(new touchcontrols::Button("gamma", touchcontrols::RectF(17, 0, 19, 2), "gamma", PORT_ACT_GAMMA, false, false, "Gamma"));
#endif

			bool hideJump = true;
			bool hideInventory = true;
			bool hideFlySlide = true;

			if((gameType == GAME_TYPE_STRIFE) || (gameType == GAME_TYPE_HEXEN))
				hideJump = false;

			if((gameType == GAME_TYPE_STRIFE) || (gameType == GAME_TYPE_HEXEN) || (gameType == GAME_TYPE_HERETIC))
				hideInventory = false;

			if(gameType == GAME_TYPE_HERETIC)
				hideFlySlide = false;

			// Quad slider for fly controls
			touchcontrols::QuadSlide *flyQs = new touchcontrols::QuadSlide("quad_slide_fly", touchcontrols::RectF(3, 4, 5, 6), "wings", "slide_arrow", PORT_ACT_FLY_UP, 0, PORT_ACT_FLY_DOWN, 0, hideFlySlide, "Heretic fly slider");
			flyQs->signal.connect(sigc::ptr_fun(&gameButton));
			tcGameMain->addControl(flyQs);


			tcGameMain->addControl(new touchcontrols::Button("jump", touchcontrols::RectF(24, 3, 26, 5), "jump", PORT_ACT_JUMP, false, hideJump, "Jump"));
			tcGameMain->addControl(new touchcontrols::Button("use_inventory", touchcontrols::RectF(0, 9, 2, 11), "inventory", KEY_SHOW_INV, false, hideInventory, "Show Inventory"));
			tcGameMain->addControl(new touchcontrols::Button("activate_inventory", touchcontrols::RectF(22, 3, 24, 5), "inventory_use_fade", PORT_ACT_INVUSE, false, true, "Use Inventory"));
			tcGameMain->addControl(new touchcontrols::Button("crouch", touchcontrols::RectF(24, 14, 26, 16), "crouch", PORT_ACT_DOWN, false, true, "Crouch"));
			tcGameMain->addControl(new touchcontrols::Button("crouch_toggle", touchcontrols::RectF(24, 14, 26, 16), "crouch", PORT_ACT_TOGGLE_CROUCH, false, true, "Crouch (toggle)"));
			tcGameMain->addControl(new touchcontrols::Button("attack_alt", touchcontrols::RectF(21, 5, 23, 7), "shoot_alt", PORT_ACT_ALT_ATTACK, false, true, "Alt attack"));
			tcGameMain->addControl(new touchcontrols::Button("attack_alt2", touchcontrols::RectF(4, 3, 6, 5), "shoot_alt", PORT_ACT_ALT_ATTACK, false, true, "Alt attack (duplicate)"));
			tcGameMain->addControl(new touchcontrols::Button("attack_alt_toggle", touchcontrols::RectF(21, 5, 23, 7), "shoot_alt", PORT_ACT_TOGGLE_ALT_ATTACK, false, true, "Alt attack (toggle)"));
			tcGameMain->addControl(new touchcontrols::Button("show_custom", touchcontrols::RectF(0, 7, 2, 9), "custom_show", KEY_SHOW_CUSTOM, false, true, "Show custom"));
			tcGameMain->addControl(new touchcontrols::Button("show_weapons", touchcontrols::RectF(12, 14, 14, 16), "show_weapons", KEY_SHOW_WEAPONS, false, false, "Show numbers"));
			tcGameMain->addControl(new touchcontrols::Button("next_weapon", touchcontrols::RectF(0, 3, 3, 5), "next_weap", PORT_ACT_NEXT_WEP, false, false, "Next weapon"));
			tcGameMain->addControl(new touchcontrols::Button("prev_weapon", touchcontrols::RectF(0, 5, 3, 7), "prev_weap", PORT_ACT_PREV_WEP, false, false, "Prev weapon"));
			tcGameMain->addControl(new touchcontrols::Button("console", touchcontrols::RectF(6, 0, 8, 2), "tild", PORT_ACT_CONSOLE, false, true, "Console"));

			touchcontrols::ButtonGrid *dpad = new touchcontrols::ButtonGrid("dpad_move", touchcontrols::RectF(6, 3, 12, 7), "", 3, 2, true, "Movement btns (WASD)");

			dpad->addCell(0, 1, "direction_left", PORT_ACT_MOVE_LEFT);
			dpad->addCell(2, 1, "direction_right", PORT_ACT_MOVE_RIGHT);
			dpad->addCell(1, 0, "direction_up", PORT_ACT_FWD);
			dpad->addCell(1, 1, "direction_down", PORT_ACT_BACK);
			tcGameMain->addControl(dpad);

			touchJoyRight = new touchcontrols::TouchJoy("touch", touchcontrols::RectF(17, 4, 26, 16), "look_arrow", "fixed_stick_circle");
			tcGameMain->addControl(touchJoyRight);
			touchJoyRight->signal_move.connect(sigc::ptr_fun(&right_stick));
			touchJoyRight->signal_double_tap.connect(sigc::ptr_fun(&right_double_tap));

			touchJoyLeft = new touchcontrols::TouchJoy("stick", touchcontrols::RectF(0, 7, 8, 16), "strafe_arrow", "fixed_stick_circle");
			tcGameMain->addControl(touchJoyLeft);
			touchJoyLeft->signal_move.connect(sigc::ptr_fun(&left_stick));
			touchJoyLeft->signal_double_tap.connect(sigc::ptr_fun(&left_double_tap));

#if 1
			// SWAPFIX
			touchJoyLeft->registerTouchJoySWAPFIX(touchJoyRight);
			touchJoyRight->registerTouchJoySWAPFIX(touchJoyLeft);
#endif

			tcGameMain->signal_button.connect(sigc::ptr_fun(&gameButton));
			tcGameMain->signal_settingsButton.connect(sigc::ptr_fun(&gameSettingsButton));


			UI_tc = touchcontrols::createDefaultSettingsUI(&controlsContainer, (std::string)xmlPath +  "/touch_settings.xml");
			UI_tc->setAlpha(1);

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

			tcGameWeapons->signal_button.connect(sigc::ptr_fun(&selectWeaponButton));
			tcGameWeapons->setAlpha(0.8);

			//Weapon wheel -------------------------------------------
			//------------------------------------------------------

			int weaponWheelNbr = 10;

			if(gameType == GAME_TYPE_HERETIC)
			{
				weaponWheelNbr = 8;
			}
			else if(gameType == GAME_TYPE_HEXEN)
			{
				weaponWheelNbr = 4;
			}

			wheelSelect = new touchcontrols::WheelSelect("weapon_wheel", touchcontrols::RectF(7, 2, 19, 14), "weapon_wheel_%d", weaponWheelNbr);
			wheelSelect->signal_selected.connect(sigc::ptr_fun(&weaponWheel));
			wheelSelect->signal_enabled.connect(sigc::ptr_fun(&weaponWheelSelected));
			tcWeaponWheel->addControl(wheelSelect);
			tcWeaponWheel->setAlpha(0.8);

			// Inventory -------------------------------------------
			//------------------------------------------------------
			if(gameType == GAME_TYPE_STRIFE)
			{
				uiInventoryButtonGrid = new touchcontrols::ButtonGrid("inventory_grid", touchcontrols::RectF(3, 7, 11, 11), "inventory_bg", 4, 2);

				uiInventoryButtonGrid->addCell(0, 0, "ammo", PORT_ACT_SHOW_WEAPONS);
				uiInventoryButtonGrid->addCell(1, 0, "key", PORT_ACT_SHOW_KEYS);
				uiInventoryButtonGrid->addCell(2, 0, "notebook", PORT_ACT_HELPCOMP);

				uiInventoryButtonGrid->addCell(0, 1, "inventory_left", PORT_ACT_INVPREV);
				uiInventoryButtonGrid->addCell(1, 1, "inventory_right", PORT_ACT_INVNEXT);
				uiInventoryButtonGrid->addCell(2, 1, "inventory_use", PORT_ACT_INVUSE);
				uiInventoryButtonGrid->addCell(3, 1, "inventory_drop", PORT_ACT_INVDROP);
			}
			else
			{
				uiInventoryButtonGrid = new touchcontrols::ButtonGrid("inventory_grid", touchcontrols::RectF(3, 9, 11, 11), "inventory_bg", 4, 1);

				uiInventoryButtonGrid->addCell(0, 0, "inventory_left", PORT_ACT_INVPREV);
				uiInventoryButtonGrid->addCell(1, 0, "inventory_right", PORT_ACT_INVNEXT);
				uiInventoryButtonGrid->addCell(2, 0, "inventory_use", PORT_ACT_INVUSE);
				uiInventoryButtonGrid->addCell(3, 0, "inventory_drop", PORT_ACT_INVDROP);
			}

			uiInventoryButtonGrid->signal_outside.connect(sigc::ptr_fun(&inventoryOutside));

			tcInventory->addControl(uiInventoryButtonGrid);
			tcInventory->setPassThroughTouch(touchcontrols::TouchControls::PassThrough::NO_CONTROL);
			tcInventory->signal_button.connect(sigc::ptr_fun(&inventoryButton));
			tcInventory->setAlpha(0.9);


			//Auto Map -------------------------------------------
			//------------------------------------------------------
			touchcontrols::MultitouchMouse *multimouse = new touchcontrols::MultitouchMouse("gamemouse", touchcontrols::RectF(0, 0, 26, 16), "");
			multimouse->setHideGraphics(true);
			tcAutomap->addControl(multimouse);
			multimouse->signal_action.connect(sigc::ptr_fun(&automap_multitouch_mouse_move));
			tcAutomap->addControl(new touchcontrols::Button("map", touchcontrols::RectF(2, 0, 4, 2), "map", PORT_ACT_MAP));
			tcAutomap->signal_button.connect(sigc::ptr_fun(&automapButton));


			//Blank -------------------------------------------
			//------------------------------------------------------
			tcBlank->addControl(new touchcontrols::Button("enter", touchcontrols::RectF(0, 0, 26, 16), "", 0x123));
#if defined(GZDOOM1_9) || defined(ZANDRONUM_30)
			tcBlank->addControl(new touchcontrols::Button("fmod", touchcontrols::RectF(22, 0, 26, 1), "fmod_logo", 0));
			tcBlank->setAlpha(0.8);
#endif
			tcBlank->signal_button.connect(sigc::ptr_fun(&blankButton));

			//Demo -------------------------------------------
			//------------------------------------------------------
			tcDemo->addControl(new touchcontrols::Button("pressed", touchcontrols::RectF(0, 2, 26, 16), "", 0));
			tcDemo->addControl(new touchcontrols::Button("keyboard", touchcontrols::RectF(2, 0, 4, 2), "keyboard", KEY_SHOW_KBRD));
			tcDemo->addControl(new touchcontrols::Button("speed_down", touchcontrols::RectF(20, 14, 22, 16), "key_-", PORT_ACT_DEMO_SPD_DWN));
			tcDemo->addControl(new touchcontrols::Button("speed_default", touchcontrols::RectF(22, 14, 24, 16), "key_0", PORT_ACT_DEMO_SPD_DEF));
			tcDemo->addControl(new touchcontrols::Button("speed_up", touchcontrols::RectF(24, 14, 26, 16), "key_+", PORT_ACT_DEMO_SPD_UP));
			tcDemo->addControl(new touchcontrols::Button("camera", touchcontrols::RectF(0, 14, 2, 16), "camera", PORT_ACT_DEMO_CAMERA));
			tcDemo->signal_button.connect(sigc::ptr_fun(&menuButton));
			tcDemo->setAlpha(1);

			//Keyboard -------------------------------------------
			//------------------------------------------------------
			uiKeyboard = new touchcontrols::UI_Keyboard("keyboard", touchcontrols::RectF(0, 8, 26, 16), "font_dual", 0, 0, 0);
			uiKeyboard->signal.connect(sigc::ptr_fun(&keyboardKeyPressed));
			tcKeyboard->addControl(uiKeyboard);
			// We want touch to pass through only where there is no keyboard
			tcKeyboard->setPassThroughTouch(touchcontrols::TouchControls::PassThrough::NO_CONTROL);

			//Yes No -------------------------------------------
			//------------------------------------------------------
			tcYesNo->addControl(new touchcontrols::Button("yes", touchcontrols::RectF(8, 12, 11, 15), "key_y", PORT_ACT_MENU_CONFIRM));
			tcYesNo->addControl(new touchcontrols::Button("no", touchcontrols::RectF(15, 12, 18, 15), "key_n", PORT_ACT_MENU_ABORT));
			tcYesNo->signal_button.connect(sigc::ptr_fun(&menuButton));
			tcYesNo->setAlpha(0.8);

			//Custom Controls -------------------------------------------
			//------------------------------------------------------
			createCustomControls(tcCustomButtons);

			//Gamepad utility -------------------------------------------
			//------------------------------------------------------
			touchcontrols::ButtonGrid *gamepadUtils = new touchcontrols::ButtonGrid("gamepad_grid", touchcontrols::RectF(8, 5, 18, 11), "gamepad_utils_bg", 3, 2);

			gamepadUtils->addCell(0, 0, "back_button", KEY_BACK_BUTTON);
			gamepadUtils->addCell(0, 1, "map", PORT_ACT_MAP);
			gamepadUtils->addCell(1, 0, "keyboard", KEY_SHOW_KBRD);
			gamepadUtils->addCell(1, 1, "tild", PORT_ACT_CONSOLE);
			gamepadUtils->addCell(2, 0, "save", PORT_ACT_QUICKSAVE);
			gamepadUtils->addCell(2, 1, "load", PORT_ACT_QUICKLOAD);

			gamepadUtils->signal_outside.connect(sigc::ptr_fun(&gameUtilitiesOutside));

			tcGamepadUtility->addControl(gamepadUtils);
			tcGamepadUtility->setAlpha(0.9);
			tcGamepadUtility->signal_button.connect(sigc::ptr_fun(&gameUtilitiesButton));

			// DPad inventory -------------------------------------------
			//------------------------------------------------------
			touchcontrols::DPadSelect *dpadInventory = new touchcontrols::DPadSelect("dpad_inventory", touchcontrols::RectF(9, 4, 17, 12), "dpad", PORT_ACT_INVUSE);

			dpadInventory->addCell(touchcontrols::DPAD_LEFT, "inventory_left", PORT_ACT_INVPREV);
			dpadInventory->addCell(touchcontrols::DPAD_RIGHT, "inventory_right", PORT_ACT_INVNEXT);
			dpadInventory->addCell(touchcontrols::DPAD_DOWN, "inventory_drop", PORT_ACT_INVDROP);
			//dpadInventory->addCell(touchcontrols::DPAD_LEFT,"inventory_left",PORT_ACT_INVPREV);
			//
			dpadInventory->signal_button.connect(sigc::ptr_fun(&dPadInventoryButton));
			dpadInventory->signal_outside.connect(sigc::ptr_fun(&dPadInventoryOutside));

			tcDPadInventory->addControl(dpadInventory);
			tcDPadInventory->setAlpha(0.9);

			// Mouse for GZDoom
			touchcontrols::Mouse *mouse = new touchcontrols::Mouse("mouse", touchcontrols::RectF(0, 2, 26, 16), "");
			mouse->setHideGraphics(true);
			mouse->setEditable(false);
			tcMouse->addControl(mouse);
			mouse->signal_action.connect(sigc::ptr_fun(&mouse_move));
			tcMouse->addControl(new touchcontrols::Button("back", touchcontrols::RectF(0, 0, 2, 2), "back_button", KEY_BACK_BUTTON, false, false, "Back"));
			// Hide the mouse button because we can now use a tap
			//tcMouse->addControl(new touchcontrols::Button("left_button", touchcontrols::RectF(0, 6, 3, 10), "mouse2", KEY_LEFT_MOUSE, false, false, "Back"));
			tcMouse->addControl(new touchcontrols::Button("hide_mouse", touchcontrols::RectF(4, 0, 6, 2), "mouse2", KEY_USE_MOUSE, false, false, "Hide mouse"));
			tcMouse->setAlpha(0.9);
			tcMouse->signal_button.connect(sigc::ptr_fun(&mouseButton));

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
			controlsContainer.addControlGroup(tcAutomap);
			controlsContainer.addControlGroup(tcBlank);
			controlsContainer.addControlGroup(tcDemo);
			controlsContainer.addControlGroup(tcMouse);

			controlsCreated = 1;

			// Doom type stays the same so existing user keep original config
			if(gameType == GAME_TYPE_HEXEN)
				touchcontrols::setGlobalXmlAppend(".hexen");
			else if(gameType == GAME_TYPE_HERETIC)
				touchcontrols::setGlobalXmlAppend(".heretic");
			else if(gameType == GAME_TYPE_STRIFE)
				touchcontrols::setGlobalXmlAppend(".strife");

			tcMenuMain->setXMLFile((std::string)xmlPath +  "/menu.xml");

			tcGameMain->setXMLFile((std::string)xmlPath +  "/game_" ENGINE_NAME ".xml");
			tcInventory->setXMLFile((std::string)xmlPath +  "/inventory_" ENGINE_NAME ".xml");
			tcWeaponWheel->setXMLFile((std::string)xmlPath +  "/weaponwheel_" ENGINE_NAME ".xml");
			tcGameWeapons->setXMLFile((std::string)xmlPath +  "/weapons_" ENGINE_NAME ".xml");
			tcCustomButtons->setXMLFile((std::string)xmlPath +  "/custom_buttons_0_" ENGINE_NAME ".xml");
		}
		else
			LOGI("NOT creating controls");

		SDL_SetSwapBufferCallBack(frameControls);

		SDL_SetShowKeyboardCallBack(showHideKeyboard);
	}




	void mobile_init(int width, int height, const char *pngPath, int options, int wheelNbr, int game)
	{
		if(options & GAME_OPTION_AUTO_HIDE_GAMEPAD)
			gamepadHideTouch = true;

		if(options & GAME_OPTION_HIDE_MENU_AND_GAME)
			hideGameAndMenu = true;

		if(options & GAME_OPTION_USE_SYSTEM_KEYBOARD)
			useSystemKeyboard = true;

		if(options & GAME_OPTION_GLES2)
		{
			touchcontrols::gl_setGLESVersion(2);
#ifndef D3ES
			touchcontrols::gl_useGL4ES(); // GLES2 always uses GL4ES library
#endif
		}

		if(options & GAME_OPTION_GLES3)
		{
			touchcontrols::gl_setGLESVersion(3);
		}


		gameType = game;

		LOGI("Game type = %d", gameType);

#ifdef __ANDROID__
		mobile_screen_width = width;
		mobile_screen_height = height;
		graphicpath = pngPath;
		LOGI("PNG Path = %s\n", graphicpath.c_str());

		putenv((char*)"TIMIDITY_CFG=./audiopack/snd_timidity/timidity.cfg");

		touchcontrols::GLScaleWidth = (float)mobile_screen_width;
		touchcontrols::GLScaleHeight = (float) - mobile_screen_height;
		touchcontrols::signal_vibrate.connect(sigc::ptr_fun(&vibrate));
		touchcontrols::gl_setGraphicsBasePath(graphicpath.c_str());
		touchcontrols::getSettingsSignal()->connect(sigc::ptr_fun(&touchSettings));

		controlsContainer.openGL_start.connect(sigc::ptr_fun(&openGLStart));
		controlsContainer.openGL_end.connect(sigc::ptr_fun(&openGLEnd));


		SDL_ShowMouseCallBack(showSDLMouseCallback);
		SDL_MouseMoveCallBack(moveSDLMouseCallback);

#ifdef D3ES
		initControlsDoom3(graphicpath.c_str());
#else
		initControls(graphicpath.c_str());
#endif


#endif


#ifdef __IOS__
		mobile_screen_width = 1152;
		mobile_screen_height = 640;

		static std::string pngPath = getPngDirectory();
		//pngPath += "/../png/";
		LOGI("PNG Path = %s\n", pngPath.c_str());

		initControls(mobile_screen_width, -mobile_screen_height, pngPath.c_str());

		SDL_SetTouchControlsInterface(&controlsContainer);
#endif
	}

	void mobileBackButton(void)
	{
		LOGI("mobileBackButton");

		if(tcKeyboard->isEnabled())
		{
			SDL_StopTextInput();
		}
		else if(tcInventory && tcInventory->isEnabled())
		{
			tcInventory->animateOut(5);
		}
		else if(tcGamepadUtility && tcGamepadUtility->isEnabled())
		{
			tcGamepadUtility->animateOut(5);
		}
		else if(tcDPadInventory && tcDPadInventory->isEnabled())
		{
			tcDPadInventory->animateOut(5);
		}
		else if(controlsContainer.isEditing())
		{
			controlsContainer.finishEditing();
			return;
		}
		else if(wheelSelect->blockGamepad())
		{
			wheelSelect->reset();
			return;
		}
		else if(showCustomMenu == true)
		{
			showCustomMenu = false;
		}
		else
		{
			PortableBackButton();
		}
	}

	static void hideControls(void)
	{
		if(tcGameMain)
		{
			if(tcGameMain->isEnabled())
				tcGameMain->animateOut(30);

			// Actually leave this enabled for the gamepad to use
			// tcWeaponWheel->animateOut(1);
		}
	}

	void gamepadAction(int state, int action)
	{
		LOGI("gamepadAction, %d  %d", state, action);
		bool used = false;

		switch(action)
		{
		case PORT_ACT_MENU_UP:
			used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::UP);
			break;

		case PORT_ACT_MENU_DOWN:
			used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::DOWN);
			break;

		case PORT_ACT_MENU_LEFT:
			used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::LEFT);
			break;

		case PORT_ACT_MENU_RIGHT:
			used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::RIGHT);
			break;

		case PORT_ACT_MENU_SELECT:
			used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::SELECT);
			break;

		case PORT_ACT_MENU_BACK:
			used = controlsContainer.gamepadInput(state, touchcontrols::GamePadKey::BACK);
			break;
		}

		if(used)
			return;

		if(tcKeyboard->isEnabled())
		{
			// Do nothing for now, stops multiple controls opening
		}
		else if(tcInventory && tcInventory->isEnabled())
		{
			// Do nothing for now, stops multiple controls opening
		}
		else if(tcGamepadUtility && tcGamepadUtility->isEnabled())
		{
			// Do nothing for now, stops multiple controls opening
		}
		else if(tcDPadInventory && tcDPadInventory->isEnabled())
		{
			// Do nothing for now, stops multiple controls opening
		}
		else if(action == PORT_ACT_SHOW_KBRD)
		{
			if(state)
			{
				SDL_StartTextInput();
			}
		}
		else if(action == PORT_ACT_SHOW_GP_UTILS)
		{
			if(state)
			{
				if(!tcGamepadUtility->isEnabled())
				{
					tcGamepadUtility->animateIn(5);
				}
			}
		}
		else if(tcDPadInventory && action == PORT_ACT_SHOW_DPAD_INV  && currentScreenMode == TS_GAME)
		{
			if(state)
			{
				if(!tcDPadInventory->isEnabled())
				{
					//tcDPadInventory->animateIn(5);
					tcDPadInventory->setEnabled(true);
				}
			}
		}
		else if(tcInventory && action == PORT_ACT_SHOW_INV && currentScreenMode == TS_GAME)
		{
			if(state)
			{
				if(!tcInventory->isEnabled())
				{
					tcInventory->animateIn(5);
				}
			}
		}
		else if(action == PORT_ACT_USE_WEAPON_WHEEL && currentScreenMode == TS_GAME)
		{
			wheelSelect->setGamepadMode(weaponWheelGamepadMode, weaponWheelAutoTimout);
			wheelSelect->gamepadActionButton(state);
		}
		else if(action == PORT_ACT_MENU_SHOW)
		{
			if(state)
			{
				mobileBackButton();
			}
		}
		else
		{
			PortableAction(state, action);

			if(gamepadHideTouch)
				hideControls();
		}
	}


	void axisValue(int axis, float value)
	{
		static float x = 0;
		static float y = 0;

		if(weaponWheelMoveStick)
		{
			if(axis == ANALOGUE_AXIS_SIDE)
				x = value;
			else if(axis == ANALOGUE_AXIS_FWD)
				y = value;
		}
		else
		{
			if(axis == ANALOGUE_AXIS_YAW)
				x = value;
			else if(axis == ANALOGUE_AXIS_PITCH)
				y = value;
		}

		if(wheelSelect)
			wheelSelect->processGamepad(x, y);   // return true is axis value was used
	}

	int blockGamepad(void)
	{
		if(wheelSelect)
		{
			if(wheelSelect->blockGamepad())
			{
				if(weaponWheelMoveStick)
					return ANALOGUE_AXIS_SIDE | ANALOGUE_AXIS_FWD;
				else
					return ANALOGUE_AXIS_YAW | ANALOGUE_AXIS_PITCH;
			}
			else
				return 0;
		}
		else
			return 0;
	}

	void weaponWheelSettings(bool useMoveStick, int mode, int autoTimeout)
	{
		weaponWheelMoveStick = useMoveStick;
		weaponWheelGamepadMode = (touchcontrols::WheelSelectMode)mode;
		weaponWheelAutoTimout = autoTimeout;
	}

	int volumeKey(int state, bool volumeUp)
	{
		if(currentScreenMode == TS_GAME) // Allow real volume to change when not in game
		{
			if(volumeUp)
			{
				if(volume_up_action)
				{
					PortableAction(state, volume_up_action);
					return 1;
				}
			}
			else
			{
				if(volume_down_action)
				{
					PortableAction(state, volume_down_action);
					return 1;
				}
			}
		}

		return 0;
	}

	TouchControlsInterface* mobileGetTouchInterface()
	{
		return &controlsContainer;
	}


	bool saveControlSettings(std::string path)
	{
		std::string settings = path + "/settings.xml";
		touchcontrols::touchSettings_save(settings);

		tcGameMain->saveXML(path + "/tcGameMain.xml");


		tcWeaponWheel->saveXML(path + "/tcWeaponWheel.xml");

		tcGameWeapons->saveXML(path + "/tcGameWeapons.xml");

		if(tcInventory)
			tcInventory->saveXML(path + "/tcInventory.xml");

		if(tcCustomButtons)
			tcCustomButtons->saveXML(path + "/tcCustomButtons.xml");

		return false;
	}

	bool loadControlSettings(std::string path)
	{
		std::string settings = path + "/settings.xml";
		touchcontrols::touchSettings_load(settings);
		touchcontrols::touchSettings_save(); // Save over current settings

		tcGameMain->loadXML(path + "/tcGameMain.xml");
		tcGameMain->save(); // Save the newly loaded

		tcWeaponWheel->loadXML(path + "/tcWeaponWheel.xml");
		tcWeaponWheel->save();

		tcGameWeapons->loadXML(path + "/tcGameWeapons.xml");
		tcGameWeapons->save();

		if(tcInventory)
		{
			tcInventory->loadXML(path + "/tcInventory.xml");
			tcInventory->save();
		}

		if(tcCustomButtons)
		{
			tcCustomButtons->loadXML(path + "/tcCustomButtons.xml");
			tcCustomButtons->save();
		}

		return false;
	}
}