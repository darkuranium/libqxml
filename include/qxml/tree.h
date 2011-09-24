#ifndef __QXML_TREE_H__
#define __QXML_TREE_H__

#include <stddef.h>

struct QXML_File;

typedef enum QXML_NodeType
{
    QXML_NT_COMMENT,
    QXML_NT_CDATA,
    QXML_NT_ELEM,
    QXML_NT_XMLDECL,
    QXML_NT_TEXT,
    QXML_NT_WHITESPACE
} QXML_NodeType;

static const char* const QXML_NodeTypeName[] =
    {
        "QXML_NT_COMMENT",
        "QXML_NT_CDATA",
        "QXML_NT_ELEM",
        "QXML_NT_XMLDECL",
        "QXML_NT_TEXT",
        "QXML_NT_WHITESPACE"
    } ;

typedef struct QXML_Node
{
    struct QXML_Node* parent;
    struct QXML_Node* prev;
    struct QXML_Node* next;
    struct QXML_Node* fchild; /* first child */
    struct QXML_Node* lchild; /* last child */
    QXML_NodeType type;
    size_t slen;
    char* str;

    size_t numattrib;
    char** attribkeys;
    char** attribvals;
} QXML_Node;

QXML_Node* qxml_tree_create(struct QXML_File* xml);
void qxml_tree_destroy(QXML_Node* root);

void _qxml_tree_dump(QXML_Node* root, size_t indent, size_t indentadd);

QXML_Node* qxml_node_insert_child(QXML_Node* parent, QXML_NodeType type, const char* str);
QXML_Node* qxml_node_insert_before(QXML_Node* next, QXML_NodeType type, const char* str);
QXML_Node* qxml_node_insert_after(QXML_Node* prev, QXML_NodeType type, const char* str);
void qxml_node_remove(QXML_Node* node);

#endif /* __QXML_TREE_H__ */
