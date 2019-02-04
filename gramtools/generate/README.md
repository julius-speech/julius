# generate

Grammar-based random sentence generator

## Synopsis

```shell
% generate [-v] [-t] [-n num] [-s spname] prefix
```

## Description

`generate` randomly generates sentences according to the given grammar
(`prefix.dfa` and `prefix.dict`).  Viewing the generated sentence will help you
to intuitively verify its coverage by assessing whether it generates wrong
sentences or not.

### Prerequisites

The grammar definition for Julius consists of several files: `.dict`, `.dfa`, and
optional `.term`.  They are generated from BNF-like grammar definition by
grammar compilation tool `mkdfa.pl`. See [Julius grammar-kit
GitHub](https://github.com/julius-speech/grammar-kit/) for details.  There are
also an
[example](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en).

### Installing

This tool will be installed together with Julius.

## Usage

An example of generating 10 sentences out of [sample
grammar](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en):

```shell
% git clone https://github.com/julius-speech/grammar-kit/
% cd grammar-kit
% generate -n 10 SampleGrammars_en/fruit
Stat: init_voca: read 31 words
Reading in term file (optional)...done
8 categories, 31 words
DFA has 10 nodes and 17 arcs
-----
 <s> FILLER I'llhave apple </s>
 <s> FILLER I'lltake orange </s>
 <s> FILLER I'llhave plum </s>
 <s> twelve apples </s>
 <s> FILLER grape </s>
 <s> six apples </s>
 <s> FILLER I'llhave apple </s>
 <s> FILLER I'llhave six bananas </s>
 <s> FILLER nine plums </s>
 <s> FILLER I'lltake twelve oranges </s>
```

## Options

### `-n`

Number of sentences to be generated. (default: 10)

### `-t`

Use category name instead of word label in input.

### `-s spname`

Short-pause word name to be suppressed. (default: "sp")

### `-v`

Debug output.

## Related tools

- "[accept_check](https://github.com/julius-speech/julius/tree/master/gramtools/accept_check)"
  can check sentence acceptance.
- "[nextword](https://github.com/julius-speech/julius/tree/master/gramtools/nextword)"
  can show word prediction of a grammar at given context for debug.
- "[mkdfa.pl](https://github.com/julius-speech/julius/tree/master/gramtools/mkdfa)"
  is the grammar compiler for Julius.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
