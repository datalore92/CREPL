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

// Updated signature to match MinGW definition
EXPORT int _setjmp(jmp_buf env, void* ctx) { return setjmp(env); }
EXPORT void _longjmp(jmp_buf env, int val) { longjmp(env, val); }

#ifdef __cplusplus
}
#endif
