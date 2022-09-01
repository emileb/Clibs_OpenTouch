#include <jni.h>
#include "TouchControlsInterface.h"
#include "JNITouchControlsUtils.h"
#include "SDL_beloko_extra.h"

#include "TouchControlsInterface.h"
#include "touch_interface.h"

#include "game_interface.h"

#include "LogWritter.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Framebuffer.h"

#ifndef NO_SEC
#include "./secure/license/license.h"
#endif

#include <time.h>

#define COMMAND_EXIT_APP  0x8007

#pragma clang diagnostic ignored "-Winfinite-recursion"

extern "C"
{
	int gameType;
	int mobile_screen_width;
	int mobile_screen_height;

	static TouchInterface touchInterface;

	JNIEnv* env_;

	int Android_JNI_SendMessage(int command, int param);

	int abc = 1;
	void exit(int status)
	{
		LogWritter_Close();
		usleep(100); // Wait 100ms

		LOGI("EXIT OVERRIDE!!! (%d)", status);
		Android_JNI_SendMessage(COMMAND_EXIT_APP, 0);

		usleep(1000 * 1000 * 5); // Wait 5 seconds

		if(abc) // To stop compiler warning
			exit(0); // This should never happen because the SDLActivity should have killed the process already
		else
		{
			while(1);
		}
	}

	// Latest version of Clang makes an optimisation which calls this function which is not present in Android, so define it here
	char *stpcpy(char *__restrict__ dest, const char *__restrict__ src);
	char *stpcpy(char *__restrict__ dest, const char *__restrict__ src)
	{
		while ((*dest++ = *src++) != '\0')
			/* nothing */;
		return --dest;
	}
#define JAVA_FUNC(x) Java_org_libsdl_app_NativeLib_##x

#define EXPORT_ME __attribute__ ((visibility("default")))

	__attribute__((visibility("default"))) jint JNI_OnLoad(JavaVM* vm, void* reserved)
	{
		LOGI("JNI_OnLoad");
		setTCJNIEnv(vm);
		return JNI_VERSION_1_4;
	}

	static int argc = 1;
	static const char * argv[500];

	const char *nativeLibsPath;
	const char *sourceFilePath_c;
	std::string filesPath;
	std::string userFilesPath;
	std::string tempFilesPath;
	std::string sourceFilePath;

	char keyGlobal[512];
	char pkgGlobal[64];

	jint EXPORT_ME
	JAVA_FUNC(init)(JNIEnv* env, jobject thiz, jstring graphics_dir, jint options, jint wheelNbr, jobjectArray argsArray, jint game, jstring game_path_, jstring logFilename, jstring nativeLibs, jstring userFiles, jstring tempFiles, jstring sourceFiles)
	{
		env_ = env;
		gameType = game;

		static std::string game_path = (char *)(env)->GetStringUTFChars(game_path_, 0);

		static std::string log_filename_path = (char *)(env)->GetStringUTFChars(logFilename, 0);
		static std::string native_libs_path = (char *)(env)->GetStringUTFChars(nativeLibs, 0);
		filesPath = (char *)(env)->GetStringUTFChars(graphics_dir, 0);
		userFilesPath = (char *)(env)->GetStringUTFChars(userFiles, 0);
		tempFilesPath = (char *)(env)->GetStringUTFChars(tempFiles, 0);
		sourceFilePath = (char *)(env)->GetStringUTFChars(sourceFiles, 0);
		sourceFilePath_c = sourceFilePath.c_str();
		nativeLibsPath = native_libs_path.c_str();

		LogWritter_Init(log_filename_path.c_str());

		argv[0] = "game";
		int argCount = (env)->GetArrayLength(argsArray);
		LOGI("argCount = %d", argCount);

		for(int i = 0; i < argCount; i++)
		{
			jstring string = (jstring)(env)->GetObjectArrayElement(argsArray, i);
			argv[argc] = (char *)(env)->GetStringUTFChars(string, 0);
			LOGI("arg = %s", argv[argc]);
			LogWritter_Write(argv[argc]);
			argc++;
		}

		LogWritter_Write("\n");

		LOGI("game_path = %s", game_path.c_str());

		setenv("TIMIDITY_CFG", "./audiopack/snd_timidity/timidity.cfg", 1);

		setenv("HOME", game_path.c_str(), 1);
		setenv("USER_FILES", userFilesPath.c_str(), 1);

		if(options & GAME_OPTION_GLES2)
		{
			setenv("LIBGL_ES", "2", 1);
			setenv("LIBGL_GL", "21", 1);
			setenv("LIBGL_USEVBO", "0", 1);
			setenv("LIBGL_DEFAULTWRAP", "0", 1);
			setenv("LIBGL_NOINTOVLHACK", "1", 1);
		}

#ifdef EDUKE32 // Eduke needs this enabled
		setenv("LIBGL_USEVBO", "1", 1);
#endif

		if(options & GAME_OPTION_SDL_OLD_AUDIO)
			setenv("SDL_AUDIODRIVER", "android", 1);

		if(options & GAME_OPTION_SDL_AAUDIO_AUDIO)
			setenv("SDL_AUDIODRIVER", "AAudio", 1);

		chdir(game_path.c_str());
#ifndef NO_SEC
		strcpy(keyGlobal, key);
		strcpy(pkgGlobal, pkg);
#endif
		//mobile_init(android_screen_width, android_screen_height, graphics_path.c_str(), options, wheelNbr, game);

		//touchInterface = new TouchInterface();
		touchInterface.init(mobile_screen_width, mobile_screen_height, filesPath.c_str(), options, wheelNbr, game);

		PortableInit(argc, argv); //Never returns!!

		LOGI("PortableInit returned");

		return 0;
	}

	void EXPORT_ME
	JAVA_FUNC(setScreenSize)(JNIEnv* env,	jobject thiz, jint width, jint height)
	{
		LOGI("setScreenSize %d x %d", width, height);
		mobile_screen_width = width;
		mobile_screen_height = height;
	}

	void EXPORT_ME
	JAVA_FUNC(setFramebufferSize)(JNIEnv* env,	jobject thiz, jint width, jint height)
	{
		LOGI("setFramebufferSize %d x %d", width, height);

		touchcontrols::fbConfig config;
		config.vidWidth = width;
		config.vidHeight = height;
		config.vidWidthReal = mobile_screen_width;
		config.vidHeightReal = mobile_screen_height;

		touchcontrols::R_FrameBufferConfig(config);
	}

	int EXPORT_ME
	JAVA_FUNC(doAction)(JNIEnv *env, jobject obj,	jint state, jint action)
	{
		if((action == PORT_ACT_VOLUME_UP) || (action == PORT_ACT_VOLUME_DOWN))
		{
			return touchInterface.volumeKey(state, (action == PORT_ACT_VOLUME_UP));
		}
		else
		{
			touchInterface.gamepadAction(state, action);
			return 0;
		}
	}

#ifndef NO_SEC
	JNIEnv * getEnv();
	int keyCheck()
	{
		JNIEnv * env = getEnv();
		return checkLicense(env, keyGlobal, pkgGlobal);
	}

	static int apkRandomDelay = -1;
	static int check = -1;
#endif

	void EXPORT_ME
	JAVA_FUNC(touchEvent)(JNIEnv *env, jobject obj, jint action, jint pid, jfloat x, jfloat y)
	{
#ifndef NO_SEC
		//LOGI("TOUCHED");
		if(apkRandomDelay == -1)
		{
			apkRandomDelay = 100 + (rand() % 300);
		}

		if(apkRandomDelay)
		{
			apkRandomDelay--;
		}
		else
		{
			if(check == -1)
			{
				check = checkLicense(env, key, pkg);
			}

			if(check != 1)
				return;
		}
#else
		    // Beta test time
		    time_t t = time(NULL);
		    struct tm tm = *localtime(&t);
		    int yr =  tm.tm_year + 1900;
		    int mo = tm.tm_mon + 1;
		    //LOGI("%d   %d",yr,mo);
		    if(yr > 2023)
		    {
		        return;
		    }
#endif

		touchInterface.processPointer(action, pid, x, y);
	}

	void EXPORT_ME
	JAVA_FUNC(backButton)(JNIEnv *env, jobject obj)
	{
		touchInterface.mobileBackButton();
	}

	void EXPORT_ME
	JAVA_FUNC(analogFwd)(JNIEnv *env, jobject obj,	jfloat v, jfloat raw)
	{
		touchInterface.axisValue(ANALOGUE_AXIS_FWD, raw);
		PortableMoveFwd(v);
	}

	void EXPORT_ME
	JAVA_FUNC(analogSide)(JNIEnv *env, jobject obj, jfloat v, jfloat raw)
	{
		touchInterface.axisValue(ANALOGUE_AXIS_SIDE, raw);
		PortableMoveSide(v);
	}

	void EXPORT_ME
	JAVA_FUNC(analogPitch)(JNIEnv *env, jobject obj, jint mode, jfloat v, jfloat raw)
	{
		if(mode == LOOK_MODE_JOYSTICK)
			touchInterface.axisValue(ANALOGUE_AXIS_PITCH, raw);

		PortableLookPitch(mode, v);
	}

	void EXPORT_ME
	JAVA_FUNC(analogYaw)(JNIEnv *env, jobject obj,	jint mode, jfloat v, jfloat raw)
	{
		if(mode == LOOK_MODE_JOYSTICK)
			touchInterface.axisValue(ANALOGUE_AXIS_YAW, raw);

		PortableLookYaw(mode, v);
	}


	void EXPORT_ME
	JAVA_FUNC(weaponWheelSettings)(JNIEnv *env, jobject obj, jint useMoveStick, jint mode, jint autoTimeout)
	{
		LOGI("GAMEPAD WEAPON WHEEL: userMoveStick = %d, mode = %d, timeout = %d", useMoveStick, mode, autoTimeout);
		touchInterface.weaponWheelSettings(useMoveStick, mode, autoTimeout);
	}

	int AUDIO_OVERRIDE_FREQ = 0;
	int AUDIO_OVERRIDE_SAMPLES = 0;

	void EXPORT_ME
	JAVA_FUNC(audioOverride)(JNIEnv *env, jobject obj, jint freq, jint samples)
	{
		LOGI("Sound settings: freq = %d, samples = %d", freq, samples);
		AUDIO_OVERRIDE_FREQ = freq;
		AUDIO_OVERRIDE_SAMPLES = samples;
	}


	int EXPORT_ME
	JAVA_FUNC(loadTouchSettings)(JNIEnv *env, jobject obj, jstring filename)
	{
		const char * filename_c = (const char *)(env)->GetStringUTFChars(filename, 0);
		LOGI("loadTouchSettings %s", filename_c);

		touchInterface.loadControlSettings(filename_c);

		env->ReleaseStringUTFChars(filename, filename_c);
		return 0;
	}

	int EXPORT_ME
	JAVA_FUNC(saveTouchSettings)(JNIEnv *env, jobject obj, jstring filename)
	{
		const char * filename_c = (const char *)(env)->GetStringUTFChars(filename, 0);
		LOGI("saveTouchSettings %s", filename_c);

		touchInterface.saveControlSettings(filename_c);

		env->ReleaseStringUTFChars(filename, filename_c);
		return 0;
	}

	int EXPORT_ME
	JAVA_FUNC(executeCommand)(JNIEnv *env, jobject obj, jstring command)
	{
		const char * command_c = (const char *)(env)->GetStringUTFChars(command, 0);
		LOGI("executeCommand %s", command_c);

		touchInterface.executeCommand(command_c);

		env->ReleaseStringUTFChars(command, command_c);
		return 0;
	}

	FILE *tmpfile()
	{
		FILE * handle = nullptr;

		char path[256];

		snprintf(path, 256, "%s/fileXXXXXX", tempFilesPath.c_str());

		LOGI("Temp file = %s", path );

		int descriptor = mkstemp(path);

		if(-1 != descriptor)
		{
			handle = fdopen(descriptor, "w+b");

			if(nullptr == handle)
			{
				close(descriptor);
			}

			// File already open,
			// can be unbound from the file system
            unlink(path);
		}
		else
		{
			LOGI("ERROR CREATING TEMP FILE NAME");
		}

		return handle;
	}

	const char * getFilesPath()
	{
		return filesPath.c_str();
	}

	int blockGamepad(void)
	{
		return touchInterface.blockGamepad();
	}

	void MouseButton(int state, int button)
	{
		if(state)
			SDL_InjectMouse(button, ACTION_DOWN, 0, 0, SDL_TRUE);
		else
			SDL_InjectMouse(0, ACTION_UP, 0, 0, SDL_TRUE);
	}

	void MouseMove(float dx, float dy)
	{
		static float mx = 0;
		static float my = 0;
		//LOGI("%f %f",dx,dy);
		mx += -dx;
		my +=  -dy;

		if((fabs(mx) > 1) || (fabs(my) > 1))
		{
			SDL_InjectMouse(0, ACTION_MOVE, (int)mx, (int)my, SDL_TRUE);
		}

		if(fabs(mx) > 1)
			mx = fmod(mx, 1); // save remainder for smoother mouse

		if(fabs(my) > 1)
			my = fmod(my, 1);
	}

	int getGameType()
	{
		return gameType;
	}


	void frameControlsSDLCallback(void)
	{
		touchInterface.frameControls();
	}

	void showKeyboardCallbackSDLCallback(int show)
	{
		touchInterface.showKeyboardCallback(show);
	}

	void showMouseSDLCallback(int show)
	{
		touchInterface.showMouseCallback(show);
	}

	void moveMouseSDLCallback(float x, float y)
	{
		touchInterface.moveMouseCallback(x, y);
	}



	int checkGfx()
	{
#ifndef NO_SEC
	#include "./secure/check_include.h"
	return 0;
#else
	return 0;
#endif
	}
}