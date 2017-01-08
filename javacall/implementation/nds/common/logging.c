/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include "javacall_logging.h"
#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOGGING_TO_FILE
/**
 * Prints out a string to a system specific output strream
 *
 * @param s a NULL terminated character buffer to be printed
*/
//Internal version
void __javacall_log_to_file(const char *s) {
    FILE *f = fopen("fat1:/messages.log", "a+b");
    if (f == NULL)
        return;

    fwrite(s, 1, strlen(s), f);
    fwrite("\n", 1, 1, f);

    fclose(f);
}

#define BUFFERSIZE (1 << 7)
static char buffer[BUFFERSIZE];

void javacall_log_to_file(const char *s, ...) {
	va_list ap;
    va_start(ap, s);
    vsnprintf(buffer, sizeof(buffer) - 1, s, ap);
    javacall_print(buffer);
    va_end(ap);
}

void javacall_print(const char *s) {

    printf("%s", s);

#ifdef LOGGING_TO_FILE
    //TODO: need to move this out
	__javacall_log_to_file(s);
#endif
}

void javacall_printf(const char *s, ...) {
    va_list ap;

    if (NULL == s)
        return;

    va_start(ap, s);
    vsnprintf(buffer, sizeof(buffer) - 1, s, ap);
    javacall_print(buffer);
    va_end(ap);
}

#ifdef __cplusplus
}
#endif
