#ifndef VIRA_PLATFORM_WINDOWS_COMPAT_HPP
#define VIRA_PLATFORM_WINDOWS_COMPAT_HPP

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>

    // Clean up macro pollution that conflicts with third-party libraries
    #ifdef TBYTE
        #undef TBYTE
    #endif
    #ifdef min
        #undef min  
    #endif
    #ifdef max
        #undef max
    #endif
#endif

#endif 