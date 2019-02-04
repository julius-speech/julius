# mkbingram

Make binary N-gram from ARPA N-gram file.

## Synopsis

```shell
% mkbingram [-nlr forward_ngram.arpa] [-nrl backward_ngram.arpa] [-d old_bingram_file] output_bingram_file
```

## Description

`mkbingram` converts ARPA N-gram definition file(s) to Julius binary N-gram
file.  Binary N-gram file is a compact binary representation of N-gram model for
Julius.  Using binary N-gram will greatly speed up Julius's startup.

Forward (left-to-right) N-gram, backward (right-to-left) N-gram can be
converted, and additional forward 2-gram for the first decoding pass of Julius
can be also compiled together into a single binary file.  The allowed
combinations of N-gram to be passed to `mkbingram` are:

- forward N-gram only
- backward N-gram only
- forward 2-gram and backward N-gram

With a single forward / backward N-gram, `mkbingram` will convert it into a
binary N-gram file.  In Julius, only the 2-gram part of the N-gram will be used
at the first decoding pass, and the entire N-gram will be applied at the second
decoding pass.

When both forward 2-gram and backward N-gram are given, `mkbingram` will pack
them into a single binary file.  In Julius, the forward 2-gram part will be
applied at the first pass, and the full N-gram part at the second pass.

### Prerequisites

Packing of forward 2-gram and backward N-gram shares its structure indices
inside binary file, so the both N-gram should be trained in the same corpus with
same parameters (i.e. cut-off thresholds), and with the same vocabulary.

### Installing

This tools will be installed together with Julius.

## Usage

Forward N-gram into binary format:

```shell
% mkbingram -nlr forwardNgramFile output.bingram
```

Backward N-gram into binary format:

```shell
% mkbingram -nrl backwardNgramFile output.bingram
```

Forward 2-gram and backward N-gram, being packed into a single bingram:

```shell
% mkbingram -nlr forward2gramFile -nrl backwardNgramFile output.bingram
```

Convert old (3.x) binary to new (4.x and later) format

```shell
% mkbingram -d old.bingram new.bingram
```

Convert text encoding while conversion using `-c` option:

```shell
% mkbingram -nlr forwardNgramFile -c sjis utf-8 output.bingram
```

Also you can convert text encoding inside binary N-gram:

```shell
% mkbingram -d old.bingram -c sjis utf-8 new.bingram
```

## Options

### `-nlr forward_ngram.arpa`

Read in a forward (left-to-right) word N-gram file in ARPA standard
format.

### `-nrl backward_ngram.arpa`

Read in a backward (right-to-left) word N-gram file in ARPA
standard format.

### `-d old_bingram_file`

Read in a binary N-gram file.

### `-swap`

Swap BOS word `<s>` and EOS word `</s>` in N-gram.

### `-c from to`

Convert character code in binary N-gram.

## Related Tools

## Related tools

- "[binlm2arpa](https://github.com/julius-speech/julius/tree/master/binlm2arpa)"
  is can revert binary N-gram into its original ARPA format.
- "[generate-ngram](https://github.com/julius-speech/julius/tree/master/generate-ngram)"
  can generate random sentences from binary N-gram.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
