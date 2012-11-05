#include <qxml/string.h>

#include <string.h>

#define QXML_MIN(x, y) (((x) < (y)) ? (x) : (y))

static int _qxml_string_startswith(const char* input, const char* what)
{
    return !strncmp(input, what, strlen(what));
}
static void _qxml_string_appendlen(size_t* num, char* out, size_t olen, const char* str, size_t len)
{
    *num += len;
    if(*num - len >= olen)
        return;

    memcpy(&out[*num - len], str, QXML_MIN(olen - *num + len, len));
}
static void _qxml_string_append(size_t* num, char* out, size_t olen, const char* str)
{
    _qxml_string_appendlen(num, out, olen, str, strlen(str));
}

size_t qxml_string_escapelent(char* out, size_t olen, const char* str, size_t len)
{
    size_t num = 0;
    size_t i;

    for(i = 0; i < len; i++)
    {
        switch(str[i])
        {
            /*case '"':  _qxml_string_append(&num, out, olen, "&quot;"); break;
            case '\'': _qxml_string_append(&num, out, olen, "&apos;"); break;*/
            case '<':  _qxml_string_append(&num, out, olen, "&lt;"); break;
            case '>':  _qxml_string_append(&num, out, olen, "&gt;"); break;
            case '&':  _qxml_string_append(&num, out, olen, "&amp;"); break;
            default: _qxml_string_appendlen(&num, out, olen, &str[i], 1);
        }
    }
    if(olen)
        out[QXML_MIN(num, olen - 1)] = 0;

    return num;
}
size_t qxml_string_escapet(char* out, size_t olen, const char* str)
{
    return qxml_string_escapelent(out, olen, str, strlen(str));
}

size_t qxml_string_escapelen(char* out, size_t olen, const char* str, size_t len)
{
    size_t num = 0;
    size_t i;

    for(i = 0; i < len; i++)
    {
        switch(str[i])
        {
            case '"':  _qxml_string_append(&num, out, olen, "&quot;"); break;
            case '\'': _qxml_string_append(&num, out, olen, "&apos;"); break;
            case '<':  _qxml_string_append(&num, out, olen, "&lt;"); break;
            case '>':  _qxml_string_append(&num, out, olen, "&gt;"); break;
            case '&':  _qxml_string_append(&num, out, olen, "&amp;"); break;
            default: _qxml_string_appendlen(&num, out, olen, &str[i], 1);
        }
    }
    if(olen)
        out[QXML_MIN(num, olen - 1)] = 0;

    return num;
}
size_t qxml_string_escape(char* out, size_t olen, const char* str)
{
    return qxml_string_escapelen(out, olen, str, strlen(str));
}

/* todo: unescape &#XXXX; */
size_t qxml_string_unescapelen(char* out, size_t olen, const char* str, size_t len)
{
    static const char* const Entities[]     = { "&quot;", "&apos;", "&lt;", "&gt;", "&amp;", NULL };
    static const char* const Replacements[] = { "\""    , "'"     , "<"   , ">"   , "&"    , NULL };

    size_t num = 0;
    size_t i, j;

    int replaced;

    for(i = 0; i < len; i++)
    {
        replaced = 0;
        for(j = 0; Entities[j]; j++)
        {
            if(_qxml_string_startswith(&str[i], Entities[j]))
            {
                _qxml_string_append(&num, out, olen, Replacements[j]);
                i += strlen(Entities[j]) - 1;
                replaced = 1;
                break;
            }
        }
        if(!replaced)
            _qxml_string_appendlen(&num, out, olen, &str[i], 1);
    }
    if(olen)
        out[QXML_MIN(num, olen - 1)] = 0;

    return num;
}
size_t qxml_string_unescape(char* out, size_t olen, const char* str)
{
    return qxml_string_unescapelen(out, olen, str, strlen(str));
}
