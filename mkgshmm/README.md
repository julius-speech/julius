# mkgshmm

Make mixture PDF definition file from monophone HMM for Gaussian mixture
selection.

## Synopsis

```shell
% mkgshmm monophoneHmmdefs > outputFile
```

## Description

`mkgshmm` extracts mixture pdf from the given monophone HMM definition, to be
used for Gaussian mixture selection in Julius.

### Prerequisites

Gaussian mixture selection mode in Julius applied the monophone HMM mixture pdf
scores as fallback value of outlier triphone.  Thus the monophone and triphone
should have the same base phone.

### Installing

This tools will be installed together with Julius.

## Usage

```shell
% mkgshmm monophone.hmmdefs > outputFile
% julius ... -gshmm outputFile
```

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
