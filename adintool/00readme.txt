    adintool

ADINTOOL(1)                                                        ADINTOOL(1)



NAME
           adintool
          - a tool to record / split / send / receive audio streams

SYNOPSIS
       adintool {-in inputdev} {-out outputdev} [options...]

DESCRIPTION
       adintool analyzes speech input, finds speech segments skipping silence,
       and records the detected segments in various ways. It performs speech
       detection based on zerocross number and power (level), and records the
       detected parts to files or other output devices sucessively.

       adintool is a upper version of adinrec with various functions.
       Supported input device are: microphone input, a speech file, standard
       tty input, and network socket (called adin-net server mode). Julius
       plugin can be also used. Detected speech segments will be saved to
       output devices: speech files, standard tty output, and network socket
       (called adin-net client mode). For example, you can split the incoming
       speech to segments and send them to Julius to be recognized.

       Output format is WAV, 16bit (signed short), monoral. If the file
       already exist, it will be overridden.

OPTIONS
       All Julius options can be set. Only audio input related options are
       treated and others are silently skipped. Below is a list of options.

   adintool specific options
        -freq  Hz
           Set sampling rate in Hz. (default: 16,000)

        -in  inputdev
           Audio input device. "mic" to capture via microphone input, "file"
           for audio file input, and "stdin" to read raw data from
           standard-input. For file input, file name prompt will appear after
           startup. Use "adinnet" to make adintool as "adinnet server",
           receiving data from client via network socket. Default port number
           is 5530, which can be altered by option "-inport".

           Alternatively, input device can be set by "-input" option, in which
           case you can use plugin input.

        -out  outputdev
           Audio output device store the data. Specify "file" to save to file,
           in which the output filename should be given by "-filename". Use
           "stdout" to standard out. "adinnet" will make adintool to be an
           adinnet client, sending speech data to a server via tcp/ip socket.
           "vecnet" will make adintool to be a vecnet client, sending feature
           vectors extracted from input to a server via tcp/ip socket. When
           using "adinnet" and "vecnet" output, the server name to send data
           should be specified by "-server". The default port number is 5530,
           which can be changed by "-port" option.

        -inport  num
           When adintool becomes adinnet server to receive data (-in adinnet),
           set the port number to listen. (default: 5530)

        -server  [host] [,host...]
           When output to adinnet server (-out adinnet), set the hostname. You
           can send to multiple hosts by specifying their hostnames as
           comma-delimited list like "host1,host2,host3".

        -port  [num] [,num...]
           When adintool send a data to adinnet server (-out adinnet), set the
           port number to connect. (default: 5530) For multiple servers,
           specify port numbers for all servers like "5530,5530,5531".

        -filename  file
           When output to file (-out file), set the output filename. The
           actual file name will be as "file.0000.wav" , "file.0001.wav" and
           so on, where the four digit number increases as speech segment
           detected. The initial number will be set to 0 by default, which can
           be changed by "-startid" option. When using "-oneshot" option to
           save only the first segment, the input will be saved as "file".

        -startid  number
           At file output, set the initial file number. (default: 0)

        -oneshot
           Exit after the end of first speech segment.

        -nosegment
           Do not perform speech detection for input, just treat all the input
           as a single valid segment.

        -raw
           Output as RAW file (no header).

        -autopause
           When output to adinnet server, adintool enter pause state at every
           end of speech segment. It will restart when the destination adinnet
           server sends it a resume signal.

        -loosesync
           When output to multiple adinnet server, not to do strict
           synchronization for restart. By default, when adintool has entered
           pause state, it will not restart until resume commands are received
           from all servers. This option will allow restart at least one
           restart command has arrived.

        -rewind  msec
           When input is a live microphone device, and there has been some
           continuing input at the moment adintool resumes, it start recording
           backtracking by the specified milliseconds.

        -paramtype  parameter_type
           When output is a vecnet (-out vecnet), specify parameter type in
           HTK format like "MFCC_E_D_N_Z".

        -veclen  vector_length
           When output is a vecnet (-out vecnet), specify vector length
           (dim.).

   Concerning Julius options
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
           (using mic input with alsa device) specify a capture device name.
           If not specified, "default" will be used.

        AUDIODEV
           (using mic input with oss device) specify a capture device path. If
           not specified, "/dev/dsp" will be used.

        PORTAUDIO_DEV
           (portaudio V19) specify the name of capture device to use. See the
           instruction output of log at start up how to specify it.

        LATENCY_MSEC
           Try to set input latency of microphone input in milliseconds.
           Smaller value will shorten latency but sometimes make process
           unstable. Default value will depend on the running OS.

EXAMPLES
       Record microphone input to files: "data.0000.wav", "data.0001.wav" and
       so on:
       Split a long speech file "foobar.raw" into "foobar.1500.wav",
       "foobar.1501.wav" ...:
       Copy an entire audio file via network socket.
       Detect speech segment, send to Julius via network and recognize it:

SEE ALSO
        julius ( 1 ) ,
        adinrec ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                       ADINTOOL(1)
