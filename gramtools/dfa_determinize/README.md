# dfa_determinize

Determinize an automaton grammar network.

## Synopsis

```shell
% dfa_determinize [-o outFile] nfaFile
```

## Description

`dfa_determinize` converts a non-deterministic automaton grammar file into an
equivalent deterministic form.

Description about grammars in Julius is available at [Julius grammar-kit
GitHub](https://github.com/julius-speech/grammar-kit/).  There is also an [tiny
example of
grammar](https://github.com/julius-speech/grammar-kit/tree/master/SampleGrammars_en).

### Installing

This tool will be installed together with Julius.

## Usage

Determinizing a non-deterministic grammar automaton into deterministic form:

```shell
% dfa_determinize -o outFile inFile
```

## Options

### `-o outFile`

Specify output file. (default: output to standard output)

## Related tools

- "[dfa_minimize](https://github.com/julius-speech/julius/tree/master/gramtools/dfa_minimize)"
  can minimize DFA.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
