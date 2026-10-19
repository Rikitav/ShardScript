// flag.h -- v1.7.1 -- command-line flag parsing
//
//   Inspired by Go's flag module: https://pkg.go.dev/flag
//
// # Macros API
//
// - FLAG_LIST_INIT_CAP - initial capacity of the Flag_List and Flag_List_Mut Dynamic Arrays.
// - FLAGS_CAP - how many flags you can define within a single context.
// - FLAG_PUSH_DASH_DASH_BACK - make flag_parse() retain "--" in the rest args
//   (available via flag_rest_argc() and flag_rest_argv()). Useful when you need
//   to know whether flag_parse() has stopped due to encountering "--" or due to
//   encountering a non-flag. Ideally this should've been a default behavior,
//   but it breaks backward compatibility. Hence it's a feature macro.
//
//   TODO: make FLAG_PUSH_DASH_DASH_BACK a default behavior on a major version upgrade.
//   Or maybe even better just expose some sort of a flag that tells the user whether
//   the dash-dash was encountered or not.
//
// # Ignoring Flags
//
//   Flag.h implements an experimental syntax for ignoring flags. Consider the following command line:
//
//   ```console
//   $ ./command -arg1 value1 -arg2 -arg3 value3
//   ```
//
//   It provides three arguments `-arg1 value1`, `-arg2`, and `-arg3 value3`. By putting a forward slash `/`
//   after the dash `-` in front of the argument you tell flag.h to parse the argument as usual, check the
//   syntax check the type, but treat it as it was never provided.
//
//   ```console
//   $ ./command -/arg1 value1 -arg2 -arg3 value3
//   ```
//
//   In the above example only `-arg2` and `-arg3 value3` are provided, while `-/arg1 value1` is parsed, but
//   ignored.
//
//   This enables you to "comment out" certain arguments so you can reenable them later as you rerun the same
//   command over and over again in the terminal by pressing Up and then Enter.
#ifndef FLAG_H_
#define FLAG_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <float.h>

#ifndef FLAGS_CAP
#define FLAGS_CAP 256
#endif // FLAGS_CAP

#ifndef FLAG_LIST_INIT_CAP
#define FLAG_LIST_INIT_CAP 8
#endif // FLAG_LIST_INIT_CAP

// Works with both Flag_List and Flag_List_Mut
#define flag_list_append(type, list, item)                                                         \
    do {                                                                                           \
        if ((list)->count >= (list)->capacity) {                                                   \
            size_t new_capacity = (list)->capacity == 0 ? FLAG_LIST_INIT_CAP : (list)->capacity*2; \
            (list)->items = (type*)realloc((list)->items, new_capacity*sizeof(*(list)->items));    \
            (list)->capacity = new_capacity;                                                       \
        }                                                                                          \
                                                                                                   \
        (list)->items[(list)->count++] = item;                                                     \
    } while(0)

typedef struct {
    const char** items;
    size_t count;
    size_t capacity;
} Flag_List;

// The only reason Flag_List_Mut exists is to enable recursive usage of flag[_c]_parse(..) in a backward compatible manner.
// That is using flag[_c]_parse(..) on Flag_List_Mut-s acquired from other flag[_c]_parse(..) calls, since argv must be mutable
// to enable -flag=value syntax.
// TODO: It was a mistake to make items const in Flag_List in the first place. In the next major release get rid of Flag_List_Mut and make items mutable in Flag_List
typedef struct {
    char** items;
    size_t count;
    size_t capacity;
} Flag_List_Mut;

/// API that operate on implicit global flag context

char* flag_name(void* val);

bool* flag_bool(const char* name, bool def, const char* desc);
void flag_bool_var(bool* var, const char* name, bool def, const char* desc);

float* flag_float(const char* name, float def, const char* desc);
void flag_float_var(float* var, const char* name, float def, const char* desc);

double* flag_double(const char* name, double def, const char* desc);
void flag_double_var(double* var, const char* name, double def, const char* desc);

uint64_t* flag_uint64(const char* name, uint64_t def, const char* desc);
void flag_uint64_var(uint64_t* var, const char* name, uint64_t def, const char* desc);

size_t* flag_size(const char* name, uint64_t def, const char* desc);
void flag_size_var(size_t* var, const char* name, uint64_t def, const char* desc);

char** flag_str(const char* name, const char* def, const char* desc);
void flag_str_var(char** var, const char* name, const char* def, const char* desc);

Flag_List* flag_list(const char* name, const char* desc);
void flag_list_var(Flag_List* var, const char* name, const char* desc);

Flag_List_Mut* flag_list_mut(const char* name, const char* desc);
void flag_list_mut_var(Flag_List_Mut* var, const char* name, const char* desc);

void flag_set_short_name(void* val, const char* short_name);

bool flag_parse(int argc, char** argv);
int flag_rest_argc(void);
char** flag_rest_argv(void);
const char* flag_program_name(void);
void flag_print_error(FILE* stream);
void flag_print_options(FILE* stream);

/// API that operate on a custom opaque flag context.

// Allocate a new opaque flag context.
//
// Setting `program_name` is needed so `flag_c_parse(c, argc, argv)` does not try to consume
// extra argument from `argv` as it usually does. If that's exactly what you want just pass
// `NULL` as the `program_name` then.
//
// If specific `program_name` does not make sense in your case, you can just set it to whatever,
// since internally it's not used for anything except consuming an extra argument at the beginning
// of flag[_c]_parse(..) and giving that argument back to the user via flag[_c]_program_name(..).
// If you are designing a subcommand system where each subcommand has its own context you can set
// `program_name` to the name of the corresponding subcommand.
void* flag_c_new(const char* program_name);
void flag_c_free(void* c);
char* flag_c_name(void* c, void* val);

bool* flag_c_bool(void* c, const char* name, bool def, const char* desc);
void flag_c_bool_var(void* c, bool* var, const char* name, bool def, const char* desc);

float* flag_c_float(void* c, const char* name, float def, const char* desc);
void flag_c_float_var(void* c, float* var, const char* name, float def, const char* desc);

double* flag_c_double(void* c, const char* name, double def, const char* desc);
void flag_c_double_var(void* c, double* var, const char* name, double def, const char* desc);

uint64_t* flag_c_uint64(void* c, const char* name, uint64_t def, const char* desc);
void flag_c_uint64_var(void* c, uint64_t* var, const char* name, uint64_t def, const char* desc);

size_t* flag_c_size(void* c, const char* name, uint64_t def, const char* desc);
void flag_c_size_var(void* c, size_t* var, const char* name, uint64_t def, const char* desc);

char** flag_c_str(void* c, const char* name, const char* def, const char* desc);
void flag_c_str_var(void* c, char** var, const char* name, const char* def, const char* desc);

Flag_List* flag_c_list(void* c, const char* name, const char* desc);
void flag_c_list_var(void* c, Flag_List* var, const char* name, const char* desc);

Flag_List_Mut* flag_c_list_mut(void* c, const char* name, const char* desc);
void flag_c_list_mut_var(void* c, Flag_List_Mut* var, const char* name, const char* desc);

void flag_c_set_short_name(void* c, void* val, const char* short_name);
void flag_c_set_short_name_by_name(void* c, const char* name, const char* short_name);

bool flag_c_parse(void* c, int argc, char** argv);
int flag_c_rest_argc(void* c);
char** flag_c_rest_argv(void* c);
const char* flag_c_program_name(void* c);
void flag_c_print_error(void* c, FILE* stream);
void flag_c_print_options(void* c, FILE* stream);

#endif // FLAG_H_

//////////////////////////////

typedef enum {
    FLAG_BOOL = 0,
    FLAG_UINT64,
    FLAG_DOUBLE,
    FLAG_FLOAT,
    FLAG_SIZE,
    FLAG_STR,
    FLAG_LIST,
    FLAG_LIST_MUT,
    COUNT_FLAG_TYPES,
} Flag_Type;

static_assert(COUNT_FLAG_TYPES == 8, "Exhaustive Flag_Value definition");
typedef union {
    char* as_str;
    uint64_t as_uint64;
    double as_double;
    float as_float;
    bool as_bool;
    size_t as_size;
    Flag_List as_list;
    Flag_List_Mut as_list_mut;
} Flag_Value;

typedef enum {
    FLAG_NO_ERROR = 0,
    FLAG_ERROR_UNKNOWN,
    FLAG_ERROR_NO_VALUE,
    FLAG_ERROR_INVALID_NUMBER,
    FLAG_ERROR_INTEGER_OVERFLOW,
    FLAG_ERROR_FLOAT_OVERFLOW,
    FLAG_ERROR_DOUBLE_OVERFLOW,
    FLAG_ERROR_INVALID_SIZE_SUFFIX,
    COUNT_FLAG_ERRORS,
} Flag_Error;

typedef struct {
    Flag_Type type;
    char* name;
    char* short_name;
    char* desc;

    Flag_Value val;
    void* ref;

    Flag_Value def;
} Flag;

typedef struct {
    Flag flags[FLAGS_CAP];
    size_t flags_count;

    Flag_Error flag_error;
    char* flag_error_name;

    const char* program_name;

    int rest_argc;
    char** rest_argv;
} Flag_Context;

/*
   Revision history:

     1.7.1 (2025-10-24) Remove flag_error_value from Flag_Context (@rexim)
     1.7.0 (2025-09-27) Add float and double flags (by @ByXeno)
                        Add more size suffixes (by @ByXeno)
     1.6.1 (2025-09-23) Remove use_ref from Flag_Context
     1.6.0 (2025-09-22) Introduce *_var variants of flag functions
     1.5.0 (2025-09-22) Introduce -/flag syntax for ignoring flags
     1.4.1 (2025-09-05) Fix -Wswitch-enum warning for GCC/Clang
     1.4.0 (2025-07-23) Add support for explicit flag contexts
                        Add Flag_List_Mut
                        Implement `-key=value` syntax for flag lists
     1.3.0 (2025-07-21) Add support for `-key=value` syntax (by @ej-shafran)
     1.2.1 (2025-07-04) flag_print_options: denote expected argument types
                        flag_print_options: indicate flag list usage more clearly
     1.2.0 (2025-05-31) Introduce FLAG_PUSH_DASH_DASH_BACK (by @nullnominal)
     1.1.0 (2025-05-09) Introduce flag list
     1.0.0 (2025-03-03) Initial release
                        Save program_name in the context

*/

// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
