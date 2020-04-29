#!/usr/bin/env python3
#
# -*- coding: utf-8 -*-
"""
Implementation of mkdfa.pl for python 3
"""

import sys
import os
import shutil
import tempfile
import re
import codecs
import subprocess

JULIUS_BIN = os.path.dirname(os.path.abspath(__file__))
if sys.platform == 'win32':
    CMD_MKFA = JULIUS_BIN + '/mkfa.exe'
    CMD_DFA_MINIMIZE = JULIUS_BIN + '/dfa_minimize.exe'
else:
    CMD_MKFA = JULIUS_BIN + '/mkfa'
    CMD_DFA_MINIMIZE = JULIUS_BIN + '/dfa_minimize'

# generate reverse grammar file
def gen_reverse_grammar(gramfile, rgramfile):
    results = []
    with codecs.open(gramfile, "r", 'utf-8') as fin:
        n = 0
        for line in fin:
            if line.find('#') >= 0:
                line = line[line.find('#'):]

            if line.find(':') == -1:
                continue

            try:
                (left, right) = line.split(':', 1)

                right_list = re.split(r' +', right.strip())
                right_list.reverse()
                #print("%s:%s" % (left, ' '.join(right_list)))
                results.append( left + ':' + ' '.join(right_list) )
                n = n + 1
            except:
                pass

    with open(rgramfile, "w") as fout:
        for line in results:
            fout.write(line + '\n')

    print("%s has %d rules" % (gramfile, n))

def extract_vocafile(src, catefile, termfile):
    n1 = 0
    n2 = 0
    categories = []

    with codecs.open(src, "r", 'utf-8') as fin:
        for line in fin:
            line = re.sub('#.*$', "", line)
            if len(line.strip()) == 0:
                continue
            if line.find('%') == 0:
                category = line[1:]
                categories.append(category.strip())
                n1 = n1 + 1
            else:
                n2 = n2 + 1

    with codecs.open(catefile, "w", 'utf-8') as fout:
        for line in categories:
            print("#%s" % line, file=fout)

    with codecs.open(termfile, "w", 'utf-8') as fout:
        termid = 0
        for line in categories:
            print("%d\t%s" % (termid, line), file=fout)
            termid = termid + 1

    print("%s has %d categories and %d words" % (vocafile, n1, n2))

def vocafile2dictfile(vocafile, dictfile):
    with codecs.open(vocafile, "r", 'utf-8') as fin:
        with codecs.open(dictfile, "w", 'utf-8') as fout:
            id = -1
            for line in fin:
                line = re.sub('#.*$', "", line)
                line = line.strip()
                if len(line) == 0:
                    continue

                if line.find('%') == 0:
                    id = id + 1
                    continue

                (name, phones) = re.split(r'[ \t]+', line, 1)
                print("%d\t[%s]\t%s" % (id, name, phones), file=fout)


def call_mkfa(rgramfile, tmpvocafile, dfafile, tmpprefix):
    cmd = [CMD_MKFA, '-e1']
    cmd = cmd + ['-fg', rgramfile]
    cmd = cmd + ['-fv', tmpvocafile]
    cmd = cmd + ['-fo', tmpprefix + '.dfa']
    cmd = cmd + ['-fh', tmpprefix + '.h']

    subprocess.run(cmd)

    print("---")
    if os.path.exists(CMD_DFA_MINIMIZE):
        cmd = [CMD_DFA_MINIMIZE, tmpprefix + '.dfa', '-o', dfafile]
        subprocess.run(cmd)
    else:
        print("Warning: dfa_minimize not found in the same place as mkdfa.py")
        print("Warning: no minimization performed")
        shutil.copyfile(tmpprefix + '.dfa', dfafile)

    os.unlink(tmpprefix + '.dfa')
    os.unlink(tmpprefix + '.h')

if __name__ == '__main__':

    if len(sys.argv) < 2:
        print("mkdfa.py --- DFA compiler for python 3")
        print("usage: mkdfa.py [-n] prefix")
        print("\t-n ... keep current dict, not generate")
        sys.exit(1)

    if (os.path.exists(CMD_MKFA) == False):
        print("Error: \"mkfa\" not found on the same directory of mkdfa.py")
        sys.exit(1)
    if (os.path.exists(CMD_DFA_MINIMIZE) == False):
        print("Warning: \"dfa_minimize\" not found in the same directory as mkdfa.py")
        print("Warning: no minimization performed")

    corename = sys.argv[1]

    gramfile = corename + ".grammar"
    vocafile = corename + ".voca"
    dfafile  = corename + ".dfa"
    dictfile = corename + ".dict"
    termfile = corename + ".term"
    fdfafile = corename + ".dfa.forward"

    tmpprefix   = tempfile.gettempdir() + "/_julius_mkdfa_tmp_"
    tmpvocafile = tmpprefix + ".voca"
    rgramfile = tmpprefix + ".grammar"

    # generate reverse grammar file
    gen_reverse_grammar(gramfile, rgramfile)

    # make temporary voca for mkfa (include only category info)
    extract_vocafile(vocafile, tmpvocafile, termfile)

    print('---')

    # call mkfa to generate .dfa from reversed grammar (and minimize) 
    call_mkfa(rgramfile, tmpvocafile, dfafile, tmpprefix)
    
    # call mkfa to generate .dfa_forward from original grammar (and minimize) 
    call_mkfa(gramfile, tmpvocafile, fdfafile, tmpprefix)

    vocafile2dictfile(vocafile, dictfile)

    print('---')
    print("generated %s %s %s %s" % (dfafile, termfile, dictfile, fdfafile))

    os.unlink(tmpvocafile)
    os.unlink(rgramfile)
