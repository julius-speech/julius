#!/usr/bin/env python3
#
# -*- coding: utf-8 -*-
"""
Implementation of mkdfa.pl for python 3
"""

import sys
import os
import shutil
import re
import codecs
import subprocess
import argparse

JULIUS_BIN = os.path.dirname(os.path.abspath(__file__))
if sys.platform == 'win32':
    CMD_MKFA = JULIUS_BIN + '/mkfa.exe'
    CMD_DFA_MINIMIZE = JULIUS_BIN + '/dfa_minimize.exe'
    CMD_DFA_DETERMINIZE = JULIUS_BIN + '/dfa_determinize.exe'
else:
    CMD_MKFA = JULIUS_BIN + '/mkfa'
    CMD_DFA_MINIMIZE = JULIUS_BIN + '/dfa_minimize'
    CMD_DFA_DETERMINIZE = JULIUS_BIN + '/dfa_determinize'

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
    cmd = [CMD_DFA_MINIMIZE, tmpprefix + '.dfa', '-o', dfafile]
    subprocess.run(cmd)

    os.unlink(tmpprefix + '.dfa')
    os.unlink(tmpprefix + '.h')

def make_forward_dfa(dfafile, fdfafile, tmpprefix):

    fdfafile_nfa = fdfafile + '_nfa'
    fdfafile_beforeminimize = fdfafile + '_beforeminimize'

    endstates = {}
    dfamax = -1;
    with codecs.open(dfafile, "r", 'utf-8') as fin:
        for line in fin:
            elem = list(map(int, line.split()))
            if (elem[1] == -1 and elem[2] == -1):
                endstates[elem[0]] = 1
            if (dfamax < elem[0]):
                dfamax = elem[0]
    dfamax += 1
    with codecs.open(fdfafile_nfa, "w", 'utf-8') as fout:
        print("%d -1 -1 1 0" % (dfamax), file=fout)
        with codecs.open(dfafile, "r", 'utf-8') as fin:
            for line in fin:
                elem = list(map(int, line.split()))
                if (elem[1] == -1 and elem[2] == -1):
                    continue
                if (elem[0] == 0):
                    elem[0] = dfamax
                if (elem[2] == 0):
                    elem[2] = dfamax
                if (elem[0] != dfamax and endstates.get(elem[0])):
                    elem[0] = 0
                if (elem[2] != dfamax and endstates.get(elem[2])):
                    elem[2] = 0
                print("%d %d %d 0 0" % (elem[2], elem[1], elem[0]), file=fout)

    cmd = [CMD_DFA_DETERMINIZE, fdfafile_nfa, '-o', fdfafile_beforeminimize]
    subprocess.run(cmd)

    cmd = [CMD_DFA_MINIMIZE, fdfafile_beforeminimize, '-o', fdfafile]
    subprocess.run(cmd)

    os.unlink(fdfafile_nfa)
    os.unlink(fdfafile_beforeminimize)


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("prefix", help = "grammar file prefix")
    parser.add_argument("-n", help = "keep current dict, not generate", action="store_true")
    parser.add_argument("-r", help = "not generate forward grammar", action="store_true")
    args = parser.parse_args()

    flag = False;
    for p in [CMD_MKFA, CMD_DFA_MINIMIZE, CMD_DFA_DETERMINIZE]:
        if (os.path.exists(p) == False):
            print("Error: %s not found" % (p), file=sys.stderr)
            flag = True
    if (flag):
        sys.exit(1)

    corename = args.prefix

    gramfile = corename + ".grammar"
    vocafile = corename + ".voca"
    dfafile  = corename + ".dfa"
    dictfile = corename + ".dict"
    termfile = corename + ".term"
    fdfafile = corename + ".dfa.forward"

    tmpprefix   = "./_julius_mkdfa_tmp_"
    tmpvocafile = tmpprefix + ".voca"
    rgramfile = tmpprefix + ".grammar"

    generated = ""

    # generate reverse grammar file
    gen_reverse_grammar(gramfile, rgramfile)

    # make temporary voca for mkfa (include only category info)
    extract_vocafile(vocafile, tmpvocafile, termfile)

    print('---')

    # call mkfa to generate .dfa from reversed grammar (and minimize) 
    call_mkfa(rgramfile, tmpvocafile, dfafile, tmpprefix)

    generated = dfafile + " " + termfile
    
    # generate .dfa_forward from original grammar
    if (args.r == False):
        make_forward_dfa(dfafile, fdfafile, tmpprefix)
        generated += " " + fdfafile

    if (args.n == False):
        vocafile2dictfile(vocafile, dictfile)
        generated += " " + dictfile

    print('---')
    print("generated " + generated)

    os.unlink(tmpvocafile)
    os.unlink(rgramfile)
