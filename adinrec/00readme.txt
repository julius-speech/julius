    adinrec

ADINREC(1)                                                          ADINREC(1)



NAME
           adinrec
          - record audio device and save one utterance to a file

SYNOPSIS
       adinrec [options...] {filename}

DESCRIPTION
       adinrec opens an audio stream, detects an utterance input and store it
       to a specified file. The utterance detection is done by level and
       zero-cross thresholds. Default input device is microphone, but other
       audio input source, including Julius A/D-in plugin, can be used by
       using "-input" option.

       The audio format is 16 bit, 1 channel, in Microsoft WAV format. If the
       given filename already exists, it will be overridden.

       If filename is "-" , the captured data will be streamed into standard
       out, with no header (raw format).

OPTIONS
       adinrec uses JuliusLib and adopts Julius options. Below is a list of
       valid options.

   adinrec specific options
        -freq  Hz
           Set sampling rate in Hz. (default: 16,000)

        -raw
           Output in raw file format.

   JuliusLib options
        -input  {mic|rawfile|adinnet|stdin|netaudio|esd|alsa|oss}
           Choose speech input source. Specify 'file' or 'rawfile' for
           waveform file. On file input, users will be prompted to enter the
           file name from stdin.

           'mic' is to get audio input from a default live microphone device,
           and 'adinnet' means receiving waveform data via tcpip network from
           an adinnet client. 'netaudio' is from DatLink/NetAudio input, and
           'stdin' means data input from standard input.

           At Linux, you can choose API at run time by specifying alsa, oss
           and esd.

        -lv  thres
           Level threshold for speech input detection. Values should be in
           range from 0 to 32767. (default: 2000)

        -zc  thres
           Zero crossing threshold per second. Only input that goes over the
           level threshold (-lv) will be counted. (default: 60)

        -headmargin  msec
           Silence margin at the start of speech segment in milliseconds.
           (default: 300)

        -tailmargin  msec
           Silence margin at the end of speech segment in milliseconds.
           (default: 400)

        -zmean
           This option enables DC offset removal.

        -smpFreq  Hz
           Set sampling rate in Hz. (default: 16,000)

        -48
           Record input with 48kHz sampling, and down-sample it to 16kHz
           on-the-fly. This option is valid for 16kHz model only. The
           down-sampling routine was ported from sptk. (Rev. 4.0)

        -NA  devicename
           Host name for DatLink server input (-input netaudio).

        -adport  port_number
           With -input adinnet, specify adinnet port number to listen.
           (default: 5530)

        -nostrip
           Julius by default removes successive zero samples in input speech
           data. This option stop it.

        -C  jconffile
           Load a jconf file at here. The content of the jconffile will be
           expanded at this point.

        -plugindir  dirlist
           Specify which directories to load plugin. If several direcotries
           exist, specify them by colon-separated list.

ENVIRONMENT VARIABLES
        ALSADEV
           Device name string for ALSA. (default: "default")

        AUDIODEV
           Device name string for OSS. (default: "/dev/dsp")

        PORTAUDIO_DEV
           (portaudio V19) specify the name of capture device to use. See the
           instruction output of log at start up how to specify it.

        LATENCY_MSEC
           Input latency of microphone input in milliseconds. Smaller value
           will shorten latency but sometimes make process unstable. Default
           value will depend on the running OS.

SEE ALSO
        julius ( 1 ) ,
        adintool ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                        ADINREC(1)
