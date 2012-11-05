#include <stdlib.h>
#include <stdio.h>

#include "qxml/qxml.h"

static void cbComment(QXML_File* xml, const char* text, size_t len)
{
    printf("COMMENT %.*s\n", (int)len, text);
}
static void cbCData(QXML_File* xml, const char* cdata, size_t len)
{
    printf("CDATA %.*s\n", (int)len, cdata);
}
static void cbElemBegin(QXML_File* xml, const char* name, size_t len)
{
    printf("ELEMENT-BEGIN %.*s\n", (int)len, name);
}
static void cbElemEnd(QXML_File* xml, const char* name, size_t len)
{
    printf("ELEMENT-END %.*s\n", (int)len, name);
}
static void cbElemAttrib(QXML_File* xml, const char* key, size_t klen, const char* val, size_t vlen)
{
    if(val)
        printf("ELEMENT-ATTRIB %.*s=%.*s\n", (int)klen, key, (int)vlen, val);
    else
        printf("ELEMENT-ATTRIB %.*s\n", (int)klen, key);
}
static void cbXMLDecl(QXML_File* xml, const char* decl, size_t len)
{
    printf("XMLDECL %.*s\n", (int)len, decl);
}
static void cbText(QXML_File* xml, const char* text, size_t len)
{
    printf("TEXT %.*s\n", (int)len, text);
}
static void cbWhitespace(QXML_File* xml, const char* ws, size_t len)
{
    printf("WHITESPACE %.*s\n", (int)len, ws);
}
static void cbError(QXML_File* xml, size_t offset, size_t line, size_t col, const char* msg, size_t len)
{
    printf("ERROR (%lu:%lu): %.*s\n", (long unsigned int)line, (long unsigned int)col, (int)len, msg);
}

int main()
{
    int ret;

    char etestf[] = "a < b && b < c";
    char etestt[256];
    char utestf[] = "a &lt; b &amp;&amp; b &lt; c";
    char utestt[256];
    QXML_Node* node;
    QXML_File* xml;

    xml = qxml_file_create_fname("test.xml");
    if(!xml)
    {
        fprintf(stderr, "Error: Cannot open file \"test.xml\"\n");
        exit(2);
    }
    xml->cb_comment = cbComment;
    xml->cb_cdata = cbCData;
    xml->cb_elem_begin = cbElemBegin;
    xml->cb_elem_end = cbElemEnd;
    xml->cb_elem_attrib = cbElemAttrib;
    xml->cb_xml_decl = cbXMLDecl;
    xml->cb_text = cbText;
    xml->cb_whitespace = cbWhitespace;
    xml->cb_error = cbError;

    ret = !qxml_file_process(xml);

    printf("----------\n");

    qxml_file_rewind(xml);

    node = qxml_tree_create(xml);

    printf("----------\n");

    _qxml_tree_dump(node, 0, 4);

    qxml_tree_destroy(node);

    printf("----------\n");

    qxml_string_escape(etestt, sizeof(etestt), etestf);
    printf("%s => %s\n", etestf, etestt);

    qxml_string_unescape(utestt, sizeof(utestt), utestf);
    printf("%s => %s\n", utestf, utestt);

    qxml_file_destroy(xml);

    return ret;
}
