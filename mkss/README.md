# mkss

Calculate average spectrum of microphone input and save to file for spectral
subtraction.

## Synopsis

```shell
% mkss [options...] fileName
```

## Description

`mkss` is a tool to estimate average spectrum of microphone input.  The output
file can be used for spectral subtraction on Julius.

It reads 3 seconds (the length can be changed by option) of audio data from
microphone input, calculate its average spectrum and save it to a file. The
output file can be used as (initial) noise spectrum data in Julius (option
"-ssload").

The recording will start immediately after startup. Sampling format is 16bit,
monaural. If output file already exist, it will be overridden.

### Prerequisites

You need one audio capture device on your machine.  If several devices are
available, the default one will be used.

### Installing

This tool will be installed together with Julius.

## Usage

Record 3 seconds of audio data from microphone, save average spectrum to
`noise.ss`, and use it in Julius

```shell
% mkss noise.ss
% julius ... -ssload noise.ss
```

Change recording length to 5 seconds

```shell
% mkss -len 5000 noise.ss
```

When the Julius uses non-default frame size or frame shift, you should also
specify the same parameter to `mkss`:

```shell
% mkss -fsize 450 -fshift 80 noise.ss
```

## Options

### `-freq Hz`

Sampling frequency in Hz (default: 16,000)

### `-len msec`

Capture length in milliseconds (default: 3000)

### `-fsize sampleNum`

Frame size in number of samples (default: 400)

### `-fshift sampleNum`

Frame shift in number of samples (default: 160)

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
