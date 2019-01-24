# dfa_minimize

Minimize a DFA grammar network.

## Synopsis

```shell
% dfa_minimize [-o outFile] dfaFile
```

## Description

`dfa_minimize` converts .dfa file into an equivalent minimal form.  This tool will be automatically invoked inside grammar compilation process in `mkdfa.pl`,

Description about grammars in Julius is available at [Julius grammar-kit
GitHub](https://github.com/julius-speech/grammar-kit/).  There is also an [tiny
example of
grammar](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en).

### Installing

This tool will be installed together with Julius.

## Usage

Minimize a dfa file:

```shell
% dfa_minimize -o out.dfa in.dfa
8 categories, 10 nodes, 17 arcs
-> minimized: 8 nodes, 13 arcs
```

## Options

### `-o outFile`

Specify output file. (default: output to standard output)

## Related tools

- "[mkdfa.pl](https://github.com/julius-speech/julius/tree/master/gramtools/mkdfa)"
  is the grammar compiler for Julius.  `dfa_minimize` is called within this
  script.
- "[dfa_determinize](https://github.com/julius-speech/julius/tree/master/gramtools/dfa_determinize)"
  can determinize DFA.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
