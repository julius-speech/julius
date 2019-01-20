# binlm2arpa

Convert Julius binary N-gram file back to ARPA format.

## Synopsys

```shell
% binlm2arpa infile outfile_prefix
```

## Description

`binlm2arpa` converts any binary N-gram file for Julius back to the original standard ARPA format.

### Installing

This tools will be installed together with Julius.

## Usage

Convert binlm file that contains forward 2-gram and backward 3-gram into ARPA files:

```shell
% binlm2arpa xxx.binlm outfile
writing reverse 3-gram to "outfile.rev-3gram.arpa"
writing forward 2-gram to "outfile.2gram.arpa"
```

Convert binlm of backward 4-gram (and no forward 2-gram) back to ARPA file:

```shell
% binlm2arpa yyy.binlm outfile
writing reverse 4-gram to "outfile.rev-4gram.arpa"
```

Convert forward 5-gram binlm into ARPA file:

```shell
% binlm2arpa zzz.binlm outfile
writing forward 5-gram to "outfile.ngram.arpa"
```

## Options

No option.

## License

This tool is licensed under the same license with Julius.  See the license term of Julius for details.