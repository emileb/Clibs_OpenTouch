//
// Created by emile on 01/01/2021.
//

#ifndef OPENTOUCH_TOUCH_INTERFACE_BASE_H
#define OPENTOUCH_TOUCH_INTERFACE_BASE_H


#include "TouchControlsInterface.h"
#include "TouchControlsContainer.h"
#include "SDL_beloko_extra.h"
#include "SDL.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "UI_TouchDefaultSettings.h"
#include "UI_ButtonListWindow.h"

#include "touch_interface.h"
#include "game_interface.h"
#include "SDL_keycode.h"

#ifdef CHOC_SETUP
#define DEFAULT_FADE_FRAMES 0
#else
#define DEFAULT_FADE_FRAMES 10
#endif

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

extern "C"
{
	int Android_JNI_SendMessage(int command, int param);
	int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
	int SDL_SendKeyboardText(const char *text);

	static void setupStatic(void);
	static void frameControlsSDLCallback(void);
	static void showKeyboardCallbackSDLCallback(int show);
	static void showMouseSDLCallback(int show);
	static void moveMouseSDLCallback(float x, float y);
}



class TouchInterfaceBase {

public:

	int gameType = 0;
	int nativeWidth = 0;
	int nativeHeight = 0;

	bool gamepadHideTouch = false;
	bool hideGameAndMenu = false;
	bool useSystemKeyboard = false;

	bool weaponWheelMoveStick = true;
	touchcontrols::WheelSelectMode weaponWheelGamepadMode = touchcontrols::WHEELSELECT_GP_MODE_HOLD;
	int weaponWheelAutoTimout = 0;

	int leftDoubleAction = 0;
	int rightDoubleAction = 0;
	int volumeUpAction = 0;
	int volumeDownAction = 0;
	
	touchscreemode_t currentScreenMode = TS_BLANK;
	touchcontrols::tTouchSettings touchSettings;
	bool useMouse = false;
	bool gameShowMouse = false;
	bool isShooting = false;
	bool showWeaponNumbersOn = false;
	bool showCustomOn = false;
	bool showCustomMenu = false;

	touchcontrols::TouchControlsContainer controlsContainer;


	touchcontrols::UI_Controls *UI_tc = 0;

	touchcontrols::TouchControls *tcMenuMain = 0;
	touchcontrols::TouchControls *tcYesNo = 0;
	touchcontrols::TouchControls *tcGameMain = 0;
	touchcontrols::TouchControls *tcGameWeapons = 0;
	touchcontrols::TouchControls *tcWeaponWheel = 0;
	touchcontrols::TouchControls *tcInventory = 0;
	touchcontrols::TouchControls *tcAutomap = 0;
	touchcontrols::TouchControls *tcBlank = 0;
	touchcontrols::TouchControls *tcKeyboard = 0;
	touchcontrols::TouchControls *tcCustomButtons = 0;
	touchcontrols::TouchControls *tcDemo = 0;
	touchcontrols::TouchControls *tcGamepadUtility = 0;
	touchcontrols::TouchControls *tcDPadInventory = 0;
	touchcontrols::TouchControls *tcMouse = 0;

#ifdef D3ES
	static touchcontrols::TouchControls *tcPda = 0;
#endif

	// So can hide and show these buttons
	touchcontrols::TouchJoy *touchJoyLeft;
	touchcontrols::TouchJoy *touchJoyRight;

	// So gampad can control keyboard
	touchcontrols::UI_Keyboard *uiKeyboard;

	// So gamepad can access it
	touchcontrols::WheelSelect *wheelSelect;
	touchcontrols::ButtonGrid *uiInventoryButtonGrid;

	virtual void createControls(std::string filesPath) = 0;

	virtual void openGLEnd() = 0;
	virtual void openGLStart() = 0;
	virtual void blankButton(int state, int code) = 0;

	virtual void newFrame() = 0;

	void init(int width, int height, const char *pngPath, int options, int wheelNbr, int game)
	{
		nativeWidth = width;
		nativeHeight = height;

		gameType = game;

		if(options & GAME_OPTION_AUTO_HIDE_GAMEPAD)
			gamepadHideTouch = true;

		if(options & GAME_OPTION_HIDE_MENU_AND_GAME)
			hideGameAndMenu = true;

		if(options & GAME_OPTION_USE_SYSTEM_KEYBOARD)
			useSystemKeyboard = true;

		if(options & GAME_OPTION_GLES2)
			touchcontrols::gl_setGLESVersion(2);


		if(options & GAME_OPTION_GLES3)
			touchcontrols::gl_setGLESVersion(3);


		touchcontrols::GLScaleWidth = (float)nativeWidth;
		touchcontrols::GLScaleHeight = (float)-nativeHeight;
		touchcontrols::signal_vibrate.connect(sigc::mem_fun(this, &TouchInterfaceBase::vibrate));
		touchcontrols::gl_setGraphicsBasePath(pngPath);
		touchcontrols::getSettingsSignal()->connect(sigc::mem_fun(this, &TouchInterfaceBase::touchSettingsCallback));

		controlsContainer.openGL_start.connect(sigc::mem_fun(this, &TouchInterfaceBase::openGLStart));
		controlsContainer.openGL_end.connect(sigc::mem_fun(this, &TouchInterfaceBase::openGLEnd));

		SDL_SetSwapBufferCallBack(frameControlsSDLCallback);
		SDL_ShowMouseCallBack(showMouseSDLCallback);
		SDL_MouseMoveCallBack(moveMouseSDLCallback);
		SDL_SetShowKeyboardCallBack(showKeyboardCallbackSDLCallback);

		// Call app specific control creation
		createControls(pngPath);

		// Always create touch settings control
		UI_tc = touchcontrols::createDefaultSettingsUI(&controlsContainer, (std::string)pngPath +  "/touch_settings.xml");
		UI_tc->setAlpha(1);
	}


	int touchActionToAction(int action)
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

	void touchSettingsCallback(touchcontrols::tTouchSettings settings)
	{
		touchSettings = settings;

		leftDoubleAction = touchActionToAction(touchSettings.dblTapLeft);
		rightDoubleAction = touchActionToAction(touchSettings.dblTapRight);

		volumeUpAction = touchActionToAction(touchSettings.volumeUp);
		volumeDownAction = touchActionToAction(touchSettings.volumeDown);

		controlsContainer.setColour(touchSettings.defaultColor);
		controlsContainer.setAlpha(touchSettings.alpha);

		touchJoyLeft->setCenterAnchor(touchSettings.fixedMoveStick);

		if(tcGameMain) tcGameMain->setAlpha(touchSettings.alpha);
		if(tcCustomButtons)  tcCustomButtons->setAlpha(touchSettings.alpha);

		if(tcYesNo) tcYesNo->setColour(touchSettings.defaultColor);
		if(tcGameMain) tcGameMain->setColour(touchSettings.defaultColor);
		if(tcGameWeapons) tcGameWeapons->setColour(touchSettings.defaultColor);
		if(tcWeaponWheel) tcWeaponWheel->setColour(touchSettings.defaultColor);
		if(tcInventory) tcInventory->setColour(touchSettings.defaultColor);
		if(tcAutomap) tcAutomap->setColour(touchSettings.defaultColor);
		if(tcCustomButtons) tcCustomButtons->setColour(touchSettings.defaultColor);
		if(tcGamepadUtility) tcGamepadUtility->setColour(touchSettings.defaultColor);
		if(tcDPadInventory) tcDPadInventory->setColour(touchSettings.defaultColor);
	}

	void gameButton(int state, int code)
	{
#ifndef NO_SEC
		if( state )
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
			isShooting = state;
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
				//mapState = 0; TODO FIX FOR DELTA
			}

			PortableAction(state, code);
		}
		else
		{
			PortableAction(state, code);
		}
	}

	///////////////////////////////////////////
	///////////     MENU BUTTONS    ///////////
	///////////////////////////////////////////
	void menuButton(int state, int code)
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
			PortableMouseButton(state, 1, 0, 0);
		}
		else
		{
			PortableAction(state, code);
		}

		//demoControlsAlpha = DEMO_ALPHA_RESET; TODO
	}

	void leftStick(float joy_x, float joy_y, float mouse_x, float mouse_y)
	{
		float fwdback = joy_y * 15 * touchSettings.fwdSensitivity;
		float strafe  = -joy_x * 10 * touchSettings.strafeSensitivity;

		PortableMove(fwdback, strafe);
	}

	void rightStick(float joy_x, float joy_y, float mouse_x, float mouse_y)
	{
		//LOGI(" mouse x = %f",mouse_x);
		int invert        = touchSettings.invertLook ? -1 : 1;
		float scale       = (isShooting && touchSettings.precisionShoot) ? touchSettings.precisionSenitivity : 1;
		float pitchMouse       = mouse_y * touchSettings.lookSensitivity * invert * scale;
		float pitchJoystick    = joy_y * touchSettings.lookSensitivity * invert * scale * -2;

		float yawMouse    = mouse_x * touchSettings.turnSensitivity * scale;
		float yawJoystick = joy_x  * touchSettings.turnSensitivity * scale * 10;

		if(touchSettings.joystickLookMode)
			PortableLookPitch(LOOK_MODE_JOYSTICK, pitchJoystick);
		else
			PortableLookPitch(LOOK_MODE_MOUSE, pitchMouse);

		if(touchSettings.joystickLookMode)
			PortableLookYaw(LOOK_MODE_JOYSTICK, yawJoystick);
		else
			PortableLookYaw(LOOK_MODE_MOUSE, yawMouse);
	}

	void inventoryButton(int state, int code)
	{
		PortableAction(state, code);
	}

	void inventoryOutside(bool fromGamepad)
	{
		if(touchSettings.autoHideInventory || fromGamepad)
		{
			if(tcInventory->enabled)
				tcInventory->animateOut(5);
		}
	}

	void gameUtilitiesButton(int state, int code)
	{
		// Auto hide the gamepad utilitie, except if showing consol
		if((code != PORT_ACT_CONSOLE) && (tcGamepadUtility->isEnabled()))
		{
			tcGamepadUtility->setEnabled(false);
		}

		gameButton(state, code);
	}

	void gameUtilitiesOutside(bool fromGamepad)
	{
		if(tcGamepadUtility->isEnabled())
		{
			tcGamepadUtility->setEnabled(false);
		}
	}

	 void customButton(int state, int code)
	{
		LOGI("Cust %d, %d", state, code);
		PortableAction(state, code);

		// If we are binding keys hide the custom buttons again
		if(state == 1)
			showCustomMenu = false;
	}

	//Weapon wheel callbacks
	void weaponWheelSelected(int enabled)
	{
		if(enabled)
			tcWeaponWheel->fade(touchcontrols::FADE_IN, 5); //fade in
	}

	void weaponWheel(int segment)
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

	void selectWeaponButton(int state, int code)
	{
		PortableAction(state, PORT_ACT_WEAP0 + code);

		if(touchSettings.autoHideNumbers && state == 0)
		{
			showWeaponNumbersOn = false;
			tcGameWeapons->animateOut(5);
		}
	}

	void mouse_move(int action, float x, float y, float mouse_x, float mouse_y)
	{
#if defined(GZDOOM) || defined(ZANDRONUM_30) || defined(D3ES) // todo

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

	void mouseButton(int state, int code)
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
	void automap_multitouch_mouse_move(int action, float x, float y, float dx, float dy)
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


	void brightnessSlideMouse(int action, float x, float y, float dx, float dy)
	{
		y = 1 - y;
		Android_JNI_SendMessage(COMMAND_SET_BACKLIGHT, y * 255);
	}

	void dPadInventoryButton(int state, int code)
	{
		PortableAction(state, code);
	}

	void dPadInventoryOutside(bool fromGamepad)
	{
		if(tcDPadInventory->enabled)
			tcDPadInventory->animateOut(5);
	}

	void leftDoubleTap(int state)
	{
		if(leftDoubleAction)
			PortableAction(state, leftDoubleAction);
	}

	void rightDoubleTap(int state)
	{
		if(rightDoubleAction)
			PortableAction(state, rightDoubleAction);
	}

	void vibrate(int duration)
	{
		Android_JNI_SendMessage(COMMAND_VIBRATE, duration);
	}

	void gameSettingsButton(int state)
	{
		if(state == 1)
		{
			controlsContainer.showUIWindow(UI_tc);
		}
	}

	void customSettingsButton(int state)
	{
		if(state == 1)
		{
			showButtonListWindow(&controlsContainer);
			//showEditButtons();
		}
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

	void updateTouchScreenModeOut(touchscreemode_t mode)
	{
		if(mode != currentScreenMode)
		{
			//first disable the last screen and fade out if necessary
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
		}
	}

	void updateTouchScreenModeIn(touchscreemode_t mode)
	{
		if(mode != currentScreenMode)
		{
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
				if(tcGameMain) tcGameMain->setAlpha(touchSettings.alpha);
				if(tcCustomButtons) tcCustomButtons->setAlpha(touchSettings.alpha);

				if(!hideGameAndMenu)
				{
					tcGameMain->setEnabled(true);
					tcGameMain->fade(touchcontrols::FADE_IN, DEFAULT_FADE_FRAMES);

					if(showCustomOn || touchSettings.alwaysShowCust)   // Also remember if custom buttons were shown
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
				if(touchSettings.weaponWheelEnabled)
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
		}
	}



    void createCustomControls( touchcontrols::TouchControls* customControls)
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

		qs1->signal.connect(sigc::mem_fun(this, &TouchInterfaceBase::customButton));
		qs2->signal.connect(sigc::mem_fun(this, &TouchInterfaceBase::customButton));
		qs3->signal.connect(sigc::mem_fun(this, &TouchInterfaceBase::customButton));
		qs4->signal.connect(sigc::mem_fun(this, &TouchInterfaceBase::customButton));
		customControls->signal_button.connect(sigc::mem_fun(this, &TouchInterfaceBase::customButton));
		customControls->signal_settingsButton.connect(sigc::mem_fun(this, &TouchInterfaceBase::customSettingsButton));
		customControls->setAlpha(touchSettings.alpha);
	}


	void keyboardKeyPressed(uint32_t key)
	{
		LOGI("Keyboard press: %d , %c", key, (char)key);

		if(key == UI_KEYBOARD_HIDE)
		{
			SDL_StopTextInput();
			return;
		}

		char text[2] = {0,0};

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

	void hideControls(void)
	{
		if(tcGameMain)
			if(tcGameMain->isEnabled())
				tcGameMain->animateOut(30);
	}

	// SDL callbacks
	public:

	void processPointer(int action, int pid, float x, float y)
	{
		controlsContainer.processPointer(action, pid, x, y);
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
				if(volumeUpAction)
				{
					PortableAction(state, volumeUpAction);
					return 1;
				}
			}
			else
			{
				if(volumeDownAction)
				{
					PortableAction(state, volumeDownAction);
					return 1;
				}
			}
		}
		return 0;
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

	void frameControls()
	{
		if(SDL_NewEGLCreated())
		{
			LOGI("NEW EGL CONTEXT");
			touchcontrols::clearGlTexCache();
			controlsContainer.initGL();
		}

		touchscreemode_t screenMode = PortableGetScreenMode();

		// Hack to show custom buttons while in the menu to bind keys
		if(screenMode == TS_MENU && showCustomMenu == true)
		{
			screenMode = TS_CUSTOM;
		}

/*
		if((screenMode == TS_MAP) && (mapState == 1)) TODO fix for delta
		{
			screenMode =  TS_GAME;
		}

		if(screenMode == TS_DEMO)   // Fade out demo buttons if not touched for a while
		{
			if(demoControlsAlpha > 0)
			{
				demoControlsAlpha -= DEMO_ALPFA_DEC;
			}

			tcDemo->setAlpha(demoControlsAlpha);
		}
*/
		if(((screenMode == TS_GAME) || (screenMode == TS_MENU)) & useMouse)   // Show mouse screen
		{
			screenMode = TS_MOUSE;
		}

		if(gameShowMouse && useMouse)
			controlsContainer.showMouse(true);
		else
			controlsContainer.showMouse(false);

		if(touchJoyLeft) touchJoyLeft->setHideGraphics(touchSettings.showJoysticks);
		if(touchJoyRight) touchJoyRight->setHideGraphics(touchSettings.showJoysticks);

		updateTouchScreenModeOut(screenMode);
		updateTouchScreenModeIn(screenMode);

		currentScreenMode = screenMode;

		controlsContainer.draw();
	}


	void showKeyboardCallback(int show)
	{
		LOGI("showKeyboardCallback %d", show);

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

	void showMouseCallback(int show)
	{
		LOGI("showMouseCallback = %d", show);
		gameShowMouse = show;
	}

	void moveMouseCallback(float x, float y)
	{
		LOGI("moveMouseCallback = %f, %f", x, y);
		controlsContainer.mousePos(x, y);
	}
};

static TouchInterfaceBase *staticTouchInterface;

static void setupStatic(TouchInterfaceBase * ti)
{
	staticTouchInterface = ti;
}

static void frameControlsSDLCallback(void)
{
	staticTouchInterface->frameControls();
}

static void showKeyboardCallbackSDLCallback(int show)
{
	staticTouchInterface->showKeyboardCallback(show);
}

static void showMouseSDLCallback(int show)
{
	staticTouchInterface->showMouseCallback(show);
}

static void moveMouseSDLCallback(float x, float y)
{
	staticTouchInterface->moveMouseCallback(x, y);
}

class TouchInterface : public TouchInterfaceBase
{
public:
	void createControls(std::string filesPath);

	void openGLEnd();

	void openGLStart();

	void blankButton(int state, int code);

	void newFrame();

	void automapButton(int state, int code);
};

#endif //OPENTOUCH_TOUCH_INTERFACE_BASE_H
