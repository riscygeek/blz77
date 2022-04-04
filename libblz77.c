#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "blz77_version.h"
#include "blz77.h"

#define my_min(x, y) ((x) < (y) ? (x) : (y))
#define my_max(x, y) ((x) > (y) ? (x) : (y))
#define my_clamp(x, mn, mx) (my_min(my_max(x, mn), mx))

static const char magic[6] = { 0x7f, 'B', 'L', 'Z', '7', '7' };

int
blz77_read_header (FILE                *file,
                   struct blz77_header *hdr)
{
    struct blz77_header tmp;
    if (fread (&tmp, sizeof (struct blz77_header), 1, file) != sizeof (struct blz77_header))
        return -1;
    if (memcmp (tmp.magic, magic, 6) != 0 || tmp.version != BLZ77_FMT_VERSION) {
        errno = EINVAL;
        return -1;
    }
    if (hdr)
        *hdr = tmp;
    return 0;
}

static void setup_hdr (struct blz77_header *hdr)
{
    memset (hdr, 0, sizeof (struct blz77_header));
    memcpy (hdr->magic, magic, sizeof (magic));
    hdr->version  = BLZ77_FMT_VERSION;
}

static void setup_lens (FILE  *input,
                        int     level,
                        size_t *len_sb,
                        size_t *len_lb,
                        size_t *min_n)
{
    static const struct preset {
        size_t len_sb_divisor;
        size_t len_sb;
        size_t len_lb;
        size_t min_n;
    } table [10] = {
        [0] = {  0,  4,  3, 4 },   //   16,    8
        [1] = {  0,  5,  4, 4 },   //   32,   16
        [2] = {  0,  6,  5, 5 },   //   64,   32
        [3] = {  0,  8,  7, 5 },   //  256,  128
        [4] = {  0, 10,  9, 5 },   //   1K,  512
        [5] = {  0, 12, 10, 5 },   //   4K,   1K
        [6] = { 10, 14, 12, 5 },   //  16K,   4K
        [7] = {  8, 16, 15, 5 },   //  64K,  32K
        [8] = {  4, 18, 17, 6 },   // 256K, 128K
        [9] = {  2, 20, 19, 6 },   //   1M, 512K
    };
    const struct preset p = table [my_clamp (level, 0, 10)];
    struct stat st;
    int fd;

    *len_lb = 1 << p.len_lb;
    *min_n = p.min_n;

    if (p.len_sb_divisor
    && (fd = fileno (input)) >= 0
    && fstat (fd, &st) == 0) {
        *len_sb = st.st_size / p.len_sb_divisor;
    } else {
        *len_sb = 1 << p.len_sb;
    }
}

int blz77_compress (FILE *input,
                    FILE *output,
                    int level)
{
    struct blz77_header hdr;
    char *sbuf, *lbuf;
    size_t cap_sb, cap_lb, len_sb = 0, len_lb, min_n;
    int ch, ec = 0, saved_errno;

    setup_hdr (&hdr);
    setup_lens (input,
                level,
                &cap_sb,
                &cap_lb,
                &min_n);
    hdr.cap_sb = cap_sb;

    sbuf = malloc (cap_sb);
    if (!sbuf)
        return -1;

    lbuf = malloc (cap_lb);
    if (!lbuf) {
        saved_errno = errno;
        free (lbuf);
        errno = saved_errno;
        return -1;
    }

    if (fwrite (&hdr, sizeof (struct blz77_header), 1, output) != 1) {
        ec = -1;
        goto ret;
    }

    len_lb = fread (lbuf, 1, cap_lb, input);

    while (len_lb > 0) {
        int d = 1, l = 0;

        // TODO: Optimize this triple-loop
        for (size_t n = len_lb - 1; n > (min_n - 1); --n) {
            for (size_t i = cap_sb - len_sb; i < cap_sb; ++i) {
                for (size_t j = 0; j < n; ++j) {
                    if (sbuf [i + j] != lbuf [j])
                        goto failed;
                }
                d = cap_sb - i;
                l = n;
                goto success;
failed:;
            }
        }
success:
        if (l == 0) {
            ch = lbuf [0];
            for (int i = (ch == '%') + 1; i != 0; --i) {
                if (fputc (ch, output) == EOF) {
                    ec = -1;
                    goto ret;
                }
            }
            l = 1;
        } else {
            if (fprintf (output, "%%%d,%d", d, l) < 0) {
                ec = -1;
                goto ret;
            }
        }

        memmove (sbuf, sbuf + l, cap_sb - l);
        memcpy (sbuf + cap_sb - l, lbuf, l);
        memmove (lbuf, lbuf + l, cap_lb - l);

        len_lb = len_lb - l + fread (lbuf + cap_lb - l, 1, l, input);
        len_sb = my_min (len_sb + l, cap_sb);
    }

ret:
    saved_errno = errno;
    free (sbuf);
    free (lbuf);
    errno = saved_errno;
    return ec;
}

static bool
append (FILE   *file,
        char   *sbuf,
        char   *lbuf,
        size_t  cap_sb,
        size_t *len_sb,
        size_t  len_lb)
{
    const size_t remain = cap_sb - *len_sb;
    char *end = sbuf + cap_sb;
    if (remain > len_lb) {
        memmove (end - *len_sb - len_lb, end - *len_sb, *len_sb);
        *len_sb += len_lb;
    } else if (remain > 0) {
        memmove (sbuf, sbuf + remain, *len_sb);
        *len_sb += remain;
    } else {
        memmove (sbuf, sbuf + len_lb, cap_sb - len_lb);
    }
    memcpy (end - len_lb, lbuf, len_lb);

    return fwrite (lbuf, 1, len_lb, file) == len_lb;
}

int blz77_decompress (FILE *input,
                      FILE *output)
{
    struct blz77_header hdr;
    char *sbuf, *lbuf;
    int ec = 0, ch, d, l, saved_errno;
    size_t len_sb = 0;

    if (fread (&hdr, sizeof (struct blz77_header), 1, input) != 1)
        return -1;

    sbuf = malloc (hdr.cap_sb);
    if (!sbuf)
        return -1;

    lbuf = malloc (hdr.cap_sb);
    if (!lbuf) {
        saved_errno = errno;
        free (sbuf);
        errno = saved_errno;
        return -1;
    }

    while ((ch = fgetc (input)) != EOF) {
        if (ch == '%') {
            ch = fgetc (input);
            if (ch == EOF) {
                errno = EINVAL;
                ec = -1;
                goto ret;
            } else if (ch != '%') {
                ungetc (ch, input);
                if (fscanf (input, "%d,%d", &d, &l) != 2) {
                    ec = -1;
                    goto ret;
                }


                memcpy (lbuf, sbuf + hdr.cap_sb - d, l);
                if (!append (output, sbuf, lbuf, hdr.cap_sb, &len_sb, l)) {
                    ec = -1;
                    goto ret;
                }
                continue;
            }
        }
        lbuf [0] = ch;
        if (!append (output, sbuf, lbuf, hdr.cap_sb, &len_sb, 1)) {
            ec = -1;
            goto ret;
        }
    }

ret:
    saved_errno = errno;
    free (sbuf);
    free (lbuf);
    errno = saved_errno;
    return ec;
}
