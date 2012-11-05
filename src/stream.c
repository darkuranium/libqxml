#include <qxml/stream.h>

#include <stdlib.h>
#include <string.h>

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/* to avoid any possible deprecation warnings */
#define read(fd, c, n)  _read(fd, c, n)
#define write(fd, c, n) _write(fd, c, n)
#define lseek(fd, o, w) _lseek(fd, o, w)
#define close(fd)       _close(fd)
#else
#include <unistd.h>
#endif /* __WIN32__ or not */

static int sgetc_fd(int* fdp)
{
    char c;
    ssize_t sz = read(*fdp, &c, 1);
    if(sz < 1)
        return EOF;
    return c;
}
static int sputc_fd(int c, int* fdp)
{
    ssize_t sz = write(*fdp, &c, 1);
    if(sz < 1)
        return EOF;
    return c;
}
static void srewind_fd(int* fdp)
{
    lseek(*fdp, 0, SEEK_SET);
}
static int sclose_fd(int* fdp)
{
    int res = close(*fdp);
    free(fdp);
    return res ? EOF : 0;
}
static int snoclose_fd(int* fdp)
{
    free(fdp);
    return 0;
}

typedef struct QXML_Buffer
{
    size_t mem;
    size_t len;
    char* ptr;
    size_t pos;
    int copy;
} QXML_Buffer;
QXML_Buffer* qxml_buffer_create(char* ptr, size_t len, int copy)
{
    QXML_Buffer* buf = malloc(sizeof(QXML_Buffer));
    if(!buf) return NULL;

    buf->mem = len + 1;
    buf->len = len;
    buf->pos = 0;
    buf->copy = copy;

    if(!copy)
        buf->ptr = ptr;
    else
    {
        buf->ptr = malloc(buf->mem);
        memcpy(buf->ptr, ptr, len);
        buf->ptr[len] = 0;
    }

    return buf;
}
void qxml_buffer_destroy(QXML_Buffer* buf)
{
    if(!buf) return;
    if(!buf->copy)
        free(buf->ptr);
    free(buf);
}

static int sgetc_str(QXML_Buffer* buf)
{
    if(buf->pos == buf->len)
        return EOF;
    return buf->ptr[buf->pos++];
}
static int sputc_str(int c, QXML_Buffer* buf)
{
    if(buf->len + 2 > buf->mem)
    {
        if(!buf->copy)
            return EOF;
        buf->mem <<= 1;
        buf->ptr = realloc(buf->ptr, buf->mem);
    }
    buf->ptr[buf->len++] = c;
    buf->ptr[buf->len] = 0;
    return c;
}
static void srewind_str(QXML_Buffer* buf)
{
    buf->pos = 0;
}
static int sclose_str(QXML_Buffer* buf)
{
    if(!buf)
        return EOF;
    qxml_buffer_destroy(buf);
    return 0;
}

QXML_Stream* qxml_stream_create(QXML_StreamGetC* sgetc, QXML_StreamPutC* sputc, QXML_StreamRewind* srewind, QXML_StreamClose* sclose, void* ptr)
{
    QXML_Stream* stream = malloc(sizeof(QXML_Stream));
    if(!stream) return NULL;

    stream->sgetc = sgetc;
    stream->sputc = sputc;
    stream->srewind = srewind;
    stream->sclose = sclose;
    stream->ptr = ptr;

    return stream;
}
QXML_Stream* qxml_stream_create_fd(int fd, int close)
{
    QXML_Stream* stream;

    int* fdp = malloc(sizeof(int));
    if(!fdp) return NULL;
    *fdp = fd;

    stream = qxml_stream_create((QXML_StreamGetC*)sgetc_fd, (QXML_StreamPutC*)sputc_fd, (QXML_StreamRewind*)srewind_fd, close ? (QXML_StreamClose*)sclose_fd : (QXML_StreamClose*)snoclose_fd, fdp);
    if(!stream) free(fdp);
    return stream;
}
QXML_Stream* qxml_stream_create_file(FILE* file, int close)
{
    return qxml_stream_create((QXML_StreamGetC*)fgetc, (QXML_StreamPutC*)fputc, (QXML_StreamRewind*)rewind, close ? (QXML_StreamClose*)fclose : NULL, file);
}
QXML_Stream* qxml_stream_create_fname(const char* fname, const char* mode)
{
    FILE* file = fopen(fname, mode);
    if(!file) return NULL;
    return qxml_stream_create_file(file, 1);
}
QXML_Stream* qxml_stream_create_strlen(char* str, size_t len, int copy)
{
    QXML_Stream* stream;

    QXML_Buffer* buf = qxml_buffer_create(str, len, copy);
    if(!buf) return NULL;

    stream = qxml_stream_create((QXML_StreamGetC*)sgetc_str, (QXML_StreamPutC*)sputc_str, (QXML_StreamRewind*)srewind_str, (QXML_StreamClose*)sclose_str, buf);
    if(!stream) qxml_buffer_destroy(buf);
    return stream;
}
QXML_Stream* qxml_stream_create_str(char* str, int copy)
{
    return qxml_stream_create_strlen(str, strlen(str), copy);
}
void qxml_stream_destroy(QXML_Stream* stream)
{
    if(!stream) return;
    if(stream->sclose)
        stream->sclose(stream->ptr);
    free(stream);
}
