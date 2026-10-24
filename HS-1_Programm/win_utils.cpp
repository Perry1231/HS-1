// Обхід конфліктів Windows API vs Raylib
#if defined(_WIN32)
    #define NOGDI
    #define NOUSER
#endif

#include "raylib.h"

#if defined(_WIN32)
    #include <windows.h>
    
    // Перестраховка від макросів WinAPI
    #ifdef CloseWindow
        #undef CloseWindow
    #endif
    #ifdef ShowCursor
        #undef ShowCursor
    #endif
    #ifdef LoadImage
        #undef LoadImage
    #endif
    #ifdef Rectangle
        #undef Rectangle
    #endif
#endif