/*
 * ftree.c
 * Copyright (C) 2018 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "util.h"

#include <archive.h>
#include <archive_entry.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// These are set as result from command line parsing.
static int opt_print_separator = '\n';

typedef struct archive ArchiveRead;
typedef struct archive_entry ArchiveEntry;

PTR_AUTO_DEFINE (ArchiveRead, archive_read_free)


enum {
    ENTRY_OK   = 0,
    ENTRY_SKIP = ENOENT,
};


typedef int (*entry_func) (struct archive_entry*, void*);


struct entry_action {
    const char *name;
    void       *data;
    entry_func  check;
    entry_func  action;
};


static int
entry_check_ok (struct archive_entry *e, void *d)
{
    (void) e;
    (void) d;
    return ENTRY_OK;
}


static int
entry_check_exists (struct archive_entry *e, void *d)
{
    (void) d;
    // TODO
    return ENTRY_OK;
}


static int
entry_print (struct archive_entry *e, void *d)
{
    (void) d;

    if (fputs (archive_entry_pathname (e), stdout) == EOF ||
        putchar (opt_print_separator) == EOF)
    {
        return errno;
    }
    return ENTRY_OK;
}


static int
entry_check (struct archive_entry *e, void *d)
{
    (void) d;

    // TODO

    return ENTRY_OK;
}


static int
entry_remove (struct archive_entry *e, void *d)
{
    (void) d;

    // TODO

    return ENTRY_OK;
}


enum action {
    ACTION_PRINT = 0,
    ACTION_CHECK,
    ACTION_REMOVE,

    ACTION_UNKNOWN, // Leave always as last
};

static const struct entry_action s_entry_actions[] = {
    [ACTION_PRINT] = {
        .name = "print",
        .check = entry_check_ok,
        .action = entry_print,
    },
    [ACTION_CHECK] = {
        .name = "check",
        .check = entry_check_exists,
        .action = entry_check,
    },
    [ACTION_REMOVE] = {
        .name = "remove",
        .check = entry_check_exists,
        .action = entry_remove,
    },
};


static int
entry_action_apply (const struct entry_action *ea, ArchiveEntry *e)
{
    int check = (*ea->check) (e, ea->data);
    switch (check) {
        case ENTRY_SKIP:
            return ENTRY_OK;
        case ENTRY_OK:
            return (*ea->action) (e, ea->data);
        default:
            return check;
    }
}


static inline _Bool
set_action (enum action *act, enum action newact)
{
    if (*act == ACTION_UNKNOWN) {
        *act = newact;
        return true;
    } else {
        return false;
    }
}


int
main (int argc, char **argv)
{
    enum action opt_action = ACTION_UNKNOWN;
    _Bool opt_verbose = false;

    ptr_auto(ArchiveRead) a = NULL;

    int opt;
    while ((opt = getopt (argc, argv, "lCR0v:h")) != -1) {
        switch (opt) {
            case '0':
                opt_print_separator = '\0';
                break;

            case 'l':
                if (!set_action (&opt_action, ACTION_PRINT))
                    goto getopt_action_err;
                break;

            case 'C':
                if (!set_action (&opt_action, ACTION_CHECK))
                    goto getopt_action_err;
                break;

            case 'R':
                if (!set_action (&opt_action, ACTION_REMOVE))
                    goto getopt_action_err;
                break;

            case 'v':
                opt_verbose = true;
                break;

            case 'h':
                fprintf (stderr, "Usage: %s [-lCR] [-0v] [-p path] < mtree\n", argv[0]);
                return EXIT_SUCCESS;

            default:
                return EXIT_FAILURE;
        }
    }

    if (opt_action == ACTION_UNKNOWN)
        opt_action = ACTION_PRINT;

    const struct entry_action *entry_actions[2] = {
        &s_entry_actions[opt_action],
        (opt_verbose && opt_action != ACTION_PRINT) ?
            &s_entry_actions[ACTION_PRINT] : NULL,
    };

    a = archive_read_new ();
    archive_read_support_format_mtree (a);

    if (archive_read_open_FILE (a, stdin) != ARCHIVE_OK)
        goto beach;

    ArchiveEntry *e = NULL;
    for (;;) {
        switch (archive_read_next_header (a, &e)) {
            case ARCHIVE_WARN:
                fprintf (stderr, "Warning: %s\n", archive_error_string (a));
                // fall-through
            case ARCHIVE_OK:
                for (unsigned i = 0; i < length_of (entry_actions); i++) {
                    if (entry_actions[i]) {
                        int ret = entry_action_apply (entry_actions[i], e);
                        if (ret) {
                            const char *errmsg = strerror ((ret < 0) ? -ret : ret);
                            const char *errlvl = (ret < 0) ? "error" : "warning";
                            fprintf (stderr, "%s: %s: %s\n", argv[0], errlvl, errmsg);
                            if (ret < 0) goto beach;
                        }
                    }
                }
                // fall-through
            case ARCHIVE_RETRY:
                break;

            case ARCHIVE_FATAL:
                goto beach;

            case ARCHIVE_EOF:
                goto end;
        }
    }
end:

    return EXIT_SUCCESS;

beach:
    fprintf (stderr, "%s: error: %s\n", argv[0], archive_error_string (a));
    return EXIT_FAILURE;

getopt_action_err:
    fprintf (stderr, "%s: Option '-%c' invalid in '%s' mode.\n", argv[0],
             opt, s_entry_actions[opt_action].name);
    return EXIT_FAILURE;
}
