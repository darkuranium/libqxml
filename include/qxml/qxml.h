#ifndef __QXML_H__
#define __QXML_H__

/*
    + done
    * in progress/half-done
    - not yet done
*/
/* +comment: <!--...--> */
/* +cdata: <[CDATA[...]]> */
/* +element: <elem>...</elem> <elem/> */
/* +element-attrib: foo="bar" baz */
/* +xmlDecl: <?xml...?> */
/* -processingInstruction: <?pi...?> */
/* *entity: &foo; &#xxxx; */
/* +characters: text */
/* +whitespace */

#include "file.h"
#include "string.h"
#include "tree.h"

#endif /* __QXML_H__ */
