/*
 * ltree.c
 * Copyright (C) 2018 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#define _GNU_SOURCE 1

#include "autocleanup/autocleanup.h"

#include <archive.h>
#include <archive_entry.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define length_of(_arr)  (sizeof (_arr) / sizeof ((_arr) [0]))

// These are set as result from command line parsing.
static int opt_prefix_fd = AT_FDCWD;
static int opt_print_separator = '\n';

typedef struct archive ArchiveRead;
typedef struct archive_entry ArchiveEntry;

PTR_AUTO_DEFINE (ArchiveRead, archive_read_free)

typedef int DirFd;
HANDLE_AUTO_DEFINE (DirFd, close, AT_FDCWD);


enum {
    ENTRY_OK = 0,
    ENTRY_SKIP,
    ENTRY_WARNING,
    ENTRY_ERROR,
};


typedef int (*entry_func) (struct archive_entry*, void*);


struct entry_action {
    const char *name;
    void       *data;
    entry_func  check;
    entry_func  action;
};


static int
entry_check_exists (struct archive_entry *e, void *d)
{
    (void) d;

    const char *e_path = archive_entry_pathname (e);

    struct stat sb;
    int ret = fstatat (opt_prefix_fd, e_path, &sb, AT_SYMLINK_NOFOLLOW);
    if (ret == 0)
        return ENTRY_OK;

    switch ((ret = errno)) {
        case ENOENT:
        case ENOTDIR:
            return ENTRY_SKIP;
        default:
            return -errno;
    }
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

    struct stat sb;
    const char *e_path = archive_entry_pathname (e);
    if (fstatat (opt_prefix_fd, e_path, &sb, AT_SYMLINK_NOFOLLOW) == -1) {
        int ret = errno;
        switch (ret) {
            case ENOENT:
            case ENOTDIR:
                fprintf (stderr, "%s: missing\n", e_path);
                return ENTRY_WARNING;
            default:
                return -errno;
        }
    }

    __auto_type e_sb = archive_entry_stat (e);
    __auto_type ret = ENTRY_OK;

    if (e_sb->st_mode != sb.st_mode) {
        // TODO: Print modes in a human-friendly format.
        fprintf (stderr, "%s: expected mode %#lx (got %#lx)\n",
                 e_path, (long) e_sb->st_mode, (long) sb.st_mode);
        ret = ENTRY_WARNING;
    }

    if (e_sb->st_uid != sb.st_uid) {
        fprintf (stderr, "%s: expected uid %zu (got %zu)\n",
                 e_path, (size_t) e_sb->st_uid, (size_t) sb.st_uid);
        ret = ENTRY_WARNING;
    }

    if (e_sb->st_gid != sb.st_gid) {
        fprintf (stderr, "%s: expected gid %zd (got %zd)\n",
                 e_path, (size_t) e_sb->st_gid, (size_t) sb.st_gid);
        ret = ENTRY_WARNING;
    }

    /*
     * TODO: There should there be some specific checks for device nodes.
     */
    if (!S_ISREG(e_sb->st_mode))
        return ret;

    if (e_sb->st_size != sb.st_size) {
        fprintf (stderr, "%s: expected size %zu (got %zu)\n",
                 e_path, e_sb->st_size, sb.st_size);
        ret = ENTRY_WARNING;
    }

    /*
     * TODO: Honor subsecond time resolutions.
     * Note that atime/ctime are not present in the mtree format.
     */
    if (e_sb->st_mtime != sb.st_mtime) {
        fprintf (stderr, "%s: expected mtime %zu (got %zu)\n",
                 e_path, e_sb->st_mtime, sb.st_mtime);
        ret = ENTRY_WARNING;
    }

    return ret;
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
        .action = entry_print,
    },
    [ACTION_CHECK] = {
        .name = "check",
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
    int check = (ea->check) ? (*ea->check) (e, ea->data) : ENTRY_OK;
    switch (check) {
        case ENTRY_SKIP:
            return ENTRY_OK;
        case ENTRY_OK:
            return (*ea->action) (e, ea->data);
        default:
            return check;
    }
}


static inline bool
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
    bool opt_verbose = false;
    char *opt_prefix = NULL;

    ptr_auto(ArchiveRead) a = NULL;
    handle_auto(DirFd) prefix_fd = AT_FDCWD;

    int opt;
    while ((opt = getopt (argc, argv, "lCR0vp:h")) != -1) {
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

            case 'p':
                opt_prefix = optarg;
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

    if (opt_action != ACTION_PRINT && opt_prefix) {
        opt_prefix_fd = prefix_fd = open (opt_prefix, O_PATH | O_DIRECTORY, 0);
        if (opt_prefix_fd < 0) {
            fprintf (stderr, "%s: error opening '%s': %s\n",
                     argv[0], opt_prefix, strerror (errno));
            return EXIT_FAILURE;
        }
    }

    const struct entry_action *entry_actions[2] = {
        &s_entry_actions[opt_action],
        (opt_verbose && opt_action != ACTION_PRINT) ?
            &s_entry_actions[ACTION_PRINT] : NULL,
    };

    a = archive_read_new ();
    archive_read_support_format_mtree (a);

    if (archive_read_open_FILE (a, stdin) != ARCHIVE_OK)
        goto beach;

    int exit_code = EXIT_SUCCESS;
    ArchiveEntry *e = NULL;

    for (;;) {
        switch (archive_read_next_header (a, &e)) {
            case ARCHIVE_WARN:
                fprintf (stderr, "Warning: %s\n", archive_error_string (a));
                exit_code = EXIT_FAILURE;
                // fall-through
            case ARCHIVE_OK:
                for (unsigned i = 0; i < length_of (entry_actions); i++) {
                    if (entry_actions[i]) {
                        int ret = entry_action_apply (entry_actions[i], e);
                        if (ret < 0) {
                            fprintf (stderr, "%s: error: %s\n", argv[0], strerror (-ret));
                            goto end;
                        } else if (ret == ENTRY_ERROR) {
                            ret = EXIT_FAILURE;
                            goto end;
                        } else if (ret != 0) {
                            /* Warning, operation may continue. */
                            ret = EXIT_FAILURE;
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
    return exit_code;

beach:
    fprintf (stderr, "%s: error: %s\n", argv[0], archive_error_string (a));
    return EXIT_FAILURE;

getopt_action_err:
    fprintf (stderr, "%s: Option '-%c' invalid in '%s' mode.\n", argv[0],
             opt, s_entry_actions[opt_action].name);
    return EXIT_FAILURE;
}
