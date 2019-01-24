# generate-ngram

An N-gram-to-text sentence generator.

## Synopsis

```shell
% generate-ngram [options] binary_ngram_file
```

## Description

`generate-ngram` generates sentences randomly according to the given N-gram
language model.  The N-gram file should be a binary N-gram file for Julius.

You can convert ARPA standard format LM to Julius binary N-gram by `mkbingram`.

`generate-ngram` generates a sequence left-to-right, beginning with EOS `<s>`
and stops when an EOS `</s>` was generated. It continues generation of sentences
until unique N sentences has been generated (N=10 by default).

### Installing

This tool will be installed together with Julius.

## Usage

Generate 10 sentences:

```shell
% mkbingram bingram_file
```

Generate 10000 sentences:

```shell
% mkbingram -n 10000 bingram_file
```

Limit the usage of N-gram entries by the length of 3:

```shell
% mkbingram -N 3 bingram_file
```

Alter the beginning-of-sentence label and end-of-sentence label:

```shell
% mkbingram -bos BOS -eos EOS my_bingram_file
```

Exclude `<UNK>` from generation:

```shell
% mkbingram -ignore "<UNK>"
```

## Options

### -n num

Number of sentences to generate (default: `10`)

### -N

N-gram to use (default: available max in the given model)

### -bos

Beginning-of-sentence word label (default: `<s>`)

### -eos

End-of-sentence word label (default: `</s>`)

### -ignore

A word to be excluded at generation (default: `<UNK>`)

### -v

Verbose output.

### -debug

Debug output.

## Related tools

- "[mkbingram](https://github.com/julius-speech/julius/tree/master/mkbingram)"
  can convert ARPA files into binary N-gram.
- "[binlm2arpa](https://github.com/julius-speech/julius/tree/master/binlm2arpa)"
  is can revert binary N-gram into its original ARPA format.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
