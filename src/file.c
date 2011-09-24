#include <qxml/file.h>
#include <qxml/internal.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static int isalphaU8(char c)
{
    return c > 127 || isalpha(c);
}
static int isalnumU8(char c)
{
    return isalphaU8(c) || isdigit(c);
}
static int isidentU8(char c)
{
    return isalnumU8(c) || strchr(".:_-+*", c);
}
static const char* skipSpace(const char* input)
{
    while(*input && isspace(*input))
        input++;
    return input;
}

static QXML_File* _qxml_file_create_textlen_nocopy(char* text, size_t len)
{
    static const QXML_File init = {0};

    QXML_File* xml = malloc(sizeof(QXML_File));
    if(!xml)
    {
        free(text);
        return NULL;
    }
    memcpy(xml, &init, sizeof(QXML_File));

    xml->len = len;
    xml->text = text;

    return xml;
}
QXML_File* qxml_file_create_textlen(const char* text, size_t len)
{
    char* copy = malloc(len + 1);
    if(!copy)
        return NULL;
    copy[len] = 0;
    memcpy(copy, text, len);

    return _qxml_file_create_textlen_nocopy(copy, len);
}
QXML_File* qxml_file_create_text(const char* text)
{
    return qxml_file_create_textlen(text, strlen(text));
}
QXML_File* qxml_file_create(const char* fname)
{
    size_t len;
    char* text;

    FILE* file = fopen(fname, "rb");
    if(!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    text = malloc(len + 1);
    if(!text)
    {
        fclose(file);
        return NULL;
    }
    text[len] = 0;

    fseek(file, 0, SEEK_SET);
    fread(text, 1, len, file);

    fclose(file);

    return _qxml_file_create_textlen_nocopy(text, len);
}
void qxml_file_destroy(QXML_File* xml)
{
    if(!xml)
        return;
    free(xml->text);
    free(xml);
}
void qxml_file_stop(QXML_File* xml)
{
    xml->stop = 1;
}

#define QXML_CALL(xml, func, args) do { if((xml)->func) (xml)->func args; if((xml)->stop) return 2; } while(0)
/*#define QXML_ERROR(xml, msg) do { if((xml)->cb_error) (xml)->cb_error((xml), 0, 0, 0, msg, strlen(msg)); } while(0)*/
#define QXML_ERROR(xml, msg) QXML_CALL(xml, cb_error, ((xml), 0, 0, 0, msg, strlen(msg)))

int qxml_file_process(QXML_File* xml)
{
    const char* input = xml->text;

    struct {
        const char* from;
        const char* to;
    } elem, akey, aval, ws;

    const char* from;
    const char* to;

    int hadchars;

    char strchar;

    /* UTF-8 byte order mark */
    if(_qxml_string_startswith(input, "\xEF\xBB\xBF"))
        input += 3;

    while(*input)
    {
        hadchars = 0;

        ws.from = input;
        input = skipSpace(input);
        ws.to = input;

        if(!*input)
            break;
        /*printf("---------INPUT----------\n%s\n", input);*/

        if(_qxml_string_startswith(input, "<!--"))
        {
            input += 4;

            from = input;
            while(*input && !_qxml_string_startswith(input, "-->"))
                input++;
            to = input;

            if(!*input)
            {
                QXML_ERROR(xml, "Unterminated comment");
                return 0;
            }

            QXML_CALL(xml, cb_comment, (xml, from, to - from));

            input += 3;
        }
        else if(_qxml_string_startswith(input, "<?xml"))
        {
            input += 5;

            from = input;
            while(*input && !_qxml_string_startswith(input, "?>"))
                input++;
            to = input;

            if(!*input)
            {
                QXML_ERROR(xml, "Unterminated XML declaration");
                return 0;
            }

            QXML_CALL(xml, cb_xml_decl, (xml, from, to - from));

            input += 2;
        }
        else if(_qxml_string_startswith(input, "<![CDATA["))
        {
            input += 9;

            from = input;
            while(*input && !_qxml_string_startswith(input, "]]>"))
                input++;
            to = input;

            if(!*input)
            {
                QXML_ERROR(xml, "Unterminated CDATA section");
                return 0;
            }

            QXML_CALL(xml, cb_cdata, (xml, from, to - from));

            input += 3;
        }
        else if(_qxml_string_startswith(input, "</"))
        {
            input += 2;

            elem.from = input;
            while(*input && isidentU8(*input))
                input++;
            elem.to = input;

            if(!*input)
            {
                QXML_ERROR(xml, "Unterminated end of element");
                return 0;
            }
            if(elem.to == elem.from)
            {
                QXML_ERROR(xml, "End of element with no name");
                return 0;
            }

            QXML_CALL(xml, cb_elem_end, (xml, elem.from, elem.to - elem.from));

            input = skipSpace(input);

            if(*input != '>')
            {
                QXML_ERROR(xml, "Unterminated end of element");
                return 0;
            }
            input += 1;
        }
        else if(_qxml_string_startswith(input, "<"))
        {
            input += 1;

            elem.from = input;
            while(*input && isidentU8(*input))
                input++;
            elem.to = input;

            if(!*input)
            {
                QXML_ERROR(xml, "Unterminated element");
                return 0;
            }
            if(elem.to == elem.from)
            {
                QXML_ERROR(xml, "Element with no name");
                return 0;
            }

            QXML_CALL(xml, cb_elem_begin, (xml, elem.from, elem.to - elem.from));

            input = skipSpace(input);
            while(*input && *input != '>' && *input != '/')
            {
                akey.from = input;
                while(*input && isidentU8(*input))
                    input++;
                akey.to = input;

                if(!*input)
                {
                    QXML_ERROR(xml, "Unterminated element");
                    return 0;
                }
                if(elem.to == elem.from)
                {
                    QXML_ERROR(xml, "Cannot parse element");
                    return 0;
                }

                if(*input == '=')
                {
                    input += 1;

                    if(!*input || !strchr("\"'`", *input))
                    {
                        QXML_ERROR(xml, "Cannot parse attribute value");
                        return 0;
                    }

                    strchar = *input;
                    input += 1;

                    aval.from = input;
                    while(*input && *input != strchar)
                        input++;
                    aval.to = input;

                    if(!*input)
                    {
                        QXML_ERROR(xml, "Unterminated attribute value");
                        return 0;
                    }

                    QXML_CALL(xml, cb_elem_attrib, (xml, akey.from, akey.to - akey.from, aval.from, aval.to - aval.from));

                    input += 1;
                }
                else
                    QXML_CALL(xml, cb_elem_attrib, (xml, akey.from, akey.to - akey.from, NULL, 0));

                input = skipSpace(input);
            }

            if(!*input)
            {
                QXML_ERROR(xml, "Unterminated element");
                return 0;
            }
            if(*input == '/')
            {
                QXML_CALL(xml, cb_elem_end, (xml, elem.from, elem.to - elem.from));
                input += 1;
                input = skipSpace(input);
            }

            if(*input != '>')
            {
                QXML_ERROR(xml, "Unterminated element");
                return 0;
            }
            input += 1;
        }
        else
        {
            from = ws.from;
            while(*input && *input != '<' && *input != '>')
                input++;
            to = input;

            if(from == to)
            {
                QXML_ERROR(xml, "Unknown item - possible stray '<' or '>' character");
                return 0;
            }

            QXML_CALL(xml, cb_text, (xml, from, to - from));
            hadchars = 1;
        }

        if(!hadchars)
        {
            QXML_CALL(xml, cb_whitespace, (xml, ws.from, ws.to - ws.from));
        }
    }

    return 1;
}
