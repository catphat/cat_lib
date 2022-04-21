#ifndef CATLIB_CORE_H_
#define CATLIB_CORE_H_

// The fmt library version in the form major * 10000 + minor * 100 + patch.
#define CATLIB_VERSION 10100 // v1.1

#if !defined(CATLIB_HEADER_ONLY) && defined(_WIN32)
#  define CATLIB_CLASS_API CATLIB_MSC_WARNING(suppress : 4275)
#  ifdef CATLIB_EXPORT
#    define CATLIB_API __declspec(dllexport)
#  elif defined(CATLIB_SHARED)
#    define CATLIB_API __declspec(dllimport)
#  endif
#else
#  define CATLIB_CLASS_API
#  if defined(CATLIB_EXPORT) || defined(CATLIB_SHARED)
#    if defined(__GNUC__) || defined(__clang__)
#      define CATLIB_API __attribute__((visibility("default")))
#    endif
#  endif
#endif
#ifndef CATLIB_API
#  define CATLIB_API
#endif


#ifdef CATLIB_HEADER_ONLY
#  include "catlib.h"
#endif
#endif  // CATLIB_CORE_H_

