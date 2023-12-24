#!/bin/sh

groff -t -e -mandoc -Tascii smdb.3 | col -bx > smdb.txt
groff -t -e -mandoc -Tps smdb.3 > smdb.ps
man2html < smdb.3 | sed 's/<BODY>/<BODY text="#0000FF" bgcolor="#FFFFFF" style="font-family: monospace;">/' > smdb.html

