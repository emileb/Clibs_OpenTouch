
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    void LogWritter_Init( const char *filename );

    void LogWritter_Write( const char *msg );

#ifdef __cplusplus
}
#endif