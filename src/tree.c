#include <qxml/tree.h>
#include <qxml/file.h>
#include <qxml/string.h>

#include <stdlib.h>
#include <string.h>

int printf(const char*, ...);

typedef struct QXML_Context
{
    size_t stacklen;
    size_t stackmem;
    QXML_Node** stack;

    QXML_Node* head;
    QXML_Node* tail;
} QXML_Context;

static void stackPush(QXML_File* xml, QXML_Node* item)
{
    QXML_Context* ctx = xml->data;

    QXML_Node** tmp;

    if(ctx->stacklen + 1 >= ctx->stackmem)
    {
        ctx->stackmem = ctx->stackmem ? ctx->stackmem << 1 : 32;

        tmp = realloc(ctx->stack, ctx->stackmem * sizeof(QXML_Node*));
        if(!tmp)
        {
            qxml_file_stop(xml);
            return;
        }
        ctx->stack = tmp;
    }
    ctx->stack[ctx->stacklen++] = item;
}
static QXML_Node* stackPeek(QXML_File* xml)
{
    QXML_Context* ctx = xml->data;

    if(!ctx->stacklen)
    {
        qxml_file_stop(xml);
        return NULL;
    }
    return ctx->stack[ctx->stacklen - 1];
}
static QXML_Node* stackPop(QXML_File* xml)
{
    QXML_Context* ctx = xml->data;

    if(!ctx->stacklen)
    {
        qxml_file_stop(xml);
        return NULL;
    }
    return ctx->stack[--ctx->stacklen];
}

static void addAttrib(QXML_File* xml, QXML_Node* node, const char* key, size_t klen, const char* val, size_t vlen)
{
    char** tmp;
    size_t ulen;

    node->numattrib++;

    tmp = realloc(node->attribkeys, node->numattrib * sizeof(char*));
    if(!tmp) goto rerror;
    node->attribkeys = tmp;

    tmp = realloc(node->attribvals, node->numattrib * sizeof(char*));
    if(!tmp) goto rerror;
    node->attribvals = tmp;

    node->attribkeys[node->numattrib - 1] = malloc(klen + 1);
    if(!node->attribkeys[node->numattrib - 1]) goto error;
    memcpy(node->attribkeys[node->numattrib - 1], key, klen);
    node->attribkeys[node->numattrib - 1][klen] = 0;

    if(val)
    {
        ulen = qxml_string_unescapelen(NULL, 0, val, vlen);

        node->attribvals[node->numattrib - 1] = malloc(ulen + 1);
        if(!node->attribvals[node->numattrib - 1]) goto error;
        qxml_string_unescapelen(node->attribvals[node->numattrib - 1], ulen + 1, val, vlen);
        node->attribvals[node->numattrib - 1][vlen] = 0;
    }
    else
        node->attribvals[node->numattrib - 1] = NULL;

    /*if(node->attribvals[node->numattrib - 1])
        printf("ATTRIB %s => %s\n", node->attribkeys[node->numattrib - 1], node->attribvals[node->numattrib - 1]);
    else
        printf("ATTRIB %s\n", node->attribkeys[node->numattrib - 1]);*/

    return;
rerror:
    node->numattrib--;
error:
    qxml_file_stop(xml);
}

static QXML_Node* initNode(QXML_NodeType type, const char* str, size_t len, int escaped)
{
    QXML_Node* node = malloc(sizeof(QXML_Node));
    if(!node)
        goto error;
    node->parent = NULL;
    node->prev = NULL;
    node->next = NULL;
    node->fchild = NULL;
    node->lchild = NULL;
    node->type = type;

    if(escaped)
        node->slen = qxml_string_unescapelen(NULL, 0, str, len);
    else
        node->slen = len;
    node->str = malloc(node->slen + 1);
    if(!node->str)
        goto error;
    if(escaped)
        qxml_string_unescapelen(node->str, node->slen + 1, str, len);
    else
    {
        memcpy(node->str, str, len);
        node->str[len] = 0;
    }

    node->numattrib = 0;
    node->attribkeys = NULL;
    node->attribvals = NULL;

    return node;
error:
    if(node)
    {
        if(node->str)
            free(node->str);
        free(node);
    }
    return NULL;
}


static QXML_Node* addNode(QXML_File* xml, QXML_NodeType type, const char* str, size_t len, int escaped)
{
    QXML_Context* ctx = xml->data;

    QXML_Node* node = initNode(type, str, len, escaped);
    if(!node)
    {
        qxml_file_stop(xml);
        return NULL;
    }

    if(ctx->stacklen)
    {
        node->parent = stackPeek(xml);
        node->prev = node->parent->lchild;
        if(node->prev)
            node->prev->next = node;
        node->next = NULL;
        if(!node->parent->fchild)
            node->parent->fchild = node;
        node->parent->lchild = node;
    }
    else /* we are at root level */
    {
        node->prev = ctx->tail;
        if(node->prev)
            node->prev->next = node;
        if(!ctx->head)
            ctx->head = node;
        ctx->tail = node;
    }

    return node;
}

static void cbComment(QXML_File* xml, const char* text, size_t len)
{
    addNode(xml, QXML_NT_COMMENT, text, len, 0);
}
static void cbCData(struct QXML_File* xml, const char* cdata, size_t len)
{
    addNode(xml, QXML_NT_CDATA, cdata, len, 0);
}
static void cbElemBegin(struct QXML_File* xml, const char* name, size_t len)
{
    QXML_Node* node = addNode(xml, QXML_NT_ELEM, name, len, 0);
    if(!node)
        return;
    stackPush(xml, node);
}
static void cbElemEnd(struct QXML_File* xml, const char* name, size_t len)
{
    QXML_Node* node = stackPeek(xml);
    if(!node)
        return;

    if(node->slen != len || strncmp(node->str, name, len))
    {
        qxml_file_stop(xml);
        return;
    }
    /* printf("POP %s\n", node->str); */
    stackPop(xml);
}
static void cbElemAttrib(struct QXML_File* xml, const char* key, size_t klen, const char* val, size_t vlen)
{
    QXML_Node* node = stackPeek(xml);
    if(!node)
        return;
    addAttrib(xml, node, key, klen, val, vlen);
}
static void cbXMLDecl(struct QXML_File* xml, const char* decl, size_t len)
{
    addNode(xml, QXML_NT_XMLDECL, decl, len, 0);
}
static void cbText(struct QXML_File* xml, const char* text, size_t len)
{
    addNode(xml, QXML_NT_TEXT, text, len, 1);
}
static void cbWhitespace(struct QXML_File* xml, const char* ws, size_t len)
{
    /* we ignore WS! */
    /* initNode(xml, QXML_NT_WHITESPACE, ws, len); */
}
/*static void cbError(struct QXML_File* xml, size_t offset, size_t line, size_t col, const char* msg, size_t len)
{
    printf("ERROR (%lu:%lu): %.*s\n", (long unsigned int)line, (long unsigned int)col, (int)len, msg);
}
*/

QXML_Node* qxml_tree_create(struct QXML_File* xml)
{
    QXML_File tmp;
    QXML_Context ctx = {0};

    ctx.stacklen = 0;
    ctx.stackmem = 32;
    ctx.stack = malloc(ctx.stackmem * sizeof(QXML_Node*));
    ctx.head = NULL;
    ctx.tail = NULL;

    memcpy(&tmp, xml, sizeof(QXML_File));
    tmp.data = &ctx;

    tmp.cb_comment = cbComment;
    tmp.cb_cdata = cbCData;
    tmp.cb_elem_begin = cbElemBegin;
    tmp.cb_elem_end = cbElemEnd;
    tmp.cb_elem_attrib = cbElemAttrib;
    tmp.cb_xml_decl = cbXMLDecl;
    tmp.cb_text = cbText;
    tmp.cb_whitespace = cbWhitespace;

    qxml_file_process(&tmp);

    free(ctx.stack);

    return ctx.head;
}
void qxml_tree_destroy(QXML_Node* root)
{
    QXML_Node* next;

    if(!root || root->parent)
        return;

    /* first, find the leftmost node */
    while(root->prev)
        root = root->prev;

    for(; root; root = next)
    {
        next = root->next;
        qxml_node_remove(root);
    }
}

void _qxml_tree_dump(QXML_Node* root, size_t indent, size_t indentadd)
{
    size_t i;
    size_t len;
    char* buf = NULL;

    while(root->prev)
        root = root->prev;

    for(; root; root = root->next)
    {
        for(i = 0; i < indent; i++) printf(" ");

        switch(root->type)
        {
            case QXML_NT_COMMENT: printf("<!--%s-->\n", root->str); break;
            case QXML_NT_CDATA: printf("<![CDATA[%s]]>\n", root->str); break;
            case QXML_NT_ELEM:
                printf("<%s", root->str);
                for(i = 0; i < root->numattrib; i++)
                {
                    printf(" %s", root->attribkeys[i]);
                    if(root->attribvals[i])
                    {
                        len = qxml_string_escape(NULL, 0, root->attribvals[i]);
                        buf = realloc(buf, len + 1);
                        qxml_string_escape(buf, len + 1, root->attribvals[i]);
                        printf("=\"%s\"", buf);
                    }
                }
                if(root->fchild)
                {
                    printf(">\n");

                    _qxml_tree_dump(root->fchild, indent + indentadd, indentadd);

                    for(i = 0; i < indent; i++) printf(" ");
                    printf("</%s>\n", root->str);
                }
                else
                    printf("/>\n");
                break;
            case QXML_NT_XMLDECL: printf("<?xml%s?>\n", root->str); break;
            case QXML_NT_TEXT:
                len = qxml_string_escape(NULL, 0, root->str);
                buf = realloc(buf, len + 1);
                qxml_string_escapet(buf, len + 1, root->str);
                printf("%s\n", buf); break;
            case QXML_NT_WHITESPACE: printf(" "); break;
                break;
        }
    }

    if(buf)
        free(buf);
}

QXML_Node* qxml_node_insert_child(QXML_Node* parent, QXML_NodeType type, const char* str)
{
    QXML_Node* node = initNode(type, str, strlen(str), 0);
    if(!node)
        return NULL;

    node->parent = parent;
    node->prev = node->parent->lchild;
    if(node->prev)
        node->prev->next = node;
    node->next = NULL;
    if(!node->parent->fchild)
        node->parent->fchild = node;
    node->parent->lchild = node;

    return node;
}
QXML_Node* qxml_node_insert_before(QXML_Node* next, QXML_NodeType type, const char* str)
{
    QXML_Node* node = initNode(type, str, strlen(str), 0);
    if(!node)
        return NULL;

    node->parent = next->parent;
    node->prev = next->prev;
    if(node->prev)
        node->prev->next = node;
    node->next = next;
    next->prev = node;

    if(next->parent && next->parent->fchild == next)
        next->parent->fchild = node;

    return node;
}
QXML_Node* qxml_node_insert_after(QXML_Node* prev, QXML_NodeType type, const char* str)
{
    QXML_Node* node = initNode(type, str, strlen(str), 0);
    if(!node)
        return NULL;

    node->parent = prev->parent;
    node->prev = prev;
    prev->next = node;
    node->next = prev->next;
    if(node->next)
        node->next->prev = node;

    if(prev->parent && prev->parent->lchild == prev)
        prev->parent->lchild = node;

    return node;
}
void qxml_node_remove(QXML_Node* node)
{
    size_t i;

    if(node)
    {
        if(node->prev)
            node->prev->next = node->next;
        if(node->next)
            node->next->prev = node->prev;
        if(node->parent)
        {
            if(node->parent->fchild == node)
                node->parent->fchild = node->prev;
            if(node->parent->lchild == node)
                node->parent->lchild = node->next;
        }

        free(node->str);

        for(i = 0; i < node->numattrib; i++)
        {
            free(node->attribkeys[i]);
            if(node->attribvals[i])
                free(node->attribvals[i]);
        }
        free(node->attribkeys);
        free(node->attribvals);

        free(node);

        while(node->fchild)
            qxml_node_remove(node->fchild);
    }
}
