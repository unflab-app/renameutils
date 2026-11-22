/* list.c - Listing the files to edit.
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

/* Files used to be listed by running `ls --quoting-style=c' and parsing
 * its output, which needs GNU ls. They're now read directly, honouring
 * the subset of ls options that qmv has always accepted, with the same
 * orderings and tie-breaks GNU ls uses. */

#include <config.h>
#include <sys/types.h>	    	/* POSIX */
#include <unistd.h>		/* gnulib (POSIX) */
#include <fcntl.h>  	    	/* gnulib (POSIX) */
#include <sys/stat.h>	    	/* gnulib (POSIX) */
#include <dirent.h>		/* POSIX */
#include <getopt.h> 	    	/* gnulib (POSIX) */
#include <stdbool.h>		/* gnulib (C99) */
#include <gettext.h>		/* gnulib (gettext) */
#define _(s) gettext(s)
#define N_(s) (s)
#include <errno.h>  	    	/* C89 */
#include <stdlib.h> 	    	/* C89 */
#include <stdio.h>  	    	/* C89 */
#include <string.h> 	    	/* gnulib (C89) */
#include "filevercmp.h"		/* gnulib */
#include "quote.h"		/* gnulib */
#include "quotearg.h"	    	/* gnulib */
#include "stat-time.h"		/* gnulib */
#include "timespec.h"		/* gnulib */
#include "xalloc.h"		/* gnulib */
#include "xvasprintf.h"		/* gnulib */
#include "common/common.h"
#include "common/io-utils.h"
#include "common/string-utils.h"
#include "common/error.h"
#include "common/llist.h"
#include "common/hmap.h"
#include "qcmd.h"

enum {
    SORT_OPT = 2000,
    TIME_OPT,
};

enum sort_key { SORT_NAME, SORT_NONE, SORT_SIZE, SORT_TIME, SORT_VERSION, SORT_EXTENSION };
enum time_key { TIME_MTIME, TIME_ATIME, TIME_CTIME };

static struct {
    bool all;			/* -a, -A: include names starting with . */
    bool ignore_backups;	/* -B: exclude names ending with ~ */
    bool directory;		/* -d: list directories, not their contents */
    bool reverse;		/* -r */
    bool recursive;		/* -R */
    enum sort_key sort;
    bool sort_given;		/* an explicit sort beats the one -c/-u imply */
    enum time_key time;
} opts;

/* One name to list. NAME is what is sorted on: the entry's own name in
 * a directory, or the operand as given. PATH is what goes in the list,
 * relative to the work directory. */
typedef struct {
    char *name;
    char *path;
    struct stat st;
    bool have_st;
} Entry;

static int old_dir = -1;

static struct option ls_option_table[] = {
    { "all",            no_argument,       NULL, 'a' },
    { "almost-all",     no_argument,       NULL, 'A' },
    { "ignore-backups", no_argument,       NULL, 'B' },
    { "directory",      no_argument,       NULL, 'd' },
    { "reverse",        no_argument,       NULL, 'r' },
    { "recursive",      no_argument,       NULL, 'R' },
    { "sort",           required_argument, NULL, SORT_OPT },
    { "time",           required_argument, NULL, TIME_OPT },
    { 0 },
};

static void
reset_ls_options(void)
{
    memset(&opts, 0, sizeof(opts));
    opts.sort = SORT_NAME;
    opts.time = TIME_MTIME;
}

struct option *
append_ls_options(const struct option *options)
{
    struct option *new_options;
    int c;
    int d;

    reset_ls_options();

    for (c = 0; options[c].name != NULL; c++);
    d = sizeof(ls_option_table)/sizeof(*ls_option_table);

    new_options = xmalloc((c+d) * sizeof(struct option));
    memcpy(new_options, options, c * sizeof(struct option));
    memcpy(new_options+c, ls_option_table, d * sizeof(struct option));

    return new_options;
}

void
display_ls_help(FILE *out)
{
    fprintf(out, _("\
Listing options:\n\
  -a, --all                  do not hide entries starting with .\n\
  -A, --almost-all           same as -a (. and .. are never listed)\n\
  -B, --ignore-backups       do not list entries ending with ~\n\
  -c                         sort by status change time, newest first\n\
  -d, --directory            list directories themselves, not their contents\n\
  -r, --reverse              reverse the sort order\n\
  -R, --recursive            list subdirectories recursively\n\
  -S                         sort by file size, largest first\n\
      --sort=WORD            sort by WORD instead of name: none (-U),\n\
                               size (-S), time (-t), version, extension (-X)\n\
      --time=WORD            sort by time WORD instead of modification\n\
                               time: atime, access, use (-u); ctime,\n\
                               status (-c)\n\
  -t                         sort by modification time, newest first\n\
  -u                         sort by access time, newest first\n\
  -U                         do not sort; list entries in directory order\n\
  -X                         sort alphabetically by entry extension\n"));
}

static bool
set_sort(const char *word)
{
    static const struct { const char *word; enum sort_key sort; } words[] = {
	{ "name", SORT_NAME },
	{ "none", SORT_NONE },
	{ "size", SORT_SIZE },
	{ "time", SORT_TIME },
	{ "version", SORT_VERSION },
	{ "extension", SORT_EXTENSION },
    };
    size_t c;

    for (c = 0; c < sizeof(words)/sizeof(*words); c++) {
	if (strcmp(word, words[c].word) == 0) {
	    opts.sort = words[c].sort;
	    opts.sort_given = true;
	    return true;
	}
    }
    warn(_("invalid argument %s for `--sort'"), quote(word));
    return false;
}

static bool
set_time(const char *word)
{
    static const struct { const char *word; enum time_key time; } words[] = {
	{ "mtime", TIME_MTIME },
	{ "modification", TIME_MTIME },
	{ "atime", TIME_ATIME },
	{ "access", TIME_ATIME },
	{ "use", TIME_ATIME },
	{ "ctime", TIME_CTIME },
	{ "status", TIME_CTIME },
    };
    size_t c;

    for (c = 0; c < sizeof(words)/sizeof(*words); c++) {
	if (strcmp(word, words[c].word) == 0) {
	    opts.time = words[c].time;
	    return true;
	}
    }
    warn(_("invalid argument %s for `--time'"), quote(word));
    return false;
}

void
import_command(char **args)
{
    FILE *fh;
    LList *new_files;
    HMap *map_files;
    char *buf = NULL;
    size_t bufsize = 0;
    ssize_t rc;

    if (args[1] == NULL) {
        warn(_("missing file argument\n"));
        return;
    }
    fh = fopen(args[1], "r");
    if (fh == NULL) {
        warn(_("%s: cannot open file for reading: %s\n"), quotearg(args[1]), errstr);
        return;
    }

    new_files = llist_new();
    map_files = hmap_new();
    while ((rc = getline(&buf, &bufsize, fh)) > 0) {
	if (buf[rc-1] == '\n')
	    buf[rc-1] = '\0';

        if (strcmp(buf, ".") != 0 && strcmp(buf, "..") != 0) {
            FileSpec *spec;

            spec = new_file_spec();
            spec->old_name = xstrdup(buf);
            spec->new_name = xstrdup(spec->old_name);

            if (hmap_contains_key(map_files, spec->old_name)) {
                warn(_("%s: file already listed"), quotearg(spec->old_name));
                free_file_spec(spec);
            } else {
                hmap_put(map_files, spec->old_name, spec);
                llist_add(new_files, spec);
            }
        }
    }
    free(buf);
    hmap_free(map_files);
    if (ferror(fh)) {
        warn(_("%s: cannot read from file: %s\n"), quotearg(args[1]), errstr);
        llist_iterate(new_files, free_file_spec);
        llist_free(new_files);
        fclose(fh);
        return;
    }
    fclose(fh);

    llist_iterate(work_files, free_file_spec);
    llist_free(work_files);
    work_files = new_files;
}

void
list_command(char **args)
{
    int argc;
    int c;

    for (argc = 1; args[argc] != NULL; argc++);

    reset_ls_options();

    optind = 0;
    while (true) {
	c = getopt_long(argc, args, "aABcdrRStuUX", ls_option_table, NULL);
	if (c == -1)
	    break;
	if (c == '?')
	    return;
	if (!process_ls_option(c, optarg))
	    return;
    }

    if (list_files(args+optind)) {
        const char *fmt = ngettext("%d file listed\n", "%d files listed\n", llist_size(work_files));
        printf(fmt, llist_size(work_files));
    }
}

bool
process_ls_option(int c, const char *arg)
{
    switch (c) {
    case 'a': /* all */
    case 'A': /* almost-all */
	opts.all = true;
	break;
    case 'B': /* ignore-backups */
	opts.ignore_backups = true;
	break;
    case 'c':
	opts.time = TIME_CTIME;
	break;
    case 'd': /* directory */
	opts.directory = true;
	break;
    case 'r': /* reverse */
	opts.reverse = true;
	break;
    case 'R': /* recursive */
	opts.recursive = true;
	break;
    case 'S':
	opts.sort = SORT_SIZE;
	opts.sort_given = true;
	break;
    case SORT_OPT: /* sort */
	return set_sort(arg);
    case 't':
	opts.sort = SORT_TIME;
	opts.sort_given = true;
	break;
    case TIME_OPT: /* time */
	return set_time(arg);
    case 'u':
	opts.time = TIME_ATIME;
	break;
    case 'U':
	opts.sort = SORT_NONE;
	opts.sort_given = true;
	break;
    case 'X':
	opts.sort = SORT_EXTENSION;
	opts.sort_given = true;
	break;
    }
    return true;
}

bool
cwd_from_work_directory()
{
    if (old_dir != -1) {
	if (fchdir(old_dir) < 0) {
	    warn(_("cannot change back to old directory: %s"), errstr);
	    close(old_dir);
	    return false;
	}
	close(old_dir);
	old_dir = -1;
    }
    return true;
}

bool
cwd_to_work_directory()
{
    if (strcmp(work_directory, ".") == 0) {
	old_dir = -1;
	return true;
    }
    if ((old_dir = open(".", O_RDONLY)) < 0) {
	warn(_("cannot get current directory: %s"), errstr);
	return false;
    }
    if (chdir(work_directory) < 0) {
	warn(_("%s: cannot change to directory: %s"), quotearg(work_directory), errstr);
	close(old_dir);
	return false;
    }
    return true;
}

/* The comparisons below are GNU ls's: newest and largest first, ties
 * broken by name, and names compared in the locale's collation order. */

static int
compare_names(const char *a, const char *b)
{
    int rc;

    errno = 0;
    rc = strcoll(a, b);
    if (errno != 0 || rc == 0)
	rc = strcmp(a, b);
    return rc;
}

static const char *
extension(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot != NULL ? dot : "";
}

static struct timespec
entry_time(const Entry *e)
{
    switch (opts.time) {
    case TIME_ATIME:
	return get_stat_atime(&e->st);
    case TIME_CTIME:
	return get_stat_ctime(&e->st);
    default:
	return get_stat_mtime(&e->st);
    }
}

static enum sort_key
effective_sort(void)
{
    /* As in GNU ls, -c, -u and --time also mean "sort by that time",
       unless a sort order was asked for explicitly. */
    if (!opts.sort_given && opts.time != TIME_MTIME)
	return SORT_TIME;
    return opts.sort;
}

static int
compare_entries(const void *pa, const void *pb)
{
    const Entry *a = *(const Entry **) pa;
    const Entry *b = *(const Entry **) pb;
    int rc = 0;

    switch (effective_sort()) {
    case SORT_SIZE:
	if (a->st.st_size != b->st.st_size)
	    rc = a->st.st_size > b->st.st_size ? -1 : 1;
	break;
    case SORT_TIME:
	rc = -timespec_cmp(entry_time(a), entry_time(b));
	break;
    case SORT_VERSION:
	rc = filevercmp(a->name, b->name);
	break;
    case SORT_EXTENSION:
	rc = compare_names(extension(a->name), extension(b->name));
	break;
    default:
	break;
    }
    if (rc == 0)
	rc = compare_names(a->name, b->name);
    return opts.reverse ? -rc : rc;
}

static void
sort_entries(Entry **entries, size_t count)
{
    if (effective_sort() != SORT_NONE && count > 1)
	qsort(entries, count, sizeof(*entries), compare_entries);
}

static Entry *
new_entry(const char *name, const char *path)
{
    Entry *e = xmalloc(sizeof(Entry));

    e->name = xstrdup(name);
    e->path = xstrdup(path);
    e->have_st = (lstat(path, &e->st) == 0);
    return e;
}

static void
free_entry(Entry *e)
{
    free(e->name);
    free(e->path);
    free(e);
}

static void
add_spec(LList *files, HMap *seen, const char *path)
{
    FileSpec *spec;

    if (hmap_contains_key(seen, path)) {
	warn(_("%s: file already listed"), quotearg(path));
	return;
    }
    spec = new_file_spec();
    spec->old_name = xstrdup(path);
    spec->new_name = xstrdup(path);
    hmap_put(seen, spec->old_name, spec);
    llist_add(files, spec);
}

/* Add the contents of directory DIR (relative to the work directory;
 * "." for the work directory itself) to FILES, and with -R, the
 * contents of its subdirectories after it. */
static void
list_directory(const char *dir, LList *files, HMap *seen)
{
    DIR *dp;
    struct dirent *ep;
    Entry **entries = NULL;
    size_t count = 0;
    size_t alloc = 0;
    size_t c;

    dp = opendir(dir);
    if (dp == NULL) {
	warn(_("%s: cannot open directory: %s"), quotearg(dir), errstr);
	return;
    }
    for (;;) {
	char *path;

	errno = 0;
	ep = readdir(dp);
	if (ep == NULL)
	    break;
	if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
	    continue;
	if (!opts.all && ep->d_name[0] == '.')
	    continue;
	if (opts.ignore_backups && ep->d_name[strlen(ep->d_name)-1] == '~')
	    continue;

	if (strcmp(dir, ".") == 0)
	    path = xstrdup(ep->d_name);
	else
	    path = xasprintf("%s%s%s", dir, dir[strlen(dir)-1] == '/' ? "" : "/", ep->d_name);
	if (count == alloc)
	    entries = x2nrealloc(entries, &alloc, sizeof(*entries));
	entries[count++] = new_entry(ep->d_name, path);
	free(path);
    }
    if (errno != 0)
	warn(_("%s: cannot read directory: %s"), quotearg(dir), errstr);
    closedir(dp);

    sort_entries(entries, count);
    for (c = 0; c < count; c++)
	add_spec(files, seen, entries[c]->path);

    /* Like ls -R: subdirectories in the order listed, and never through
       a symbolic link, since lstat sees the link rather than its target. */
    if (opts.recursive) {
	for (c = 0; c < count; c++) {
	    if (entries[c]->have_st && S_ISDIR(entries[c]->st.st_mode))
		list_directory(entries[c]->path, files, seen);
	}
    }

    for (c = 0; c < count; c++)
	free_entry(entries[c]);
    free(entries);
}

/**
 * Generate a list of specified files.
 * Return true if listing succeeded.
 */
bool
list_files(char **args)
{
    Entry **operands = NULL;
    size_t count = 0;
    size_t alloc = 0;
    size_t c;
    bool list_work_dir;
    HMap *seen;

    free(work_directory);
    work_directory = xstrdup(".");

    /* A single directory becomes the work directory, so the names in
       the editor are relative to it. Without arguments, the current
       directory is listed. Otherwise directory arguments are skipped,
       unless -d asks for the directories themselves. */
    list_work_dir = (args[0] == NULL);
    if (args[0] != NULL && args[1] == NULL && !opts.directory && is_directory(args[0])) {
	char *old = work_directory;
	work_directory = xstrdup(args[0]);
	if (!cwd_to_work_directory()) {
	    free(work_directory);
	    work_directory = old;
	    return false;
	}
	free(old);
	list_work_dir = true;
    } else {
	for (; *args != NULL; args++) {
	    Entry *e;

	    if (!opts.directory && is_directory(*args)) {
		warn(_("%s: skipping directory argument"), quotearg(*args));
		continue;
	    }
	    e = new_entry(*args, *args);
	    if (!e->have_st) {
		warn(_("%s: cannot access: %s"), quotearg(*args), errstr);
		free_entry(e);
		continue;
	    }
	    if (count == alloc)
		operands = x2nrealloc(operands, &alloc, sizeof(*operands));
	    operands[count++] = e;
	}
    }

    llist_clear(edit_file_list);
    if (plan != NULL) {
        free_plan(plan);
        plan = NULL;
    }
    llist_iterate(work_files, free_file_spec);
    llist_clear(work_files);

    seen = hmap_new();
    sort_entries(operands, count);
    for (c = 0; c < count; c++) {
	if (strcmp(operands[c]->path, ".") != 0 && strcmp(operands[c]->path, "..") != 0)
	    add_spec(work_files, seen, operands[c]->path);
	free_entry(operands[c]);
    }
    free(operands);
    if (list_work_dir && !opts.directory)
	list_directory(".", work_files, seen);
    hmap_free(seen);

    return cwd_from_work_directory();
}

/* Create and initialize a FileSpec instance.
 * The returned FileSpec should be freed when
 * no longer used.
 */
FileSpec *
new_file_spec(void)
{
    FileSpec *spec;

    spec = xmalloc(sizeof(FileSpec));
    spec->old_name = NULL;
    spec->new_name = NULL;
    spec->status = STATUS_UNCHECKED;
    spec->prev_spec = NULL;
    spec->next_spec = NULL;

    return spec;
}

/* Free the specified FileSpec.
 */
void
free_file_spec(void *spec_ptr)
{
    FileSpec *spec = spec_ptr;

    if (spec->prev_spec != NULL)
    	spec->prev_spec->next_spec = spec->next_spec;
    if (spec->next_spec != NULL)
    	spec->next_spec->prev_spec = spec->prev_spec;
    free(spec->old_name);
    free(spec->new_name);
    free(spec);
}

void
dump_spec_list(LList *list)
{
    LListIterator it;

    puts("list begin");
    for (llist_iterator(list, &it); it.has_next(&it); ) {
        FileSpec *spec = it.next(&it);
        printf("list entry %p old=%s[%p] new=%s[%p]\n", spec, spec->old_name, spec->old_name, spec->new_name, spec->new_name);
    }
    puts("list end");
}
