# nextword

Another grammar inspector: list predicted next words in a grammar on a given
context.

## Synopsis

```shell
% nextword [-t] -[-r] -[-s spname] [-v] prefix
```

## Description

`nextword` parses a given partial (part of) sentence, and show the next word
candidates in the specified grammar (`prefix.dfa` and `prefix.dict`).  It can be
used to see how the given grammar predicts words at the context.

**It's backward!** Since the second recognition pass of Julius is a backward
process, the direction of compiled recognition grammar (`.dfa`) is reversed.  So
when verifying grammar for Julius, you should give a latter part of the sentence
and will get predicted word in reverse direction.  See the Usage below
carefully.

### Prerequisites

The grammar definition in Julius consists of several files: `.dict`, `.dfa`, and
optional `.term`.  They are generated from BNF-like grammar definition by
grammar compilation tool `mkdfa.pl`. See [Julius grammar-kit
GitHub](https://github.com/julius-speech/grammar-kit/) for details.  There are
also an
[example](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en).

A sentence should be given as space-separated word sequence. It may be required
to add head / tail silence word like sil, depending on your grammar.

### Installing

This tool will be installed together with Julius.

## Usage

Test run on a shell, using sample grammar in
[grammar-kit/SampleGrammars_en/](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en)

```shell
% git clone https://github.com/julius-speech/grammar-kit/
% cd grammar-kit/SampleGrammars_en
% nextword SampleGrammars_en/fruit
Stat: init_voca: read 31 words
Reading in term file (optional)...done
8 categories, 31 words
DFA has 8 nodes and 13 arcs
-----
command completion is disabled
-----
wseq >
```

Type "`banana </s>`" to the prompt, and Enter:

```shell
wseq > banana </s>
[wseq: banana </s>]
[cate: FRUIT_N_1 NS_E]
PREDICTED CATEGORIES/WORDS:
                  TAKE_V (I'lltake I'llhave )
                     HMM (FILLER FILLER )
                    NS_B (<s> )
```

Another test: type "`bananas </s>`" and Enter, and see how the word prediction changes:

```shell
wseq > bananas </s>
[wseq: bananas </s>]
[cate: FRUIT_N NS_E]
PREDICTED CATEGORIES/WORDS:
                     NUM (one two three ...)
```

## Options

### `-t`

Use category name instead of word label in input.

### `-r`

Read input word sequence as reverse order.

### `-s spname`

Short-pause word name to be skipped. (default: "sp")

### `-v`

Debug output.

## Related tools

- "[accept_check](https://github.com/julius-speech/julius/tree/master/gramtools/accept_check)"
  can check sentence acceptance.
- "[generate](https://github.com/julius-speech/julius/tree/master/gramtools/generate)"
  can generate sentences randomly according to the grammar.
- "[mkdfa.pl](https://github.com/julius-speech/julius/tree/master/gramtools/mkdfa)"
  is the grammar compiler for Julius.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
