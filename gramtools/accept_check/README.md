# accept_check

Check whether a grammar can accept sentences.

## Synopsis

```shell
% accept_check [options] prefix < test_sentences.txt
```

## Description

`accept_check` reads sentences, i.e. word sequences, from standard input one per
line, and check whether they are acceptable or not on a given grammar
(`prefix.dfa` and `prefix.dict`).  You can use this tool to verify whether the
to-be-accepted sentences in a grammar task can actually be accepted in the
developing grammar.

If a given word exists in multiple word categories as same label in a grammar,
in that case `accept_check` checks all the possible category sequence patterns
and determine as accepted if at least one of them is acceptable.

### Prerequisites

The grammar definition in Julius consists of several files: `.dict`, `.dfa`, and
optional `.term`.  They are generated from BNF-like grammar definition by grammar compilation tool `mkdfa.pl`.
See [Julius grammar-kit GitHub](https://github.com/julius-speech/grammar-kit/) for details.  There are also an [example](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en).

A sentence should be given as space-separated word sequence. It may be required
to add head / tail silence word like sil, depending on your grammar. And should
not contain a short-pause word.

### Installing

This tool will be installed together with Julius.

## Usage

Verifying a sentence "`<s> three apples </s>`" against the sample grammar in [grammar-kit/SampleGrammars_en/](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en)

```shell
% git clone https://github.com/julius-speech/grammar-kit/
% cd grammar-kit/SampleGrammars_en
% echo '<s> three apples </s>' | ./accept_check fruit
Stat: init_voca: read 31 words
Reading in term file (optional)...done
8 categories, 31 words
DFA has 8 nodes and 13 arcs
-----
please input word sequence>wseq: <s> three apples </s>
cate: NS_B NUM FRUIT_N NS_E
accepted
```

## Options

### `-t`

Use category name instead of word label in input.

### `-s spname`

Short-pause word name to be skipped. (default: "sp")

### `-v`

Debug output.

## Related tools

- "[nextword](https://github.com/julius-speech/julius/tree/master/gramtools/nextword)"
  can show word prediction of a grammar at given context for debug.
- "[generate](https://github.com/julius-speech/julius/tree/master/gramtools/generate)"
  can generate sentences randomly according to the grammar.
- "[mkdfa.pl](https://github.com/julius-speech/julius/tree/master/gramtools/mkdfa)"
  is the grammar compiler for Julius.
- "[yomi2voca.pl](https://github.com/julius-speech/julius/tree/master/gramtools/yomi2voca)"
  can convert Japanese Hiragana to phoneme for making recognition dictionary.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
