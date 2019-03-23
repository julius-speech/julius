# Audio Input

Julius can recognize audio data via file, live audio device and tcp-ip network.
A single source is chosen by option
[-input](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-input-micfilerawfilemfcfileoutprobadinnetvecnetstdinnetaudioalsaossesdpulseaudio).
Data should be in 16 bit (signed short), monaural (1 channel).

Note that **sampling rate of input should be set to the same as the training
data** of acoustic model.  If you give data with different sampling rate with
the acoustic model condition, it will not be recognized correctly.  Julius has
no down-sampling or up-sampling scheme in it.

Julius assumes the default sampling rate to 16 kHz, so when using acoustic model
trained with other sampling rate, the rate should be given explicitly by option,
either
[-smpPediod](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-smpperiod-period)
or
[-smpFreq](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-smpfreq-hz),

## Processing mode

Julius can be used on both off-line recognition of audio files and on-line
recognition of live audio input.  It has two processing modes suitable for each,
"**buffered processing**" and "**stream processing**".

In the following section, "*buffered processing*" denotes buffered decoding:
first the input audio will be stored to memory until the end of a segment comes,
and then feature extraction and decoding will be processed in turn.  Also
"*stream processing*" denotes on-the-fly low-latency decoding, where the input
audio will be processed per short chunk and recognition process will be run
concurrently in parallel with the incoming input.  They can be switched by
[-realtime and
-norealtime](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-realtime--norealtime)
options.

## File Input

File input is chosen when specifying "`-input file`".
Supported file types are:

- WAV format (.wav), Linear PCM
- RAW format (no header), signed short (16bit), Big Endian

Other formats such as .au, .nist and more can be used by using `libsndfile`.  To
use, install `libsndfile` headers and libraries before build.

Notes on RAW format:

- Samples should be in **Big Endian** byte order.
- Sampling rate check will not work because no header information is available in RAW file.

Buffered processing is the default processing mode for file input. Voice
activity detection is disabled by default. Each file is assumed as a single
utterance speech. VAD can be enabled by option
[-cutsilence](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-cutsilence--nocutsilence)
to perform voice part detection, just as the same as live audio capture.

When a list of input files by option
[-filelist](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-filelist-filename),
all the listed files will be recognized by turn.

## Capture Live Audio

Julius can read live audio input from audio device and perform on-the-fly
recognition with low latency.  Stream processing is the default mode for this
kind of input source.

### On Linux

Available audio APIs are:

- `alsa` - ALSA (Advanced Linux Sound Architecture)
- `oss` - OSS (Open Sound System)
- `pulseaudio` - PulseAudio
- `esd` - ESD (Enlightened Sound Daemon)

ALSA, PulseAudio and ESD requires corresponding library to be incorporated into
Julius.  See the Installation instruction how to enable them.

When `-input mic` is specified, live audio capturing is chosen.  All the enabled
APIs are searched by the order of the list above, and the first one found will
be used.  You can also specify which audio API to use by
"[`-input`](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-input-micfilerawfilemfcfileoutprobadinnetvecnetstdinnetaudioalsaossesdpulseaudio)
`apiname`".  For example, `-input pulseaudio` will choose PulseAudio.

Choosing audio devices:

- On ALSA, set device name to environment variable "`ALSADEV`". See [ALSA document](https://www.alsa-project.org/main/index.php/DeviceNames#Capture_device_names) about naming rules.
- On OSS, set device path to environment variable "`AUDIODEV`".  Default is "`/dev/dsp`".

### On Windows

At run time, Julius try to check for supported audio interface in the following
order consulting [portaudio library](http://www.portaudio.com/).

- WASAPI
- ASIO
- DirectSound
- MME

The first found one will be chosen.  DirectSound will be chosen in most PCs.

The default device will be opened by default.  To open other device, set the
device's name to env "`PORTAUDIO_DEV` or index number to env
"`PORTAUDIO_DEV_NUM`".  The list of available devices, their index numbers and
names, are outputted at startup process of Julius in the following format:

```text
id [desc1: desc2]
```

You can choose the device by either setting its "`id`" number by
`PORTAUDIO_DEV_NUM`, or setting the string "`desc1: desc2`" to `PORTAUDIO_DEV`.
If the same name is found, the first one will be chosen.

### On Other OS

Default audio interface, default audio device will be used.

## Network

"`-input adinnet`" enables network streamed audio input.  Julius will wait for
tcp-ip connection from client, and then start receiving audio streams from the
client. The tool
[adintool](https://github.com/julius-speech/julius/tree/master/adintool) can be
run as a sample streaming client.

```shell
% julius ... -input adinnet
% adintool -in mic -out adinnet -server localhost
```

No checks for sampling frequency, the client should sent the audio data whose
sampling rate matches the conditions.

## Checking Audio Input

It is strongly recommended to test your audio setting separately with Julius
setup. Use
[adinrec](https://github.com/julius-speech/julius/tree/master/adinrec) or
[adintool](https://github.com/julius-speech/julius/tree/master/adintool) to
check for audio recording and receiving before Julius. They are simple, and uses
the same audio module as Julius, thus "what they record is what Julius listens".

You can also snoop what Julius listens by logging the audio inputs to files.
Option
[-record](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-record-dir)
records all segmented input into files to a specified directory.
