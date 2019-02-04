# Julius

Open source multi-purpose LVCSR engine.

## Synopsis

```shell
% julius [-C jconffile] [options...]
```

## Description

`julius` is a high-performance, multi-purpose, open-source speech recognition
engine for researchers and developers. It is capable of performing almost
real-time recognition of continuous speech with over 60k-word N-gram language
model and triphone HMM model, on most current PCs.  `julius` can perform
recognition on audio files, live microphone input, network input and feature
parameter files.

Recognition algorithm of `julius` is based on a two-pass strategy. Word 2-gram
and reverse word N-gram is used on the respective passes. The entire input is
processed on the first pass, and again the final searching process is performed
again for the input, using the result of the first pass to narrow the search
space. Specifically, the recognition algorithm is based on a tree-trellis
heuristic search combined with left-to-right frame-synchronous beam search and
right-to-left stack decoding search.

When using context dependent phones (triphones), inter-word contexts are taken
into consideration. For tied-mixture and phonetic tied-mixture models,
high-speed acoustic likelihood calculation is possible using gaussian pruning.

For more details, see the documents in [doc/](https://github.com/julius-speech/julius/blob/master/doc/).

### Prerequisites

- Language model and acoustic model is required to run Julius.
- Audio capture device is required if you want to recognize a live audio input.
  If several devices are found, the default one will be used.

### Installing

Julius has autoconf-based configuration script.  See the installation
instruction at the [top directory](https://github.com/julius-speech/julius) for
detailed instruction.

## Models

A brief summary of models to be used with Julius is described here.  Standard
models for some languages are also provided as "dictation kit" on Julius site.

### Acoustic model

Sub-word GMM-HMM (Hidden Markov Model) and DNN-HMM in HTK ascii format are supported. Phoneme
models (monophone), context dependent phoneme models (triphone), tied-mixture
and phonetic tied-mixture models of any unit can be used. When using context
dependent models, inter-word context dependency is also handled. Multi-stream
feature and MSD-HMM is also supported.

Supported base features are `FBANK` (log-scale filterbank parameter), `MELSPEC`
(mel-scale filterbank parameter) and `MFCC` (mel-frequency cepstral
coefficients). All qualifiers and parameters are HTK-compatible.  You can also
run recognition with an acoustic HMM trained with other features, by giving the
input in HTK parameter file of the same feature type.

You can convert HMM definition to binary form for faster startup by
[mkbinhmm](mkbinhmm) and [mkbinhmmlist](mkbinhmm)

Julius can perform DNN-HMM based recognition in two ways: standalone (read and
compute DNN inside Julius) and network-based (receive computed state
probabilities from other process via socket).  See
[00readme-DNN.txt](../00readme-DNN.txt) for details.

### Word N-gram LM

Word N-gram language model, up to 10-gram, is supported. `julius` uses different
N-gram for each pass: left-to-right 2-gram on 1st pass, and right-to-left N-gram
on 2nd pass. It is recommended to use both LR 2-gram and RL N-gram for `julius`.
However, you can use only single LR N-gram or RL N-gram. In such case,
approximated LR 2-gram computed from the given N-gram will be applied at the
first pass.

The Standard ARPA format is supported. In addition, a binary format is also
supported for efficiency. The tool [mkbingram](mkbingram) can convert ARPA
format N-gram to binary format.

### Grammar LM

 `julius`'s grammar format is an original one, and tools to create a recognition
 grammar are included in the distribution. A grammar consists of two files: one
 is a 'grammar' file that describes sentence structures in a BNF style, using
 word 'category' name as terminate symbols. Another is a 'voca' file that
 defines words with its pronunciations (i.e. phoneme sequences) for each
 category. They should be converted by [mkdfa.pl](gramtools/mkdfa) to a
 deterministic finite automaton file (.dfa) and a dictionary file (.dict),
 respectively. You can also use multiple grammars.

### Isolated word LM

You can perform isolated word recognition using only word dictionary. With this
model type, `julius` will perform rapid one pass recognition with static context
handling. Silence models will be added at both head and tail of each word. You
can also use multiple dictionaries in a process.

## Usage

Start recognition with a config file:

```shell
% julius -C foobar.jconf
```

## Options

See [Options.md](../doc/Options.md).

## Environment Variables

### ALSADEV

device name string for ALSA (default: "`default`")

### AUDIODEV

device name string for OSS (default: "/dev/dsp")

### PORTAUDIO_DEV

With portaudio, index of capture device to use. The available devices will be
listed at startup.

### LATENCY_MSEC

Input latency of microphone input in milliseconds. Smaller value will shorten
latency but sometimes make process unstable. Default value will depend on the
running OS.

## Authors

Julius is mainly developed and maintained by Akinobu LEE (Kyoto University, now
at Nagoya Institute of Technology).  The base algorithm and the name "`Julius`"
was given by Tatsuya Kawahara (Kyoto University).

Many researchers having been engaged in using and developing Julius by giving
great ideas, comments and suggestions.  Also thanks to all the contributors from
speech recognition society and many developers in GitHub!

## License

This tool is licensed under the same license with Julius - see the license term
of Julius for details.
