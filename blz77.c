#define _XOPEN_SOURCE 700
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "blz77_version.h"
#include "blz77.h"

#define errstr() (strerror (errno))

enum Mode {
    M_COMPRESS,
    M_DECOMPRESS,
    M_LIST,
};

static char *
make_ofname (const char *ifname,
             const char *suffix,
             bool        to_stdout)
{
    if (to_stdout || !strcmp (ifname, "-"))
        return strdup ("-");
    const size_t len_ifn = strlen (ifname), len_suf = strlen (suffix);
    const size_t len = len_ifn + len_suf + 1;
    char *str = malloc (len);
    snprintf (str, len, "%s%s", ifname, suffix);
    return str;
}

static FILE *
open_file (const char *filename, bool rw)
{
    return !strcmp (filename, "-") ? (rw ? stdout : stdin) : fopen (filename, rw ? "wb+" : "rb");
}

static void
close_file (FILE *file)
{
    if (file != NULL && file != stdin && file != stdout)
        fclose (file);
}

static bool
perform (enum Mode   mode,
         FILE       *input,
         FILE       *output,
         const char *input_name,
         const char *output_name,
         int         level)
{
    switch (mode) {
    case M_COMPRESS:
        if (blz77_compress (input, output, level) < 0) {
            fprintf (stderr, "blz77: Failed to compress '%s' to '%s': %s\n",
                     input_name, output_name, errstr());
            return false;
        }
        break;
    case M_DECOMPRESS:
        if (blz77_decompress (input, output) < 0) {
            fprintf (stderr, "blz77: Failed to decompress '%s' to '%s': %s\n",
                     input_name, output_name, errstr());
            return false;
        }
        return false;
    case M_LIST:
        // TODO: list
        return false;
    }
    return true;
}

int
main (int argc, char *argv[])
{
    const char *suffix = ".blz";
    int option, level;
    enum Mode mode = M_COMPRESS;
    bool success = true, keep = false, to_stdout = false;

    while ((option = getopt (argc, argv, ":cdhklLS:V0123456789")) != -1) {
        switch (option) {
        case 'c':
            to_stdout = true;
            keep = true;
            break;
        case 'd':
            mode = M_DECOMPRESS;
            break;
        case 'k':
            keep = true;
            break;
        case 'l':
            mode = M_LIST;
            break;
        case 'S':
            suffix = optarg;
            break;
        case '0' ... '9':
            level = option - '0';
            break;
        case 'h':
            puts (
                "Usage: blz77 [OPTION]... [FILE]...\n"
                "Compress or uncompress FILEs (by default compress FILES in-place).\n"
                "\n"
                "  -c            Write on standard output, keep original files unchanged.\n"
                "  -d            Decompress.\n"
                "  -h            Print this help page.\n"
                "  -k            Keep (don't delete) input files.\n"
                "  -l            List compressed file contents.\n"
                "  -L            Display software license.\n"
                "  -S SUF        Use suffix SUF on compressed files.\n"
                "  -V            Display version number.\n"
                "  -#            Select compression level preset (0..9).\n"
                "\n"
                "With no FILE, or when FILE is '-', read standard input.\n"
                "\n"
                "Report bugs to <benni@stuerz.xyz>."
            );
            return 0;
        case 'L':
        case 'V':
            puts (
                "blz77 " BLZ77_VERSION "\n"
                "Copyright (C) 2022 Benjamin Stürz.\n"
                "This is free software.  You may redistribute copies of it under the terms of\n"
                "the GNU General Public License <https://www.gnu.org/licenses/gpl.html>.\n"
                "There is NO WARRANTY, to the extent permitted by law."
            );
            if (option == 'V')
                puts ("\nWritten by Benjamin Stürz.");
            return 0;
        case '?':
            fprintf (stderr, "blz77: invalid option '-%c'.\n", optopt);
            goto syntax_err;
        case ':':
            fprintf (stderr, "blz77: option '-%c' requires an argument.\n", optopt);
        syntax_err:
            fputs ("Try `blz77 -h` for more information.\n", stderr);
            return 1;
        }
    }

    if ((argc - optind) == 0) {
        success = perform (mode, stdin, stdout, "stdin", "stdout", level);
    } else {
        for (; optind < argc; ++optind) {
            const char *input_name = argv[optind];
            char *output_name = NULL;
            FILE *input, *output = NULL;

            input = open_file (input_name, false);
            if (!input) {
                fprintf (stderr, "blz77: open('%s', 'rb'): %s\n", input_name, errstr());
                success = false;
                continue;
            }

            if (mode != M_LIST) {
                output_name = make_ofname (input_name, suffix, to_stdout);
                output = open_file (output_name, true);
                if (!output) {
                    fprintf (stderr, "blz77: open('%s', 'wb+'): %s\n", output_name, errstr());
                    success = false;
                    continue;
                }
            }

            success &= perform (mode, input, output, input_name, output_name, level);

            if (!keep)
                remove (input_name);

            close_file (input);
            close_file (output);
            free (output_name);
        }
    }
    return !success;
}
