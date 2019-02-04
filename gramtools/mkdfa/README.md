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

The output files, `prefix.dfa` and `prefix.dict`, can be read by Julius to
perform grammar-based recognition based on the given grammar.

`mkdfa.py` is a python 3 version of `mkdfa.pl`, just works as the same.

### Prerequisites

`mkdfa.pl` requires Perl to run.  `mkdfa.py` requires Python 3.x to run.

Both of them requires `mkfa` and `dfa_minimize` to be placed on the same
directory of `mkdfa.pl`.

See [Julius grammar-kit GitHub](https://github.com/julius-speech/grammar-kit/)
for details about grammars. There are also an
[example](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en).

### Installing

This tool will be installed together with Julius. Place `mkfa` and
`dfa_minimize` at the same directory as `mkdfa.pl` or `mkdfa.py`, since it invokes them
internally while compilation.

## Usage

Compiling the [sample
grammar](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en)
"fruit":

```shell
% git clone https://github.com/julius-speech/grammar-kit/
% cd grammar-kit/SampleGrammars_en
% mkdfa.py fruit
fruit.grammar has 8 rules
fruit.voca    has 8 categories and 31 words
---
Now parsing grammar file
Now modifying grammar to minimize states[0]
Now parsing vocabulary file
Now making nondeterministic finite automaton[10/10]
Now making deterministic finite automaton[10/10]
Now making triplet list[10/10]
8 categories, 10 nodes, 17 arcs
-> minimized: 8 nodes, 13 arcs
---
generated: fruit.dfa fruit.term fruit.dict
```

Running Julius with them:

```shell
% julius ... -dfa fruit.dfa -v fruit.dict
```

or

```shell
% julius ... -gram fruit
```

## Options

### `-n`

Skip voca-to-dict conversion. Build `.dfa` from `.grammar` but leave `.dict`
unchanged.

## Environment Variables

### TMP / TEMP

Temporary directory for `mkdfa.pl`.  When not specified, one from the following
list will be used:

- /tmp
- /var/tmp
- /WINDOWS/Temp
- /WINNT/Temp.

`mkdfa.py` does not use environment value.  It uses `tempfile.gettempdir()` to
get temporary directory.

## Related tools

- "[dfa_minimize](https://github.com/julius-speech/julius/tree/master/gramtools/dfa_minimize)"
  can minimize DFA. `mkdfa.pl` and `mkdfa.py` call call this inside the process.
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
