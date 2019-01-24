<!-- markdownlint-disable MD041 -->

[English / [Japanese](README.ja.md)]

# adinrec

Record one utterance from audio device and save to a file.

## Synopsis

```shell
% adinrec [options...] file.wav
```

## Description

`adinrec` detects an utterance input and store it to a file.

This tool uses Julius's internal VAD module for speech detection. The detection
algorithm and parameters are the same as Julius.

The audio format is 16 bit, 1 channel in Microsoft WAV format. If the given
filename already exists, it will be overridden.  When filename is "-" , the
captured data will be streamed into standard out with no header (raw) format.

### Prerequisites

You need one audio capture device on your machine.  If several devices are
available, the default one will be used.

### Installing

This tool will be installed together with Julius.

## Usage

Record one utterance with 16kHz, 16bit mono format:

```shell
% adinrec test.wav
```

Record one utterance with 48kHz, 16bit mono format:

```shell
% adinrec -freq 48000 test.wav
```

Receive audio stream from adinnet audio client, detect speech, and save the
first speech segment into a file.  Also activates libfvad-based VAD module.

```shell
% adinrec -input adinnet -fvad 3 test.wav
```

## Options

### -freq

sampling rate in Hz. (Default: 16000)

### -raw

output in raw (no header) format.  (Default: save in .wav format)

### Other options (-input, -lv, ...)

Julius's audio options are fully applicable to this tool.  You can choose input
device, set level threshold, change head/tail silence margin, load Julius's
jconf file and so on.  For the available options, see the options in Julius.

## Environment Variables

### ALSADEV

device name string for ALSA (default: "default")

### AUDIODEV

device name string for OSS (default: "/dev/dsp")

### PORTAUDIO_DEV

With PortAudio, index of capture device to use. The available devices will be
listed at startup.

### LATENCY_MSEC

Input latency of microphone input in milliseconds. Smaller value will shorten
latency but sometimes make process unstable. Default value will depend on the
running OS.

## Related tools

- "[adintool](https://github.com/julius-speech/julius/tree/master/adintool)" is
  another recording software with rich functions.
- "[julius](https://github.com/julius-speech/julius/)" uses the same audio
  detection algorithm.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
