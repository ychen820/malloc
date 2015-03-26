#ifndef CS550_UTILITY_H
#define CS550_UTILITY_H



/*
 * The utility file includes a few things that might be useful.  It requires a C99 compiler, though.
 * 
 * cs550_print() is a print function that supports %lu, %lx, %s, and %%, and will not call malloc().
 * 
 * cs550_dbg_print() is a macro that calls cs550_print(). If you do not define CS550_DBG_PRINT, then
 * nothing that uses this macro will print out.
 * 
 * cs550_assert() is like regular assert(), except that it won't call malloc().
 * 
 * Lastly, if you define CS550_DEBUG, then all malloc API functions will have
 * cs550_ prefixed in front of them.  This will assist greatly in debugging.
*/



#include <stddef.h>



#ifdef CS550_DBG_PRINT
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wgnu"
    #define cs550_dbg_print(fmt, ...) \
        cs550_print("[%s, %s, %lu]: " fmt, __FILE__, __func__, (unsigned long) __LINE__, ##__VA_ARGS__);
    #pragma GCC diagnostic pop
#else
    #define cs550_dbg_print(fmt, ...) ((void) 0)
#endif



#define cs550_assert(c) \
    (c) ? (void) 0 : cs550_assert_fail(#c, __FILE__, __func__, __LINE__)



#ifdef __cplusplus
extern "C" {
#endif



void cs550_assert_fail(const char *cond, const char *file_name, const char *func_name, int line_num);
void cs550_print(const char *fmt, ...);



#ifdef __cplusplus
}
#endif



#ifdef CS550_DEBUG
    #define malloc cs550_malloc
    #define free cs550_free
    #define calloc cs550_calloc
    #define realloc cs550_realloc
#endif



#endif
