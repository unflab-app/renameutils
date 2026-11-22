/* quoting.c - Quoting and dequoting strings like in the C language.
 *
 * Copyright (C) 2001, 2002, 2004, 2005, 2007, 2008 Oskar Liljeblad
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <ctype.h>  	    /* C89 */
#include <wchar.h>	    /* C99 */
#include <wctype.h>	    /* C99 */
#include <string.h> 	    /* gnulib (C89) */
#include "common/common.h"
#include "common/strbuf.h"
#include "qcmd.h"

static const char *c_abbrevs = "abtnvfr";

static inline bool
is_octal_digit(char c)
{
    return (c >= '0' && c <= '7');
}

static inline char
octal_to_char(const char *str)
{
    return (char) ((str[0]-'0')*64 + (str[1]-'0')*8 + (str[2]-'0'));
}

/**
 * Quote characters in a string, as is done in C: backslash, and
 * every byte that isn't part of a printable character in the
 * current locale. A name in UTF-8 is shown as it is, rather than
 * as octal escapes for each byte of every non-ASCII character.
 *
 * @param t
 *   The string to quote.
 * @returns
 *   A newly allocated string.
 */
char *
quote_output_file(const char *t)
{
    StrBuf *res;
    int c;

    mbstate_t state;
    size_t len = strlen(t);

    res = strbuf_new();
    memset(&state, 0, sizeof(state));

    for (c = 0; t[c] != '\0'; ) {
	unsigned char b = t[c];
	wchar_t wc;
	size_t n;

	if (b == '\\') {
	    strbuf_append(res, "\\\\");
	    c++;
	    continue;
	}
	n = mbrtowc(&wc, t + c, len - c, &state);
	if (n == (size_t) -1 || n == (size_t) -2 || n == 0) {
	    /* Not a valid character: escape this byte, start afresh. */
	    memset(&state, 0, sizeof(state));
	    n = 1;
	} else if (iswprint(wc)) {
	    strbuf_append_substring(res, t, c, c + n);
	    c += n;
	    continue;
	}
	/* Escape every byte of a non-printable or invalid character. */
	for (; n > 0; n--, c++) {
	    b = t[c];
	    if (b >= 7 && b <= 13)
		strbuf_appendf(res, "\\%c", c_abbrevs[b-7]);
	    else
		strbuf_appendf(res, "\\%03o", b);
	}
    }

    return strbuf_free_to_string(res);
}

/**
 * Dequote a string.
 *
 * @param str
 *   The string to dequote.
 * @returns
 *   A newly allocated string.
 */
char *
dequote_output_file(const char *str)
{
    StrBuf *res;
    int len;
    int c;

    len = strlen(str);

    res = strbuf_new();
    for (c = 0; c < len; c++) {
	if (str[c] == '\\') {
	    c++;
	    if (c >= len) {
		strbuf_free(res);
		return NULL;
	    }

	    if (is_octal_digit(str[c])) {
		if (c+2 >= len || !is_octal_digit(str[c+1]) || !is_octal_digit(str[c+2])) {
			strbuf_free(res);
			return NULL;
		}
		strbuf_append_char(res, octal_to_char(&str[c]));
		c += 2;
	    } else if (strchr(c_abbrevs, str[c]) != NULL) {
		strbuf_append_char(res, 7 + (strchr(c_abbrevs, str[c]) - c_abbrevs));
	    } else {
		strbuf_append_char(res, str[c]);
	    }
	} else {
	    strbuf_append_char(res, str[c]);
	}
    }

    return strbuf_free_to_string(res);
}
