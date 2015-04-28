#!/bin/sh
#
# update all 00readme.txt and 00readme-ja.txt of the tools from each manuals
#
# should be invoked at parent directory.
#
# The new manuals should be located at man and man/ja.
#
# If conversion fails, see makeman.sh in this directory.
#

echo 'Did you placed all the new manuals in "man" and "man/ja/" ?'

./support/makeman.sh accept_check gramtools/accept_check
./support/makeman.sh adinrec adinrec
./support/makeman.sh adintool adintool
./support/makeman.sh dfa_determinize gramtools/dfa_determinize
./support/makeman.sh dfa_minimize gramtools/dfa_minimize
./support/makeman.sh generate-ngram generate-ngram 
./support/makeman.sh generate gramtools/generate
./support/makeman.sh gram2sapixml.pl gramtools/gram2sapixml
./support/makeman.sh jclient.pl jclient-perl
./support/makeman.sh jcontrol jcontrol
./support/makeman.sh julius julius
./support/makeman.sh mkbingram mkbingram
./support/makeman.sh mkbinhmmlist mkbinhmm
mv mkbinhmm/00readme.txt mkbinhmm/00readme-mkbinhmmlist.txt
mv mkbinhmm/00readme-ja.txt mkbinhmm/00readme-mkbinhmmlist-ja.txt
./support/makeman.sh mkbinhmm mkbinhmm
./support/makeman.sh mkdfa.pl gramtools/mkdfa
./support/makeman.sh mkgshmm mkgshmm
./support/makeman.sh mkss mkss
./support/makeman.sh nextword gramtools/nextword

echo Finished.
