#ifndef __QXML_STREAM_H__
#define __QXML_STREAM_H__

#include <stdio.h>

typedef int QXML_StreamGetC(void* ptr);
typedef int QXML_StreamPutC(int c, void* ptr);
typedef void QXML_StreamRewind(void* ptr);
typedef int QXML_StreamClose(void* ptr);
typedef struct QXML_Stream
{
    QXML_StreamGetC* sgetc;
    QXML_StreamPutC* sputc;
    QXML_StreamRewind* srewind;
    QXML_StreamClose* sclose;
    void* ptr;
} QXML_Stream;

QXML_Stream* qxml_stream_create(QXML_StreamGetC* sgetc, QXML_StreamPutC* sputc, QXML_StreamRewind* srewind, QXML_StreamClose* sclose, void* ptr);
QXML_Stream* qxml_stream_create_fd(int fd, int close);
QXML_Stream* qxml_stream_create_file(FILE* file, int close);
QXML_Stream* qxml_stream_create_fname(const char* fname, const char* mode);
QXML_Stream* qxml_stream_create_strlen(char* str, size_t len, int copy);
QXML_Stream* qxml_stream_create_str(char* str, int copy);
void qxml_stream_destroy(QXML_Stream* stream);

#endif /* __QXML_STREAM_H__ */
