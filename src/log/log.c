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


#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static LOGLEVEL _loglevel = INFO;

void set_loglevel(LOGLEVEL level) {
  _loglevel = level;
}

void _log(char const * file, int linenumber, char const * const function, LOGLEVEL level, char const * const format, ...) {
  if (level < _loglevel) return;

  va_list args;
  va_start(args, format);

  char const * tmp = strstr(file, "/src/");
  if (tmp) file = tmp + 1;

  fprintf(stderr, "%s:%s:%d ", file, function, linenumber);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  fflush(stderr);
}

/* returns a malloc'ed string which needs to be free'd */
char *safe_format(const char *format, ...) {
	va_list args;
	va_start(args, format);

	return vsafe_format(format, args);
}

char *vsafe_format(const char *format, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);

    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed < 0)
        return NULL;

    size_t size = (size_t)needed + 1;

    char *buf = malloc(size);
    if (!buf)
        return NULL;

    vsnprintf(buf, size, format, args);
    return buf;
}
