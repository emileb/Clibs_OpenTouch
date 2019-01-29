#include <jni.h>
#include "TouchControlsInterface.h"
#include "JNITouchControlsUtils.h"
#include "SDL_beloko_extra.h"

#include "TouchControlsInterface.h"
#include "touch_interface.h"

#include "game_interface.h"

#include "LogWritter.h"

#include <unistd.h>


#ifndef NO_SEC
#include "./secure/license/license.h"
#endif

#include <time.h>


extern "C"
{

static int android_screen_width;
static int android_screen_height;

JNIEnv* env_;


//#define JAVA_FUNC(x) Java_com_beloko_opengames_gzdoom2_NativeLib_##x


#define JAVA_FUNC(x) Java_org_libsdl_app_NativeLib_##x

#define EXPORT_ME __attribute__ ((visibility("default")))

//Bit strange! This is called by the setup program to launch the game..
void launchChocGame()
{
	jclass helloWorldClass;
	jmethodID mainMethod;
	helloWorldClass = env_->FindClass("org/libsdl/app/SDLActivity");
	mainMethod = env_->GetStaticMethodID(helloWorldClass, "reLaunch", "()V");
	env_->CallStaticVoidMethod(helloWorldClass, mainMethod);
}

__attribute__((visibility("default"))) jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	LOGI("JNI_OnLoad");
	setTCJNIEnv(vm);
	return JNI_VERSION_1_4;
}

static int argc=1;
static const char * argv[128];

jint EXPORT_ME
JAVA_FUNC(init) ( JNIEnv* env,	jobject thiz,jstring graphics_dir,jint options,jobjectArray argsArray,jint game,jstring game_path_,jstring logFilename )
{
	env_ = env;

	static std::string game_path = (char *)(env)->GetStringUTFChars( game_path_, 0);
	static std::string graphics_path = (char *)(env)->GetStringUTFChars( graphics_dir, 0);
    static std::string log_filename_path = (char *)(env)->GetStringUTFChars( logFilename, 0);


    LogWritter_Init( log_filename_path.c_str());

	argv[0] = "doom";
	int argCount = (env)->GetArrayLength( argsArray);
	LOGI("argCount = %d",argCount);
	for (int i=0; i<argCount; i++) {
		jstring string = (jstring) (env)->GetObjectArrayElement( argsArray, i);
		argv[argc] = (char *)(env)->GetStringUTFChars( string, 0);
		LOGI("arg = %s",argv[argc]);
		LogWritter_Write( argv[argc] );
		argc++;
	}

    LogWritter_Write("\n");

	LOGI("game_path = %s",game_path.c_str());

	setenv("HOME", game_path.c_str(),1);

	setenv("LIBGL_ES","2",1);
	setenv("LIBGL_GL","20",1);
	setenv("LIBGL_DEFAULTWRAP","0",1);

	chdir( game_path.c_str() );

    //checkLicense( env_ );

    mobile_init(android_screen_width, android_screen_height, graphics_path.c_str(),options,game);
	PortableInit(argc,argv); //Never returns!!

	return 0;
}

void EXPORT_ME
JAVA_FUNC(setScreenSize) ( JNIEnv* env,	jobject thiz, jint width, jint height)
{
	android_screen_width = width;
	android_screen_height = height;
}

void EXPORT_ME
JAVA_FUNC(doAction) (JNIEnv *env, jobject obj,	jint state, jint action)
{
	gamepadAction(state,action);
}


static int apkRandomDelay = -1;
static int check = -1;

void EXPORT_ME
JAVA_FUNC(touchEvent) (JNIEnv *env, jobject obj,jint action, jint pid, jfloat x, jfloat y)
{
#ifndef NO_SEC
	//LOGI("TOUCHED");
	if (apkRandomDelay == -1)
    {
        apkRandomDelay = 100 + (rand() % 300);
    }

    if (apkRandomDelay)
    {
        apkRandomDelay--;
    }
    else
    {
        if( check == -1 )
        {
            check =  checkLicense( env,"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0Ty9fat4Mag+a/UAncpVM8lNDrAQxk754HupOlYbJt3ALv6Fqagjj2vzPK8570aALqw2XEk5JxPAazdTQJ+W5aEVM8N2Ij1SbqN/yF+HfqDG+hHfszddwAZzKzWUlAkkeqW6qiIEy4L/TTOgj2vQv24ix4YcpO3eea2Ltz2UDyq+o0+K1cOCMqtuGL/GQbFS92zp3dnH9CpgtWFsbvVarjntJWiI6RrZpqpTTsuZWckK1ztMBjzNNnD1w6QbgTRqoGU7xmsHImWjk5MtwxiDqKL1EFOBvQDqOXxVc/jmT8StqAjk1ItCWStvJLZTzejNoGdTisxBQT/P3Xyppo8/MwIDAQAB",
             "com.opentouchgaming.deltatouch");
        }

        if( check != 1)
            return;
    }
#else
    // Beta test time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    int yr =  tm.tm_year + 1900;
    int mo = tm.tm_mon + 1;
    //LOGI("%d   %d",yr,mo);
    if(yr > 2019 || mo > 6)
    {
        return;
    }
#endif
	mobileGetTouchInterface()->processPointer(action,pid,x,y);
}

void EXPORT_ME
JAVA_FUNC(backButton) (JNIEnv *env, jobject obj)
{
	mobileBackButton();
}

void EXPORT_ME
JAVA_FUNC(analogFwd) (JNIEnv *env, jobject obj,	jfloat v)
{
	PortableMoveFwd(v);
}

void EXPORT_ME
JAVA_FUNC(analogSide) (JNIEnv *env, jobject obj,jfloat v)
{
	PortableMoveSide(v);
}

void EXPORT_ME
JAVA_FUNC(analogPitch) (JNIEnv *env, jobject obj,
jint mode,jfloat v)
{
    PortableLookPitch(mode, v);
}

void EXPORT_ME
JAVA_FUNC(analogYaw) (JNIEnv *env, jobject obj,	jint mode,jfloat v)
{
	PortableLookYaw(mode, v);
}

}