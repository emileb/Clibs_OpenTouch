//
//  touch_interface.h
//  D-Touch
//
//  Created by Emile Belanger on 17/01/2016.
//  Copyright © 2016 Beloko Games. All rights reserved.
//

#ifndef touch_interface_h
#define touch_interface_h


#define NO_SEC

extern "C"
{
	void mobile_init(int width, int height, const char *pngPath, int options, int game);

	TouchControlsInterface* mobileGetTouchInterface();

	void mobileBackButton(void);
	void gamepadAction(int state, int action);
	void axisValue(int axis, float value);
	void weaponWheelSettings(bool useMoveStick, int mode, int autoTimeout);
	int blockGamepad(void);
}

#endif /* touch_interface_h */
