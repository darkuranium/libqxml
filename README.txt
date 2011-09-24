QuickXML is a simple, lightweight XML-parsing library made in ANSI C, meant
for embedding into projects which need XML-parsing, but don't quite need to
support the whole specification.
It has two modes of access, event-driven and tree-based.

The following things are handled:
- elements: <foo></foo> <bar/>
- attributes: <elem foo="bar" baz>
- cdata sections: <![CDATA[...]]>
- comments (not quite as per spec, but close): <!--comment-->
- xml declaration: <?xml ... ?>
- XML entities: quot, apos, lt, gt, amp - note: &#xxxx; are not *yet* supported

Handling of processing instructions - for use with,
say, <?php ... ?> - is planned.

Note that the library supports multiple document roots and text in root, as in:
<?xml ... ?>
<foo>...</foo>
text!
<bar>...</bar>

It is released under the 2-clause BSD license - for more info,
see the file COPYING.txt.
