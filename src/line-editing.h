/* line-editing.h - the readline API, from GNU readline or libedit
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef LINE_EDITING_H
#define LINE_EDITING_H

#include <stdio.h>
#if USE_LIBEDIT
# include <editline/readline.h>
# include <histedit.h>
#else
# include <readline/readline.h>
# include <readline/history.h>
#endif

#endif
