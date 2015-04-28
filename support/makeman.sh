#!/bin/sh
#
# Update 00readme.txt and 00readme-ja.txt from man file.
# 
# usage: makeman.sh generate gramtools/generate
# 
#
nroff -man man/$1.1 | sed -e 's/.//g' | nkf -c > $2/00readme.txt
groff -Dutf8 -Tutf8 -mandoc -mja -E man/ja/$1.1 | sed -e 's/.//g' | nkf -s -c > $2/00readme-ja.txt
echo $1 "->" $2


