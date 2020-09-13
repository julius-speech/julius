# mkdfa.py / mkdfa.pl

Grammar compiler for Julius.

## Synopsis

Python 3 version:

```shell
% mkdfa.py [options] prefix
```

Perl version:

```shell
% mkdfa.pl [options] prefix
```

## Description

`mkdfa.pl` compiles Julius grammar definition (BNF-style definition file
`.grammar` and vocabulary file  `.voca`) into finite automaton grammar to be
used in Julius.

The output files, `prefix.dfa` and `prefix.dict`, can be used in Julius as
grammar constrained speech recognition.  If additional forward grammar file
`prefix.dfa.forward` exists, Julius will further enable applying full grammar
on the 1st pass.

`mkdfa.py` is a python 3 version of `mkdfa.pl`, just works as the same.

### Prerequisites

`mkdfa.pl` requires Perl to run.  `mkdfa.py` requires Python 3.x to run.

Requires `mkfa` and `dfa_minimize` executables to be placed on the same
directory.  Additionally `dfa_determinize` is required to generate forward grammar.

See [Julius grammar-kit GitHub](https://github.com/julius-speech/grammar-kit/)
for details about grammars. There are also an
[example](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en).

### Installing

This tool will be installed together with Julius. Place `mkfa` and
`dfa_minimize` at the same directory as `mkdfa.pl` or `mkdfa.py`, since it invokes them
internally while compilation.  Additionally place `dfa_determinize` as the same for generating forward grammar.

## Usage

Compiling the [sample grammar](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en) "fruit":

```shell
% git clone https://github.com/julius-speech/grammar-kit/
% cd grammar-kit/SampleGrammars_en
% mkdfa.pl fruit
fruit.grammar has 8 rules
fruit.voca    has 8 categories and 31 words
---
executing [/usr/local/bin/mkfa -e1 -fg ./tmp24634-rev.grammar -fv ./tmp24634.voca -fo fruit.dfa_beforeminimize -fh ./tmp24634.h]
Now parsing grammar file
Now modifying grammar to reduce states[0]
Now parsing vocabulary file
Now making nondeterministic finite automaton[10/10]
Now making deterministic finite automaton[10/10]
Now making triplet list[10/10]
executing [/usr/local/bin/dfa_minimize fruit.dfa_beforeminimize -o fruit.dfa]
8 categories, 10 nodes, 17 arcs
-> minimized: 8 nodes, 13 arcs
---
now reversing fruit.dfa into NFA "fruit.dfa.forward_nfa"
executing [/usr/local/bin/dfa_determinize fruit.dfa.forward_nfa -o fruit.dfa.forward_beforeminimize]
8 categories, 9 nodes, 13 arcs
-> determinized: 8 nodes, 14 arcs
executing [/usr/local/bin/dfa_minimize fruit.dfa.forward_beforeminimize -o fruit.dfa.forward]
8 categories, 8 nodes, 14 arcs
-> minimized: 8 nodes, 14 arcs
---
generated: fruit.dfa fruit.term fruit.dict fruit.dfa.forward
```

Running Julius with them:

```shell
% julius ... -gram fruit
```

## Options

### `-n`

Skip voca-to-dict conversion. Build `.dfa` from `.grammar` but leave `.dict`
unchanged.

### `-r`

Skip generating forward grammar.

## Related tools

- "[dfa_minimize](https://github.com/julius-speech/julius/tree/master/gramtools/dfa_minimize)"
  can minimize DFA. `mkdfa.pl` and `mkdfa.py` call call this inside the process.
- "[dfa_determinize](https://github.com/julius-speech/julius/tree/master/gramtools/dfa_determinize)"
  can determinize NFA into DFA. `mkdfa.pl` and `mkdfa.py` call call this inside the process.
- "[mkfa](https://github.com/julius-speech/julius/tree/master/gramtools/mkdfa/mkfa-1.44-flex)"
    is the core BNF-to-FSA compiler.  `mkdfa.pl` and `mkdfa.py` call this inside the process.
- "[generate](https://github.com/julius-speech/julius/tree/master/gramtools/generate)"
  can generate sentences randomly according to the grammar.
- "[accept_check](https://github.com/julius-speech/julius/tree/master/gramtools/accept_check)"
  can check sentence acceptance.
- "[nextword](https://github.com/julius-speech/julius/tree/master/gramtools/nextword)"
  can show word prediction of a grammar at given context for debug.

## License

`mkdfa.pl` is distributed under the same license with Julius.  See the license
term of Julius for details.

`mkdfa.py` is a contributed script, distributed under the MIT license:

> Copyright (c) 2019 Sunao Hara.
> This script is released under the MIT license.
> [https://opensource.org/licenses/mit-license.php](https://opensource.org/licenses/mit-license.php)
>
> Original Site:
> [https://gist.github.com/naoh16/eabd11ed010b450963c108b2436eac4f](https://gist.github.com/naoh16/eabd11ed010b450963c108b2436eac4f)
