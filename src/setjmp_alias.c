#include <setjmp.h>

// Ensure proper linkage and export for Windows builds
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

// Use extern "C" to prevent name mangling
#ifdef __cplusplus
extern "C" {
#endif

// Platform-specific implementations
#ifdef _WIN32
// Windows (MinGW) requires the extra ctx parameter
EXPORT int _setjmp(jmp_buf env, void* ctx) { return setjmp(env); }
#else
// Linux version doesn't have the extra parameter
EXPORT int _setjmp(jmp_buf env) { return setjmp(env); }
#endif

// Common implementation for _longjmp
EXPORT void _longjmp(jmp_buf env, int val) { longjmp(env, val); }

#ifdef __cplusplus
}
#endif
