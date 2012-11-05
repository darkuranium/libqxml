#ifndef __QXML_FILE_H__
#define __QXML_FILE_H__

#include "stream.h"

struct QXML_File;

typedef void (QXML_CB_Comment)(struct QXML_File* xml, const char* text, size_t len);
typedef void (QXML_CB_CData)(struct QXML_File* xml, const char* cdata, size_t len);
typedef void (QXML_CB_ElemBegin)(struct QXML_File* xml, const char* name, size_t len);
typedef void (QXML_CB_ElemEnd)(struct QXML_File* xml, const char* name, size_t len);
typedef void (QXML_CB_ElemAttrib)(struct QXML_File* xml, const char* key, size_t klen, const char* val, size_t vlen);
typedef void (QXML_CB_XMLDecl)(struct QXML_File* xml, const char* decl, size_t len);
typedef void (QXML_CB_Text)(struct QXML_File* xml, const char* text, size_t len);
typedef void (QXML_CB_Whitespace)(struct QXML_File* xml, const char* ws, size_t len);
typedef void (QXML_CB_Error)(struct QXML_File* xml, size_t offset, size_t line, size_t col, const char* msg, size_t len);

typedef struct QXML_File
{
    QXML_Stream* stream;
    int delstream;

    void* data;
    int stop;

    QXML_CB_Comment* cb_comment;
    QXML_CB_CData* cb_cdata;
    QXML_CB_ElemBegin* cb_elem_begin;
    QXML_CB_ElemEnd* cb_elem_end;
    QXML_CB_ElemAttrib* cb_elem_attrib;
    QXML_CB_XMLDecl* cb_xml_decl;
    QXML_CB_Text* cb_text;
    QXML_CB_Whitespace* cb_whitespace;

    QXML_CB_Error* cb_error;
} QXML_File;

QXML_File* qxml_file_create(QXML_Stream* stream, int del);
QXML_File* qxml_file_create_fd(int fd, int close);
QXML_File* qxml_file_create_file(FILE* file, int close);
QXML_File* qxml_file_create_fname(const char* fname);
QXML_File* qxml_file_create_strlen(const char* str, size_t len, int copy);
QXML_File* qxml_file_create_str(const char* str, int copy);

void qxml_file_destroy(QXML_File* xml);
void qxml_file_rewind(QXML_File* xml);
void qxml_file_stop(QXML_File* xml);

void qxml_file_set_data(QXML_File* xml, void* data);
void* qxml_file_get_data(QXML_File* xml);

int qxml_file_process(QXML_File* xml);

#endif /* __QXML_FILE_H__ */
