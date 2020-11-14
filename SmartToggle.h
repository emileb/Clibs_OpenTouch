#include <time.h>

#define TAP_TIME 300

typedef struct
{
	uint64_t timeDown;
	int wasActive;
} SmartToggle_t;

inline uint64_t getMS()
{
	uint64_t ms; // Milliseconds
	time_t   s;  // Seconds
	struct timespec spec;

	clock_gettime(CLOCK_REALTIME, &spec);

	s  = spec.tv_sec;
	ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds

	if(ms > 999)
	{
		s++;
		ms = 0;
	}

	return (s * 1000ull) + ms;
}

int SmartToggleAction( SmartToggle_t *data, int buttonState, int isActive )
{
	uint64_t timeNow = getMS();

	int newState = 0;

	if(buttonState) // Button pressed
	{
		data->wasActive = isActive;
		data->timeDown = timeNow;
		newState = 1; // When being press, we always activate
	}
	else // Button released
	{
		if((timeNow - data->timeDown) < TAP_TIME) // Was a tap
		{
			newState = data->wasActive ? 0: 1; // toggle action
		}
		else // Long press, action off
		{
			newState = 0;
		}
	}

	return newState;
}