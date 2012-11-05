#include <qxml/file.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static int isalphaU8(char c)
{
    return ((unsigned char)c) > 127 || isalpha(c);
}
static int isalnumU8(char c)
{
    return isalphaU8(c) || isdigit(c);
}
static int isidentU8(char c)
{
    return isalnumU8(c) || strchr(".:_-+*", c);
}

QXML_File* qxml_file_create(QXML_Stream* stream, int delstream)
{
    static const QXML_File init = {0};

    QXML_File* xml = malloc(sizeof(QXML_File));
    if(!xml)
    {
        if(delstream) qxml_stream_destroy(stream);
        return NULL;
    }
    memcpy(xml, &init, sizeof(QXML_File));

    xml->stream = stream;
    xml->delstream = delstream;

    return xml;
}
QXML_File* qxml_file_create_fd(int fd, int close)
{
    return qxml_file_create(qxml_stream_create_fd(fd, close), 1);
}
QXML_File* qxml_file_create_file(FILE* file, int close)
{
    return qxml_file_create(qxml_stream_create_file(file, close), 1);
}
QXML_File* qxml_file_create_fname(const char* fname)
{
    return qxml_file_create(qxml_stream_create_fname(fname, "rb"), 1);
}
QXML_File* qxml_file_create_strlen(const char* str, size_t len, int copy)
{
    return qxml_file_create(qxml_stream_create_strlen((char*)str, len, copy), 1);
}
QXML_File* qxml_file_create_str(const char* str, int copy)
{
    return qxml_file_create(qxml_stream_create_str((char*)str, copy), 1);
}

void qxml_file_destroy(QXML_File* xml)
{
    if(!xml)
        return;
    if(xml->delstream)
        qxml_stream_destroy(xml->stream);
    free(xml);
}

void qxml_file_rewind(QXML_File* xml)
{
    xml->stream->srewind(xml->stream->ptr);
}
void qxml_file_stop(QXML_File* xml)
{
    xml->stop = 1;
}

void qxml_file_set_data(QXML_File* xml, void* data)
{
    xml->data = data;
}
void* qxml_file_get_data(QXML_File* xml)
{
    return xml->data;
}

#define QXML_CALL(xml, func, args) do { if((xml)->func) (xml)->func args; if((xml)->stop) return 2; } while(0)
#define QXML_ERROR(xml, msg) QXML_CALL(xml, cb_error, ((xml), 0, 0, 0, msg, strlen(msg)))

#define BUFSIZE 17

typedef struct CBuffer
{
    size_t len;
    size_t head;
    size_t tail;
    char* ptr;
    QXML_File* xml;
} CBuffer;

static size_t cbuf_len(CBuffer* buf)
{
    return (buf->tail + buf->len - buf->head) % buf->len;
}
static void cbuf_fill(CBuffer* buf)
{
    int c;

    if(!buf->xml)
        return;

    while((buf->tail + 1) % buf->len != buf->head)
    {
        c = buf->xml->stream->sgetc(buf->xml->stream->ptr);
        if(c == EOF)
            break;
        buf->ptr[buf->tail] = c;
        buf->tail = (buf->tail + 1) % buf->len;
    }
}
static void cbuf_discard(CBuffer* buf, size_t n)
{
    while(n--)
    {
        if(buf->head == buf->tail) cbuf_fill(buf);
        if(buf->head == buf->tail) break;
        buf->head = (buf->head + 1) % buf->len;
    }
}
static int cbuf_head(CBuffer* buf)
{
    if(buf->head == buf->tail) cbuf_fill(buf);
    if(buf->head == buf->tail) return EOF;
    return buf->ptr[buf->head];
}
static int cbuf_getc(CBuffer* buf)
{
    int c = cbuf_head(buf);
    if(c != EOF)
        cbuf_discard(buf, 1);
    return c;
}

typedef struct Buffer
{
    size_t mem;
    size_t len;
    char* ptr;
} Buffer;
static void buf_free(Buffer b)
{
    free(b.ptr);
}
static Buffer buf_init(void)
{
    Buffer b = { 0, 0, NULL };
    return b;
}
static Buffer buf_clear(Buffer b)
{
    if(b.len)
    {
        b.len = 0;
        b.ptr[0] = 0;
    }
    return b;
}
static Buffer buf_copy(Buffer b, Buffer from)
{
    while(b.len + from.len + 1 > b.mem)
    {
        b.mem = b.mem ? b.mem << 1 : 1;
        b.ptr = realloc(b.ptr, b.mem);
    }
    memcpy(b.ptr + b.len, from.ptr, from.len);
    b.len += from.len;
    b.ptr[b.len] = 0;
    return b;
}
static Buffer buf_append(Buffer b, char c)
{
    Buffer tmp = { 1, 1, NULL };
    tmp.ptr = &c;
    return buf_copy(b, tmp);
}
static int startswith(CBuffer* cbuf, const char* what, int alnum)
{
    int c;
    CBuffer mybuf = *cbuf;
    mybuf.xml = NULL;

    while(*what && (c = cbuf_getc(&mybuf)) != EOF)
    {
        /*printf("CMP %u==%u (0x%X) %c==%c", c, (unsigned char)*what, (unsigned char)*what, c, *what);*/
        if(c != *what)
            return 0;
        what++;
    }
    if(c == EOF)
        return 0;
    c = cbuf_getc(&mybuf);
    if(alnum == 0 && c != EOF && isalnumU8(c))
        return 0;
    return 1;
}
static Buffer get_space(Buffer b, CBuffer* buf)
{
    int c;
    for(;;)
    {
        c = cbuf_head(buf);
        if(c == EOF || !isspace(c))
            break;
        cbuf_discard(buf, 1);
        b = buf_append(b, c);
    }
    return b;
}

void pbuf(CBuffer* buf)
{
    int c;
    CBuffer mybuf = *buf;
    mybuf.xml = NULL;

    while((c = cbuf_getc(&mybuf)) != EOF)
        printf("%c", c);
    printf("\n");
}

/*
 * If you're wondering what the hell is going on here with all the
 * macros (and the above mess of a code), let me reassure you that it is
 * merely a temporary measure, done to ease the conversion from the old
 * functions (which would only work on whole strings) to this.
 *
 * You don't see a good reason because there is none anymore, but it has
 * helped me a *lot* in getting this done incrementally (say, by only
 * "updating" comment handling and leaving the rest as it was).
 *
 * There are two issues left to resolve, however:
 * 1) Should I merge element-begin and element-attribute events?
 * 2) Should text data (attributes, text, etc...) be automagically
 *    unescaped?
 * Comparing with other SAX parsers, libxml2's parser has the answer
 * "yes" to both; Java has "yes" to the former, but I'm not sure on the
 * latter. But then again, this isn't exactly the most standarized
 * API/parsing method in existance...
 *
 * I still have to add an API to use `getc()`-sort of callbacks for this
 * (with convenience wrappers for strings, FILE* and filenames) plus
 * possibly an error handling callback.
 *
 * I'll also need something for doing things the other way - writing
 * the tree.
 *
 * Finally, I'll have to make the tree API more friendly.
 */
int qxml_file_process(QXML_File* xml)
{
    char quot;

    int ret;

    Buffer wsbuf = buf_init();
    Buffer buf = buf_init();
    Buffer keybuf = buf_init();
    Buffer valbuf = buf_init();

    char sbuf[BUFSIZE];

    CBuffer cbuf;
    cbuf.len = BUFSIZE;
    cbuf.head = 0;
    cbuf.tail = 0;
    cbuf.ptr = sbuf;
    cbuf.xml = xml;

#define LEN() cbuf_len(&cbuf)
#define FILL() cbuf_fill(&cbuf)
#define READ() FILL()
#define READTO(n) READ()
#define DISCARDN(n) cbuf_discard(&cbuf, n)
#define HEAD() cbuf_head(&cbuf)

    FILL();
    /* UTF-8 byte order mark */
    if(startswith(&cbuf, "\xEF\xBB\xBF", 1))
        DISCARDN(3);

    for(;;)
    {
        READ();

        wsbuf = get_space(wsbuf, &cbuf);

        if(!LEN())
            break;

        if(startswith(&cbuf, "<!--", 1))
        {
            if(wsbuf.len)
                QXML_CALL(xml, cb_whitespace, (xml, wsbuf.ptr, wsbuf.len));
            wsbuf = buf_clear(wsbuf);

            DISCARDN(4);
            READTO(3);
            buf = buf_clear(buf);
            while(LEN() && !startswith(&cbuf, "-->", 1))
            {
                buf = buf_append(buf, cbuf_getc(&cbuf));
                READTO(3);
            }
            if(!LEN())
            {
                QXML_ERROR(xml, "Unterminated comment");
                goto error;
            }
            QXML_CALL(xml, cb_comment, (xml, buf.ptr, buf.len));
            buf = buf_clear(buf);
            DISCARDN(3);
        }
        else if(startswith(&cbuf, "<?xml", 0))
        {
            if(wsbuf.len)
                QXML_CALL(xml, cb_whitespace, (xml, wsbuf.ptr, wsbuf.len));
            wsbuf = buf_clear(wsbuf);

            DISCARDN(5);
            READTO(2);
            while(LEN() && !startswith(&cbuf, "?>", 1))
            {
                buf = buf_append(buf, cbuf_getc(&cbuf));
                READTO(2);
            }
            if(!LEN())
            {
                QXML_ERROR(xml, "Unterminated XML declaration");
                goto error;
            }
            QXML_CALL(xml, cb_xml_decl, (xml, buf.ptr, buf.len));
            buf = buf_clear(buf);
            DISCARDN(2);
        }
        else if(startswith(&cbuf, "<![CDATA[", 1))
        {
            if(wsbuf.len)
                QXML_CALL(xml, cb_whitespace, (xml, wsbuf.ptr, wsbuf.len));
            wsbuf = buf_clear(wsbuf);

            DISCARDN(9);
            READTO(3);
            while(LEN() && !startswith(&cbuf, "]]>", 1))
            {
                buf = buf_append(buf, cbuf_getc(&cbuf));
                READTO(3);
            }
            if(!LEN())
            {
                QXML_ERROR(xml, "Unterminated CDATA section");
                goto error;
            }
            QXML_CALL(xml, cb_cdata, (xml, buf.ptr, buf.len));
            buf = buf_clear(buf);
            DISCARDN(3);
        }
        else if(startswith(&cbuf, "</", 1))
        {
            if(wsbuf.len)
                QXML_CALL(xml, cb_whitespace, (xml, wsbuf.ptr, wsbuf.len));
            wsbuf = buf_clear(wsbuf);

            DISCARDN(2);
            READTO(1);
            while(LEN() && isidentU8(HEAD()))
            {
                buf = buf_append(buf, cbuf_getc(&cbuf));
                READTO(1);
            }
            if(!LEN())
            {
                QXML_ERROR(xml, "Unterminated end of element");
                goto error;
            }
            if(!buf.len)
            {
                QXML_ERROR(xml, "End of element with no name");
                goto error;
            }
            QXML_CALL(xml, cb_elem_end, (xml, buf.ptr, buf.len));
            buf = buf_clear(buf);
            READTO(1);
            while(LEN() && isspace(HEAD()))
            {
                DISCARDN(1);
                READTO(1);
            }
            if(HEAD() != '>')
            {
                QXML_ERROR(xml, "Unterminated end of element");
                goto error;
            }
            DISCARDN(1);
        }
        else if(startswith(&cbuf, "<", 1))
        {
            if(wsbuf.len)
                QXML_CALL(xml, cb_whitespace, (xml, wsbuf.ptr, wsbuf.len));
            wsbuf = buf_clear(wsbuf);

            DISCARDN(1);
            READTO(1);
            while(LEN() && isidentU8(HEAD()))
            {
                buf = buf_append(buf, cbuf_getc(&cbuf));
                READTO(1);
            }
            if(!LEN())
            {
                QXML_ERROR(xml, "Unterminated start of element");
                goto error;
            }
            if(!buf.len)
            {
                QXML_ERROR(xml, "Start of element with no name");
                goto error;
            }
            QXML_CALL(xml, cb_elem_begin, (xml, buf.ptr, buf.len));
            READTO(1);
            while(LEN() && isspace(HEAD()))
            {
                DISCARDN(1);
                READTO(1);
            }
            READTO(1);
            while(LEN() && HEAD() != '>' && HEAD() != '/')
            {
                while(LEN() && isidentU8(HEAD()))
                {
                    keybuf = buf_append(keybuf, cbuf_getc(&cbuf));
                    READTO(1);
                }
                if(!LEN())
                {
                    QXML_ERROR(xml, "Unterminated start of element");
                    goto error;
                }
                if(!keybuf.len)
                {
                    QXML_ERROR(xml, "Cannot parse element");
                    goto error;
                }

                if(HEAD() == '=')
                {
                    DISCARDN(1);
                    READTO(1);

                    if(!LEN() || !strchr("\"'`", HEAD()))
                    {
                        QXML_ERROR(xml, "Cannot parse attribute value");
                        goto error;
                    }

                    quot = cbuf_getc(&cbuf);

                    READTO(1);
                    while(LEN() && HEAD() != quot)
                    {
                        valbuf = buf_append(valbuf, cbuf_getc(&cbuf));
                        READTO(1);
                    }
                    if(!LEN())
                    {
                        QXML_ERROR(xml, "Unterminated attribute value");
                        goto error;
                    }
                    DISCARDN(1);

                    QXML_CALL(xml, cb_elem_attrib, (xml, keybuf.ptr, keybuf.len, valbuf.ptr, valbuf.len));
                    valbuf = buf_clear(valbuf);
                }
                else
                    QXML_CALL(xml, cb_elem_attrib, (xml, keybuf.ptr, keybuf.len, NULL, 0));
                keybuf = buf_clear(keybuf);
                READTO(1);
                while(LEN() && isspace(HEAD()))
                {
                    DISCARDN(1);
                    READTO(1);
                }
            }
            if(!LEN())
            {
                QXML_ERROR(xml, "Unterminated start of element");
                goto error;
            }
            if(HEAD() == '/')
            {
                QXML_CALL(xml, cb_elem_end, (xml, buf.ptr, buf.len));
                DISCARDN(1);
                READTO(1);
                while(LEN() && isspace(HEAD()))
                {
                    DISCARDN(1);
                    READTO(1);
                }
            }
            if(HEAD() != '>')
            {
                QXML_ERROR(xml, "Unterminated start of element");
                goto error;
            }
            DISCARDN(1);
            buf = buf_clear(buf);
        }
        else
        {
            buf = buf_copy(buf, wsbuf);
            wsbuf = buf_clear(wsbuf);
            READTO(1);
            while(LEN() && HEAD() != '<' && HEAD() != '>')
            {
                buf = buf_append(buf, cbuf_getc(&cbuf));
                READTO(1);
            }
            if(!buf.len)
            {
                QXML_ERROR(xml, "Unknown item - possible stray '<' or '>' character");
                goto error;
            }
            QXML_CALL(xml, cb_text, (xml, buf.ptr, buf.len));
            buf = buf_clear(buf);
        }
    }

    ret = 1;
end:
    buf_free(wsbuf);
    buf_free(buf);
    buf_free(keybuf);
    buf_free(valbuf);
    return ret;
error:
    ret = 0;
    goto end;
    return 0; /* should never happen */
}
