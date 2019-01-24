# binlm2arpa

Convert Julius binary N-gram file back to ARPA format.

## Synopsis

```shell
% binlm2arpa inputFile outputFile_prefix
```

## Description

`binlm2arpa` converts any binary N-gram file for Julius back to the original
standard ARPA format.

### Installing

This tools will be installed together with Julius.

## Usage

Convert binlm file that contains forward 2-gram and backward 3-gram into ARPA
files:

```shell
% binlm2arpa xxx.binlm outputFile
writing reverse 3-gram to "outputFile.rev-3gram.arpa"
writing forward 2-gram to "outputFile.2gram.arpa"
```

Convert binlm of backward 4-gram (and no forward 2-gram) back to ARPA file:

```shell
% binlm2arpa yyy.binlm outputFile
writing reverse 4-gram to "outputFile.rev-4gram.arpa"
```

Convert forward 5-gram binlm into ARPA file:

```shell
% binlm2arpa zzz.binlm outputFile
writing forward 5-gram to "outputFile.ngram.arpa"
```

## Related tools

- "[mkbingram](https://github.com/julius-speech/julius/tree/master/mkbingram)"
  can convert ARPA files into binary N-gram.
- "[generate-ngram](https://github.com/julius-speech/julius/tree/master/generate-ngram)"
  can generate random sentences from binary N-gram.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
