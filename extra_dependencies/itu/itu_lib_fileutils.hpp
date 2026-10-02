#ifndef ITU_LIB_FILEUTILS_HPP
#define ITU_LIB_FILEUTILS_HPP

const char* itu_lib_fileutils_get_file_name(const char* path);

#endif // ITU_LIB_FILEUTILS_HPP

// TMP
#define ITU_LIB_FILEUTILS_IMPLEMENTATION

#if (defined ITU_LIB_FILEUTILS_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL.h>
#endif


#ifdef SDL_PLATFORM_WINDOWS
#define is_path_separator(c) ((c) == '/' || (c) == '\\')
#else
#define is_path_separator(c) ((c) == '/')
#endif

const char* itu_lib_fileutils_get_file_name(const char* path);

#endif //  (defined ITU_LIB_FILEUTILS_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)
