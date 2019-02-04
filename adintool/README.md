<!-- markdownlint-disable MD041 -->

English / [Japanese](README.ja.md)

# adintool / adintool-gui

Multi-input / Multi-output audio tool to detect / record / split / send /
receive audio streams.

## Synopsis

```shell
% adintool -in InputDevice -out OutputDevice [options...]
```

GUI version:

```shell
% adintool-gui [options...]
```

## Description

`adintool` analyzes speech input, detects speech segments skipping silence, and
records the detected segments in various ways.

Input waveform:

- microphone
- a speech file
- tty stdin
- network socket (adinnet)

Speech processing:

- speech detection
- speech segmentation
- feature vector extraction
- one-shot / successive

Output waveform / feature vector:

- waveform file (.wav)
- network socket (adinnet) audio to Julius
- network socket (vecnet) feature vector to Julius
- tty stdout
- none

This tool uses Julius's internal VAD module for speech detection. The detection
algorithm and parameters are the same as Julius.

The default audio format is 16 bit, 1 channel in Microsoft WAV format.

`adintool-gui` is a GUI version of adintool.  All the functions are as same as
`adintool`, except that server connection will be established manually with
pressing `c` key.  When executed with no argument, `adintool-gui` assumes `-in
mic -out none`.

### Prerequisites

If you are capturing from microphone device, you need one audio capture device
on your machine.  If several devices are available, the default one will be
used.

### Installing

This tools will be installed together with Julius.  SDL v2 library is required
to build `adintool-gui`, so it should be installed before build phase. On
Ubuntu, do this before installation:

```shell
% sudo apt-get install libsdl2-dev
```

## Usage

Record utterances one by one, into file "test0001.wav", "test0002.wav", ...

```shell
% adintool -in mic -out file -filename test
```

Record only one utterance into "test.wav"

```shell
% adintool -in mic -out file -oneshot -filename test.wav
```

Split audio file "speech.wav" into segments and send to Julius spawned with
`-input adinnet` at localhost

```shell
% echo speech.wav | adintool -in file -out adinnet -server localhost
```

Receive wave data segments from adinnet and save to files "wave_xxxx.wav"

```shell
% adintool -in adinnet -out file -nosegment -filename wave_
```

`adintool-gui` can be manipulated by key:

- Press `Up/Down` to move trigger threshold
- Press `c` to connect/disconnect with server (manual connect)
- Press `Enter` to force speech down-trigger at that point
- Press `m` to mute/un-mute

## Options: audio property

### -freq

sampling rate in Hz. (Default: 16000)

### -raw

output in raw (no header) format.  (Default: save in .wav format)

## Options: speech detection / segmentation

### -nosegment

Do not perform speech detection. Treat the whole input as a single valid segment.

### -rewind msec

With `-in mic` and `-out adinnet` or `-out vecnet`, recording back to the
specified milliseconds at each adintool resume.  May be valid when the beginning
of segment is missing at resume.

### -oneshot

One-shot recording: will exit after the end of first speech segment was
detected.  If not specified, `adintool` will perform successive detection.

## Options: feature vector extraction

### -paramtype parameter_type

With `-out vecnet`, specify parameter type in HTK parameter description format
like "MFCC_E_D_N_Z".

### -veclen vector_length

With `-out vecnet`, specify vector length in # dimensions.

## Options: I/O

### -in InputDevice

(REQUIRED) Select audio input

- `mic`: capture via microphone input
- `file`: audio file input (will be prompted for file name)
- `stdin`: standard input (assume raw format)
- `adinnet`: become adinnet server, receive from adinnet client

### -out OutputDevice

(REQUIRED) Select output

- `file`: save in .wav file (require `-filename`)
- `stdout`: standard output (raw format)
- `adinnet`: become adinnet client, send to adinnet server (require `-server`)
- `vecnet`: become vecnet client, send features to vecnet server (require `-server`)
- `none`: no output

### -filename

With `-out file`, specify output file name base.  When "foobar" is specified,
the successive outputs will be saved to "foobar.0001.wav", "foobar.0002.wav" and
so on.  The default initial number is 0001, but can be changed by `-startid`.
When `-oneshot` is specified together, the output will be saved to "file", as
is.

### -startid number

With `-out file` and `-filename`, specify the initial number (default is 0)

### -server host[,host,...]

With `-out adinnet`, specify hostname(s) to send the audio data.

### -port num

With `-out adinnet`, specify port number to connect to.  (default:5530)

### -inport num

With `-in adinnet`, specify port number to listen.  (default:5530)

## Options: adinnet synchronization

### -autopause

With `-out adinnet`, specify this option to tell `adintool` automatically enter
pause state after every speech segment detection.  It will blocks until a resume
signal from adinnet server.

### -loosesync

With `-out adinnet` and specify multiple servers in `-server`, this option
specifies how to wait for resume from multiple servers.  By default, when
`adintool` enters pause state, it resumes only after receiving resume commands
from all servers. When this option is specified, `adintool` resumes immediately
after one of the servers have emitted resume commands, not waiting for all
servers.

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

With portaudio, index of capture device to use. The available devices will be
listed at startup.

### LATENCY_MSEC

Input latency of microphone input in milliseconds. Smaller value will shorten
latency but sometimes make process unstable. Default value will depend on the
running OS.

## Related tools

- "[adinrec](https://github.com/julius-speech/julius/tree/master/adinrec)" is a
  simplified version recording only the first audio segment.
- "[julius](https://github.com/julius-speech/julius/)" uses the same audio
  detection algorithm, and can receive data from `adintool`.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
