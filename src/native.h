#ifndef BIOFLOW_NATIVE_H
#define BIOFLOW_NATIVE_H

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BF_MAX_BYTES ((size_t)128 * 1024 * 1024)
#define BF_MAX_LINE ((size_t)1024 * 1024)
#define BF_MAX_ROWS 1000000
#define BF_MAX_COLS 256

typedef char bf_check_int[(INT_MAX == 2147483647) ? 1 : -1];
typedef char bf_check_double[
    (DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024) ? 1 : -1
];

typedef struct {
    char *text;
    char **cells;
    int width;
    int line;
} BfRow;

typedef struct {
    BfRow *rows;
    int count;
    int capacity;
} BfTable;

static BfTable bf_tables[2];
static FILE *bf_outputs[6];

static const char *bf_output_names[6] = {
    "diagnostics.tsv",
    "annotation_evidence.tsv",
    "protein_assignments.tsv",
    "gene_family_counts.tsv",
    "normalized_features.tsv",
    "qc.tsv"
};

static const char *bf_input_names[2] = {
    "species_metadata.tsv", "protein_annotations.tsv"
};

static void bf_clear_table(int slot) {
    BfTable *t = &bf_tables[slot];
    for (int i = 0; i < t->count; ++i) {
        free(t->rows[i].cells);
        free(t->rows[i].text);
    }
    free(t->rows);
    memset(t, 0, sizeof(*t));
}

static void bf_free_inputs(void) {
    bf_clear_table(0);
    bf_clear_table(1);
}

static int bf_utf8(const unsigned char *s, size_t n) {
    size_t i = 0;
    while (i < n) {
        unsigned char c = s[i++];
        if (c < 128) continue;

        int need;
        uint32_t cp, minimum;
        if (c >= 0xC2 && c <= 0xDF) {
            need = 1; cp = c & 0x1F; minimum = 0x80;
        } else if (c >= 0xE0 && c <= 0xEF) {
            need = 2; cp = c & 0x0F; minimum = 0x800;
        } else if (c >= 0xF0 && c <= 0xF4) {
            need = 3; cp = c & 0x07; minimum = 0x10000;
        } else {
            return 0;
        }

        if ((size_t)need > n - i) return 0;
        while (need--) {
            unsigned char d = s[i++];
            if ((d & 0xC0) != 0x80) return 0;
            cp = (cp << 6) | (d & 0x3F);
        }
        if (cp < minimum || cp > 0x10FFFF) return 0;
        if (cp >= 0xD800 && cp <= 0xDFFF) return 0;
    }
    return 1;
}

static int bf_load(int slot, char *path) {
    BfTable *t = &bf_tables[slot];
    FILE *f = NULL;
    char *line = NULL;
    size_t capacity = 0, total = 0;
    int physical_line = 0;
    const char *reason = "I/O or allocation failure";
    struct stat st;

    bf_clear_table(slot);
    f = fopen(path, "rb");
    if (!f) goto fail;

    if (fstat(fileno(f), &st) != 0 || !S_ISREG(st.st_mode)) {
        reason = "input must be a regular file";
        goto fail;
    }
    if (st.st_size < 0 || (uintmax_t)st.st_size > BF_MAX_BYTES) {
        reason = "input exceeds 128 MiB limit";
        goto fail;
    }

    ssize_t length;
    while ((length = getline(&line, &capacity, f)) >= 0) {
        ++physical_line;
        total += (size_t)length;
        if (total > BF_MAX_BYTES || (size_t)length > BF_MAX_LINE) {
            reason = "file or line resource limit exceeded";
            goto fail;
        }
        if (memchr(line, '\0', (size_t)length)) {
            reason = "embedded NUL byte";
            goto fail;
        }

        size_t n = (size_t)length;
        if (n && line[n - 1] == '\n') --n;
        if (n && line[n - 1] == '\r') --n;
        line[n] = '\0';

        if (!bf_utf8((unsigned char *)line, n)) {
            reason = "invalid UTF-8";
            goto fail;
        }
        if (physical_line == 1 && n >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            reason = "UTF-8 BOM is unsupported";
            goto fail;
        }

        int width = 1;
        for (size_t i = 0; i < n; ++i) {
            unsigned char c = (unsigned char)line[i];
            if ((c < 32 && c != '\t') || c == 127) {
                reason = "embedded control character";
                goto fail;
            }
            if (c == '\t') ++width;
        }
        if (width > BF_MAX_COLS || t->count >= BF_MAX_ROWS + 1) {
            reason = "column or record limit exceeded";
            goto fail;
        }

        if (t->count == t->capacity) {
            int next = t->capacity ? t->capacity * 2 : 64;
            BfRow *grown = realloc(t->rows, (size_t)next * sizeof(*grown));
            if (!grown) goto fail;
            t->rows = grown;
            t->capacity = next;
        }

        char *copy = strdup(line);
        char **cells = malloc((size_t)width * sizeof(*cells));
        if (!copy || !cells) {
            free(copy); free(cells);
            goto fail;
        }

        cells[0] = copy;
        int column = 1;
        for (size_t i = 0; i < n; ++i) {
            if (copy[i] == '\t') {
                copy[i] = '\0';
                cells[column++] = copy + i + 1;
            }
        }

        if (t->count == 0) {
            for (int i = 0; i < width; ++i) {
                if (!cells[i][0]) {
                    free(copy); free(cells);
                    reason = "empty header name";
                    goto fail;
                }
                for (int j = 0; j < i; ++j) {
                    if (strcmp(cells[i], cells[j]) == 0) {
                        free(copy); free(cells);
                        reason = "duplicate header name";
                        goto fail;
                    }
                }
            }
        }

        t->rows[t->count++] = (BfRow){copy, cells, width, physical_line};
    }

    if (ferror(f) || !feof(f)) goto fail;
    {
        int closed = fclose(f);
        f = NULL;
        if (closed != 0) goto fail;
    }
    free(line);
    if (!t->count) {
        line = NULL;
        reason = "empty file";
        goto fail;
    }
    return 0;

fail:
    fprintf(stderr, "%s:%d: %s\n", path, physical_line, reason);
    if (f) fclose(f);
    free(line);
    bf_clear_table(slot);
    return -1;
}

static int bf_rows(int slot) {
    return bf_tables[slot].count - 1;
}

static int bf_width(int slot, int row) {
    return bf_tables[slot].rows[row].width;
}

static int bf_line(int slot, int row) {
    return bf_tables[slot].rows[row].line;
}

static char *bf_get(int slot, int row, int column) {
    BfRow *r = &bf_tables[slot].rows[row];
    return column >= 0 && column < r->width ? r->cells[column] : "";
}

static int bf_column(int slot, char *name) {
    BfRow *header = &bf_tables[slot].rows[0];
    for (int i = 0; i < header->width; ++i)
        if (strcmp(header->cells[i], name) == 0) return i;
    fprintf(stderr, "%s: missing required column: %s\n",
            bf_input_names[slot], name);
    return -1;
}

/* Sort records by requested keys, then by all columns and original line. */
static int bf_sort_keys[BF_MAX_COLS];
static int bf_sort_key_count;

static int bf_compare_rows(const void *left, const void *right) {
    const BfRow *a = left, *b = right;
    for (int i = 0; i < bf_sort_key_count; ++i) {
        int k = bf_sort_keys[i];
        const char *x = k < a->width ? a->cells[k] : "";
        const char *y = k < b->width ? b->cells[k] : "";
        int c = strcmp(x, y);
        if (c) return c;
    }
    if (a->width != b->width)
        return (a->width > b->width) - (a->width < b->width);
    return (a->line > b->line) - (a->line < b->line);
}

static int bf_sort(int slot, int *keys, int count) {
    int width = bf_width(slot, 0);
    bf_sort_key_count = 0;
    for (int i = 0; i < count; ++i) {
        if (keys[i] < 0 || keys[i] >= width) return -1;
        bf_sort_keys[bf_sort_key_count++] = keys[i];
    }
    for (int column = 0; column < width; ++column) {
        int present = 0;
        for (int i = 0; i < count; ++i)
            if (keys[i] == column) present = 1;
        if (!present) bf_sort_keys[bf_sort_key_count++] = column;
    }
    qsort(bf_tables[slot].rows + 1, (size_t)bf_rows(slot),
          sizeof(BfRow), bf_compare_rows);
    return 0;
}

static int bf_same(int slot, int first, int second) {
    BfRow *a = &bf_tables[slot].rows[first];
    BfRow *b = &bf_tables[slot].rows[second];
    if (a->width != b->width) return 0;
    for (int i = 0; i < a->width; ++i)
        if (strcmp(a->cells[i], b->cells[i]) != 0) return 0;
    return 1;
}

/* Return 0 for missing, 1 for valid, -1 for invalid. */
static int bf_integer(char *text, int *value) {
    *value = 0;
    if (!text[0]) return 0;
    int result = 0;
    for (const unsigned char *p = (unsigned char *)text; *p; ++p) {
        if (*p < '0' || *p > '9') return -1;
        int digit = *p - '0';
        if (result > (INT_MAX - digit) / 10) return -1;
        result = result * 10 + digit;
    }
    *value = result;
    return 1;
}

static int bf_decimal(char *text, double *value) {
    *value = 0.0;
    if (!text[0]) return 0;
    for (char *p = text; *p; ++p)
        if (!strchr("0123456789.eE+-", *p)) return -1;

    char *end;
    double result = strtod(text, &end);
    if (end == text || *end || !isfinite(result)) return -1;
    if (result != 0.0 && fabs(result) < DBL_MIN) return -1;

    if (result == 0.0) {
        for (char *p = text; *p && *p != 'e' && *p != 'E'; ++p)
            if (*p >= '1' && *p <= '9') return -1;
    }
    *value = result;
    return 1;
}

static int bf_open_outputs(void) {
    for (int i = 0; i < 6; ++i) {
        bf_outputs[i] = fopen(bf_output_names[i], "wx");
        if (!bf_outputs[i]) {
            fprintf(stderr, "Cannot create %s: %s\n",
                    bf_output_names[i], strerror(errno));
            return -1;
        }
    }
    return 0;
}

static FILE *bf_out(int index) {
    return bf_outputs[index];
}

static void bf_fields(FILE *out, int slot, int row, int *columns, int count) {
    for (int i = 0; i < count; ++i) {
        if (i) fputc('\t', out);
        fputs(bf_get(slot, row, columns[i]), out);
    }
}

static void bf_record_json(FILE *out, int slot, int row) {
    BfRow *r = &bf_tables[slot].rows[row];
    fputc('[', out);
    for (int i = 0; i < r->width; ++i) {
        if (i) fputc(',', out);
        fputc('"', out);
        for (unsigned char *p = (unsigned char *)r->cells[i]; *p; ++p) {
            if (*p == '"' || *p == '\\') fputc('\\', out);
            fputc(*p, out);
        }
        fputc('"', out);
    }
    fputc(']', out);
}

static void bf_reject(int slot, int row, char *reason) {
    FILE *out = bf_out(0);
    fprintf(out, "%s\t%d\t%s\t",
            bf_input_names[slot], bf_line(slot, row), reason);
    bf_record_json(out, slot, row);
    fputc('\n', out);
}

static int bf_finish_outputs(void) {
    int failed = 0;
    for (int i = 0; i < 6; ++i) {
        if (bf_outputs[i]) {
            if (ferror(bf_outputs[i])) failed = 1;
            if (fclose(bf_outputs[i]) != 0) failed = 1;
            bf_outputs[i] = NULL;
        }
    }
    return failed ? -1 : 0;
}

#endif