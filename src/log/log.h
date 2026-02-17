/*
 * Backup manager to make backups using tar with gpg encryption and xz compression
 * Copyright (C) 2026 N Liam Waaga
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/


#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
namespace Logger {
extern "C" {
#endif


/* this is the log macro. use it like this `logf(ERROR, "failed to malloc array at address %d", somearr);`*/
#define logf(loglevel, format, ...) _log(__FILE__, __LINE__, __func__, loglevel, format, __VA_ARGS__)
#define log(loglevel, format) _log(__FILE__, __LINE__, __PRETTY_FUNCTION__, loglevel, format)

typedef enum {
INFO,
WARN,
ERROR,
} LOGLEVEL;

void set_loglevel(LOGLEVEL level);


/* internal function for logging, not meant to be used by anything except through the macro */
void _log(char const * const file, int linenumber, char const * const function, LOGLEVEL level, char const * const format, ...);

const char *safe_format(const char *format, ...);

const char *vsafe_format(const char *format, va_list args);


#ifdef __cplusplus
} /* extern "C" */
} /* namespace Logger */
#endif

#endif
