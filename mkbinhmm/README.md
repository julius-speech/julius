# mkbinhmm, mkbinhmmlist

Make binary HMM and binary HMM list.

## Synopsis

```shell
% mkbinhmm [-htkconf HTKConfigFile] hmmdefsFile binHMMFile
```

```shell
% mkbinhmmlist hmmdefsFile hmmListFile binHMMListFile
```

## Description

`mkbinhmm` converts an HMM definition file in HTK ascii format into a binary HMM
file for Julius. It will greatly speed up the launching process of Julius.

`mkbinhmm` can embed acoustic analysis condition parameters needed for
recognition into the binary file.  The embedded parameters in a binary HMM
format will be loaded into Julius automatically, so you do not need to specify
the acoustic feature options at run time. It will be convenient when you deliver
an acoustic model.

`mkbinhmmlist` converts a HMMList file to binary format, with the index trees
for lookup embedded. It will also speeds up the startup of
Julius, namely when using big HMMList file.

The binary files above can be used in Julius as the same manner with their
original format: `-h` for HMM definition and `-hlist` for HMMList.  Julius will
auto-detect whether the given models are text or binary.

### Prerequisites

The binary HMMList file converted by `mkbinhmmlist` will work only with the HMM
definition being specified at conversion, since static hard-coded reference
index toward the HMM model names will be embedded into the binary at conversion
time.

### Installing

This tools will be installed together with Julius.

## Usage

Convert HMM definition in HTK ascii format into binary form:

```shell
% mkbinhmm hmmdefsFile output.binhmm
```

Conversion with acoustic feature parameter embedding:

```shell
% mkbinhmm -htkconf Config hmmdefsFile output.binhmm
```

Convert HMM List file into binary: the `hmmdefsFile` should be the HMM
definition file that will be used with the target HMM List at recognition in
Julius.

```shell
% mkbinhmmlist hmmdefsFile HMMListFile output.binhmmlist
```

The converted files can be used as the same as original:

```shell
% julius ... -h output.binhmm -hlist output.binhmmlist ...
```

## Options

### `-htkconf HTKConfigFile`

(mkbingram)  HTK Config file you used at HMM training time. If specified, the
values are embedded to the output file.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
