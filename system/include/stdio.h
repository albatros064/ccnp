#ifndef CCNP_SYS_STDIO_H
#define CCNP_SYS_STDIO_H

#define EOF -1

typedef struct {
    char status;
    char *stream;
} FILE;

extern FILE *fopen (const char *__restrict __filename,
            const char *__restrict __modes) __wur;
extern int fclose (FILE *__stream);

#endif

