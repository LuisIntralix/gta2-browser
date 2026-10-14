/* unshield 1.6.2 configuration for the Emscripten (WebAssembly) build used by
 * the browser installer import (web/installer-worker.js). Mirrors the CMake
 * result for the native build, without iconv or dlfcn. */
#define HAVE_INTTYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define PACKAGE "unshield"
#define PACKAGE_NAME "unshield"
#define PACKAGE_STRING "unshield 1.6.2"
#define PACKAGE_VERSION "1.6.2"
#define SIZE_FORMAT "zi"
#define HAVE_FNMATCH 1
#define USE_OUR_OWN_MD5 1
#define VERSION "1.6.2"
