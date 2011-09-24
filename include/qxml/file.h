#ifndef __QXML_FILE_H__
#define __QXML_FILE_H__

#include <stddef.h>

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
    size_t len;
    char* text;
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


QXML_File* qxml_file_create_textlen(const char* text, size_t len);
QXML_File* qxml_file_create_text(const char* text);
QXML_File* qxml_file_create(const char* fname);

void qxml_file_destroy(QXML_File* xml);
void qxml_file_stop(QXML_File* xml);

int qxml_file_process(QXML_File* xml);

#endif /* __QXML_FILE_H__ */
