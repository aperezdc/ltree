/*
 * ftree.c
 * Copyright (C) 2018 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <archive.h>
#include <archive_entry.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>


static int s_print_entry_separator = '\n';

static void
print_entry (struct archive_entry *e)
{
    fputs (archive_entry_pathname (e), stdout);
    putchar (s_print_entry_separator);
}


int
main (int argc, char **argv)
{
    void (*process_entry)(struct archive_entry*) = print_entry;

    int opt;
    while ((opt = getopt (argc, argv, "0h")) != -1) {
        switch (opt) {
            case '0':
                s_print_entry_separator = '\0';
                break;
            case 'h':
                fprintf (stderr, "Usage: %s [-0] < mtree\n", argv[0]);
                return EXIT_SUCCESS;
            default:
                return EXIT_FAILURE;
        }
    }

    struct archive *a = archive_read_new ();
    archive_read_support_format_mtree (a);

    if (archive_read_open_FILE (a, stdin) != ARCHIVE_OK)
        goto beach;

    struct archive_entry *e = NULL;
    for (;;) {
        switch (archive_read_next_header (a, &e)) {
            case ARCHIVE_WARN:
                fprintf (stderr, "Warning: %s\n", archive_error_string (a));
                // fall-through
            case ARCHIVE_OK:
                (*process_entry) (e);
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
    fprintf (stderr, "Error: %s\n", archive_error_string (a));
    return EXIT_FAILURE;
}
