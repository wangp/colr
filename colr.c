// colr - colour lines based on regular expression
// Copyright (C) 2017 Peter Wang

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <regex.h>

#define PROG "colr"

static void
die(const char *message)
{
    fprintf(stderr, "%s: %s\n", PROG, message);
    exit(EXIT_FAILURE);
}

static void
die2(const char *message, const char *detail)
{
    fprintf(stderr, "%s: %s: %s\n", PROG, message, detail);
    exit(EXIT_FAILURE);
}

static void
checked_fputc(int c, FILE *fp)
{
    if (fputc(c, fp) == EOF) {
        perror(PROG ": write error");
        exit(EXIT_FAILURE);
    }
}

static void
checked_fputs(const char *s, FILE *fp)
{
    if (fputs(s, fp) == EOF) {
        perror(PROG ": write error");
        exit(EXIT_FAILURE);
    }
}

static size_t
nextpowerof2(size_t x)
{
    size_t y = 1;

    while (y < x) {
        size_t y2 = y * 2;
        if (y2 < y) {
            die("integer overflow\n");
        }
        y = y2;
    }

    return y;
}

/*-------------------------------------------------------------------------*/

typedef struct Buffer Buffer;

struct Buffer {
    char    *buffer;
    size_t  capacity;
    size_t  size;
};

typedef struct Slice Slice;

struct Slice {
    char    *buffer;
    size_t  size;
};

static void
buffer_init(Buffer *buf)
{
    buf->buffer = NULL;
    buf->capacity = 0;
    buf->size = 0;
}

static void
buffer_free(Buffer *buf)
{
    free(buf->buffer);
    buffer_init(buf);
}

static void
buffer_reset(Buffer *buf)
{
    buf->size = 0;
}

static void
buffer_ensure_space(Buffer *buf, size_t extra)
{
    size_t new_size = buf->size + extra;
    if (new_size > buf->capacity) {
        size_t new_cap = nextpowerof2(new_size);
        buf->buffer = realloc(buf->buffer, new_cap);
        if (buf->buffer == NULL) {
            die("out of memory");
        }
        buf->capacity = new_cap;
    }
}

static void
buffer_put(Buffer *buf, char c)
{
    buffer_ensure_space(buf, 1);
    buf->buffer[buf->size] = c;
    buf->size++;
}

/*-------------------------------------------------------------------------*/

#define MAX_MATCH       32
#define COLOR_NORMAL    -1
#define COLOR_UNKNOWN   -2

static const char *
color_sequence(int i)
{
    const char *normal = "\x1B[0m";
    const char *bold[6] = {
        "\x1B[31;01m",  // red
        "\x1B[32;01m",  // green
        "\x1B[33;01m",  // yellow
        "\x1B[34;01m",  // blue
        "\x1B[35;01m",  // magenta
        "\x1B[36;01m"   // cyan
    };

    if (i < 0) {
        return normal;
    } else {
        return bold[i % 6];
    }
}

static void
read_line(FILE *stream, Buffer *buf)
{
    for (;;) {
        int c = fgetc(stream);
        if (c == EOF) {
            if (ferror(stream)) {
                die2("read error", strerror(errno));
            }
            break;
        }
        buffer_put(buf, c);
        // Treat NUL as eol terminator as POSIX regexp routines
        // cannot deal with internal NULs.
        if (c == '\n' || c == '\0') {
            break;
        }
    }
}

static char     *color_buf = NULL;
static size_t   color_buf_length = 0;

static void
print_matched(const Slice *slice, regmatch_t matches[MAX_MATCH], FILE *fp,
    int *cur_color)
{
    size_t      i;
    int         m;
    regoff_t    off;

    if (color_buf_length < slice->size) {
        color_buf_length = slice->size;
        color_buf = realloc(color_buf, sizeof(char) * color_buf_length);
        if (color_buf == NULL) {
            die("out of memory");
        }
    }

    for (i = 0; i < slice->size; i++) {
        color_buf[i] = COLOR_NORMAL;
    }

    for (m = 0; m < MAX_MATCH; m++) {
        if (matches[m].rm_so != -1) {
            for (off = matches[m].rm_so; off < matches[m].rm_eo; off++) {
                color_buf[off] = m;
            }
        }
    }

    for (i = 0; i < slice->size; i++) {
        if (*cur_color != color_buf[i]) {
            *cur_color = color_buf[i];
            checked_fputs(color_sequence(*cur_color), fp);
        }
        checked_fputc(slice->buffer[i], fp);
    }
}

static void
print_unmatched(const Slice *slice, FILE *fp, int *cur_color)
{
    size_t i;

    if (*cur_color != COLOR_NORMAL) {
        *cur_color = COLOR_NORMAL;
        checked_fputs(color_sequence(*cur_color), fp);
    }

    for (i = 0; i < slice->size; i++) {
        checked_fputc(slice->buffer[i], fp);
    }
}

static void
highlight_line(const regex_t *reg, const Buffer *buf, FILE *fp, int *cur_color)
{
    size_t  offset = 0;

    while (offset < buf->size) {
        regmatch_t  matches[MAX_MATCH];
        Slice       slice;
        int         eflags;
        int         rc;

        slice.buffer = buf->buffer + offset;
        slice.size = buf->size - offset;

        eflags = (offset == 0) ? 0 : REG_NOTBOL;
        rc = regexec(reg, slice.buffer, MAX_MATCH, matches, eflags);
        if (rc == 0 && matches[0].rm_eo > 0) {
            slice.size = matches[0].rm_eo;
            print_matched(&slice, matches, fp, cur_color);
        } else {
            if (rc == 0 && matches[0].rm_eo == 0) {
                // Ouch, zero-length match. Ensure progress.
                slice.size = 1;
            }
            print_unmatched(&slice, fp, cur_color);
        }

        offset += slice.size;
    }
}

static void
highlight(const regex_t *reg, FILE *inp, FILE *out)
{
    Buffer  buf;
    char    c;
    int     terminator;
    int     cur_color;

    buffer_init(&buf);

    while (!feof(inp)) {
        buffer_reset(&buf);
        read_line(inp, &buf);

        if (buf.size == 0) {
            break;
        }

        c = buf.buffer[buf.size - 1];
        if (c == '\n' || c == '\0') {
            terminator = c;
            buf.size--;
        } else {
            terminator = -1;
        }

        /* Ensure buffer is NUL terminated (but not counted in its size). */
        buffer_put(&buf, '\0');
        buf.size--;

        cur_color = COLOR_UNKNOWN;
        highlight_line(reg, &buf, out, &cur_color);

        if (terminator >= 0) {
            if (cur_color != COLOR_NORMAL) {
                checked_fputs(color_sequence(COLOR_NORMAL), out);
            }
            checked_fputc(terminator, out);
        }
    }

    buffer_free(&buf);
}

int
main(int argc, char **argv)
{
    regex_t reg;
    int     reg_index;
    int     cflags;
    int     rc;
    char    errbuf[128];
    int     c;

    cflags = REG_EXTENDED | REG_NEWLINE;

    while ((c = getopt(argc, argv, "i")) != -1) {
        switch (c) {
            case 'i':
                cflags |= REG_ICASE;
                break;
            case '?':
                die("unrecognised option");
                break;
        }
    }

    reg_index = optind;
    if (reg_index != argc - 1) {
        die("wrong number of arguments");
    }

    rc = regcomp(&reg, argv[reg_index], cflags);
    if (rc != 0) {
        regerror(rc, &reg, errbuf, sizeof(errbuf));
        die(errbuf);
    }

    highlight(&reg, stdin, stdout);

    return 0;
}

/*-------------------------------------------------------------------------*/
