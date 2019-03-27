# Julius Options

## Before you start

### Basic rules

- IF options are conflicting or appears several times, the last one will
  overrides former ones.
- The option "-C `jconffile`" will expand the content of the jconf file at the
  position.
- In jconf file, relative path arguments are treated as relative to the *jconf
  file itself*, not the current working directory.

### Option groups

Options are categorized into one of the following groups:

- `APP`
- `LM`
- `AM`
- `SR`
- `GLOBAL`

`APP` group is application-side option group.  The options in the `APP` group can be specified for the command-line application `"julius"`, but not available at your application integrating `libjulius` and `libsent`.  They can be specified at any point of the option sequence.

`LM`, `AM` and `SR` are option groups for language model, acoustic model and search algorithm, respectively.

`GLOBAL` group contains misc. options that are independent from models and algorithms. They contains options concerning audio front-end and utility back-end.

You do not need to care about the groups when you just use Julius with single recognition instance (1 AM, 1 LM).  If you are going to set up Julius to perform multi-instance decoding with multiple models, you should care about the groups and section declaration as described in the following section.

## Instance declaration options for multi decoding

Julius supports multi-instance decoding with multiple models.  The multi-instance decoding runs each models simultaneously for an audio input within a single thread and output multiple results at once.

To perform multi-model decoding, you should define several model sets, and also define search instances using the options. To specify multiple models sets and instances, a special option `-AM`, `-LM` and `-SR` can be used to switch the mode.  Below is an example of jconf file for multi-model decoding:

```text
# Example of two ASR module, each has its original AM/LM

(GLOBAL options)
...
-AM Mname1
(AM options for Mname1)

-AM Mname2
(AM options for Mname2)
...

-LM Lname1
(LM options for Lname1)
...

-LM Lname2
(LM options for Lname2)
...

-SR module1 Mname1 Lname1
(SR options for module1)
...

-SR module2 Mname2 Lname2
(SR options for module1)
...

-GLOBAL
(GLOBAL options)
```

```text
# Example of two ASR modules with different LMs sharing single AM

(GLOBAL options)
...
-AM name
(AM options for name)

-LM Lname1
(LM options for Lname1)
...

-LM Lname2
(LM options for Lname2)
...

-SR module1 name Lname1
(SR options for module1)
...

-SR module2 name Lname2
(SR options for module1)
...

-GLOBAL
(GLOBAL options)
```

When using the switches, the `GLOBAL` options should be placed either at the top (before any instance declaration), or after `-GLOBAL` option.

### -AM name

Create a new AM configuration set, and switch current to the new
one. You should give a unique name. (Rev.4.0)

### -LM name

Create a new LM configuration set, and switch current to the new
one. You should give a unique name. (Rev.4.0)

### -SR name am_name lm_name

Create a new search configuration set, and switch current to the
new one. The specified AM and LM will be assigned to it. The
`am_name` and `lm_name` can be either name or ID number. You should
give a unique name. (Rev.4.0)

### -AM_GMM

When using GMM for front-end processing, you can specify
GMM-specific acoustic parameters after this option. If you does not
specify -AM_GMM with GMM, the GMM will share the same parameter
vector as the last AM. The current AM will be switched to the GMM
one, so be careful not to confuse with normal AM configurations.
(Rev.4.0)

### -GLOBAL

Start a global section. The global options should be placed before
any instance declaration, or after this option on multiple model
recognition. This can be used multiple times. (Rev.4.1)

### -nosectioncheck, -sectioncheck

Disable / enable option location check in multi-model decoding.
When enabled, the options between instance declaration is treated
as "sections" and only the belonging option types can be written.
For example, when an option -AM is specified, only the AM related
option can be placed after the option until other declaration is
found. Also, global options should be placed at top, before any
instance declaration. This is enabled by default. (Rev.4.1)

## Application options (category `APP`)

### -outfile

On file input, save the recognition result of each
file to a separate file. The output file of an input file will be
the same name but the suffix will be changed to ".out". (rev.4.0)

### -separatescore

Output the language and acoustic scores separately.

### -callbackdebug

Print the callback names at each call for debug. (rev.4.0)

### -charconv from to

Print with character set conversion.  `from` is the source character
set used in the language model, and `to` is the target character set
you want to get.

On Linux, the arguments should be a code name. You can obtain the
list of available code names by invoking the command "iconv
--list". On Windows, the arguments should be a code name or
code page number. Code name should be one of "ansi", "mac", "oem",
"utf-7", "utf-8", "sjis", "euc". Or you can specify any code page
number supported at your environment.

### -nocharconv

Disable character conversion.

### -module [port]

Run Julius on "Server Module Mode". After startup, Julius waits for
tcp/ip connection from client. Once connection is established,
Julius start communication with the client to process incoming
commands from the client, or to output recognition results, input
trigger information and other system status to the client. The
default port number is 10500.

### -outcode arg

For module mode, choose which information to be output to module client. `arg` is a sequence of characters to be output:

- `W` : word
- `L` : LM entry
- `P` : phone sequence
- `S` : score
- `C` : confidence score
- `w` : 1st pass word
- `l` : 1st pass LM entry
- `p` : 1st pass phone sequence
- `s` : 1st pass score

Default is "`WLPS`", which means to output word, LM entry, phone sequence and score of the final result.

### -noxmlescape

For module mode, disable XML special character escaping (Rev.4.5).

### -record dir

Auto-save all input speech data into the specified directory. Each
segmented inputs are recorded each by one. The file name of the
recorded data is generated from system time when the input ends, in
a style of YYYY.MMDD.HHMMSS.wav. File format is 16bit monaural WAV.
Invalid for mfcc file input.

With input rejection by `-rejectshort`, the rejected input will also
be recorded even if they are rejected.

### -logfile file

Save all log output to a file instead of standard output. (Rev.4.0)

### -nolog

Disable all log output. (Rev.4.0)

### -help

Output help message and exit.

## Audio input options (category `GLOBAL`)

### -input {mic|file|rawfile|mfcfile|outprob|adinnet|vecnet|stdin|netaudio|alsa|oss|esd|pulseaudio|...}

Choose speech input source. Specify '`file`' or '`rawfile`' for
waveform file, '`htkparam`' or '`mfcfile`' for HTK parameter file,
'`outprob`' for outprob vectors from HTK parameter file. On file
input, users will be prompted to enter the file name from
stdin, or you can use `-filelist` option to specify list of files
to process.

'`mic`' is to get audio input from a default live microphone
device, and '`adinnet`' means receiving waveform data via tcp-ip
network from an adinnet client. '`netaudio`' is from
DatLink/NetAudio input, and 'stdin' means data input from
standard input. 'vecnet' means receiving feature / outprob
vectors via tcp-ip network from vecnet client.

For waveform file input, only WAV (no compression) and RAW
(no header, 16bit, big endian) are supported by default. Other
format can be read when compiled with libsnd library. To see
what format is actually supported, see the help message using
option -help. For stdin input, only WAV and RAW is supported.
(default: `mfcfile`)

On some OS, instead of `mic`, you can explicitly specify available audio API (alsa/oss/esd/...).

### -filelist filename

(With -input rawfile|mfcfile|outprob) perform recognition on
all files listed in the file. The file should contain input
file per line. Engine will end when all of the files are
processed.  See also `-outfile` for per-input result output.

### -48

Record input with 48kHz sampling, and down-sample it to 16kHz
on-the-fly. This option is valid for 16kHz model only. The
down-sampling routine was ported from sptk. (Rev. 4.0)

### -NA devicename

Host name for DatLink server input (-input netaudio).

### -adport port_number

With `-input adinnet`, specify adinnet port number to listen.
(default: 5530)

### -nostrip

Julius by default removes successive zero samples in input
speech data. This option disables the zero sample removal.

### -zmean, -nozmean

This option enables/disables DC offset removal of input
waveform. Offset will be estimated from the whole input. For
microphone / network input, zero mean of the first 48000
samples (3 seconds in 16kHz sampling) will be used for the
estimation. (default: disabled)

This option uses static offset for the channel. See also
`-zmeansource` for frame-wise offset removal.

### -lvscale factor

Posterior scaling of magnitude of audio input.

## Speech detection options (category `GLOBAL`)

### -cutsilence, -nocutsilence

Turn on / off the speech detection by level and zero-cross.
Default is on for mic / adinnet input, and off for files.

### -lv threshold

Level threshold for speech input detection. Values should be in
range from 0 to 32767. (default: 2000)

### -zc thresold

Zero crossing threshold per second. Only input that goes over
the level threshold (`-lv`) will be counted. (default: 60)

### -headmargin msec

Silence margin at the start of speech segment in milliseconds.
(default: 300)

### -tailmargin msec

Silence margin at the end of speech segment in milliseconds.
(default: 400)

### -chunk_size size

Buffer length of the audio input can be set with number of
samples (default number is 1000). If you set small number, you
can reduce the delay. However, it becomes unstable when too
small.

### -fvad mode

Set libfvad-based VAD mode. `mode` is an integer value from -1 to 3, specify -1 to disable, 0 for moderate detection, 3 for most aggressive detection (more likely to drop speech-like noises).  Default value is -1 (disabled)

### -fvad_param nFrame threshold

Set libfvad detailed parameter.  `nFrame` is the number of smoothing frame. `threshold` is the threshold to detect speech trigger [0.0-1.0]. Default values are 5 and 0.5 respectively.

## Input rejection options (category `GLOBAL`)

Two simple front-end input rejection methods are implemented, based
on input length and average power of detected segment. The
rejection by average power is experimental, and can be enabled by
`--enable-power-reject` on compilation. Valid for MFCC feature with
power coefficient and real-time input only.

For GMM-based input rejection see the GMM section below.

### -rejectshort msec

Reject input shorter than specified milliseconds. Search will
be terminated and no result will be output.

### -rejectlong msec

Reject input longer than specified milliseconds. Search will be
terminated and no result will be output.

### -powerthres threshold

Reject the inputted segment by its average energy. If the
average energy of the last recognized input is below the
threshold, Julius will reject the input. (Rev.4.0)

This option is valid when `--enable-power-reject` is specified at
compilation time.

## GMM rejection / VAD options (category `GLOBAL`)

GMM will be used for input rejection by accumulated score, or for
front-end GMM-based VAD when `--enable-gmm-vad` is specified.

NOTE: You should also set the proper MFCC parameters required for
the GMM, specifying the acoustic parameters described in AM section
`-AM_GMM`.

When GMM-based VAD is enabled, the voice activity score will be
calculated at each frame as front-end processing. The value will be
computed as
$$ \max_{m \in M_v} p(x|m) - \max_{m \in M_n} p(x|m) $$
where $M_v$ is a set of voice GMM, and $M_n$ is a set of noise GMM
whose names should be specified by `-gmmreject`. The activity score
will be then averaged for the last N frames, where N is specified
by `-gmmmargin`. Julius updates the averaged activity score at each
frame, and detect speech up-trigger when the value gets higher than
a value specified by `-gmmup`, and detect down-trigger when it gets
lower than a value of `-gmmdown`.

### -gmm hmmdefs_file

GMM definition file in HTK format. If specified, GMM-based
input verification will be performed concurrently with the 1st
pass, and you can reject the input according to the result as
specified by -gmmreject. The GMM should be defined as one-state
HMMs.

### -gmmnum number

Number of Gaussian components to be computed per frame on GMM
calculation. Only the N-best Gaussians will be computed for
rapid calculation. The default is 10 and specifying smaller
value will speed up GMM calculation, but too small value may cause degradation of identification performance.

### -gmmreject string

Comma-separated list of GMM names to be rejected as invalid
input. When recognition, the log likelihoods of GMMs
accumulated for the entire input will be computed concurrently
with the 1st pass. If the GMM name of the maximum score is
within this string, the 2nd pass will not be executed and the
input will be rejected.

### -gmmmargin frames

(GMM_VAD) Head margin in frames. When a speech trigger detected
by GMM, recognition will start from current frame minus this
value. (Rev.4.0)

This option will be valid only if compiled with
`--enable-gmm-vad`.

### -gmmup value

(GMM_VAD) Up trigger threshold of voice activity score.
(Rev.4.1)

This option will be valid only if compiled with
`--enable-gmm-vad`.

### -gmmdown value

(GMM_VAD) Down trigger threshold of voice activity score.
(Rev.4.1)

This option will be valid only if compiled with
`--enable-gmm-vad`.

## Misc. options (category `GLOBAL`)

### -realtime, -norealtime

Explicitly switch between "stream processing" and "buffered processing".  "-realtime" sets mode to stream processing, and "-norealtime" sets mode to buffered processing.

Default is buffer processing for files, and stream processing for microphone and network input.  Setting "-realtime" to a file input can simulate the recognition process as if it were input from microphone.

### -C jconffile

Load a jconf file at here. The content of the jconffile will be
expanded at this point.

### -version

Print version information to standard error, and exit.

### -setting

Print engine setting information to standard error, and exit.

### -quiet

Output less log. For result, only the best word sequence will
be printed.

### -debug

(For debug) output enormous internal message and debug
information to log.

### -check {wchmm|trellis|triphone}

For debug, enter interactive check mode.

### -plugindir dirlist

Specify directory to load plugin. If several directories exist,
specify them by colon-separated list.

### -outprobout file

Save computed outprob vectors to HTK file (for debug).

## N-gram options (category `LM`)

### -d bingram_file

Use binary format N-gram. An ARPA N-gram file can be converted
to Julius binary format by mkbingram.

### -nlr arpa_ngram_file

A forward, left-to-right N-gram language model in standard ARPA
format. When both a forward N-gram and backward N-gram are
specified, Julius uses this forward 2-gram for the 1st pass,
and the backward N-gram for the 2nd pass.

Since ARPA file often gets huge and requires a lot of time to
load, it may be better to convert the ARPA file to Julius
binary format by mkbingram. Note that if both forward and
backward N-gram is used for recognition, they together will be
converted to a single binary.

When only a forward N-gram is specified by this option and no
backward N-gram specified by `-nrl`, Julius performs recognition
with only the forward N-gram. The 1st pass will use the 2-gram
entry in the given N-gram, and The 2nd pass will use the given
N-gram, with converting forward probabilities to backward
probabilities by Bayes rule. (Rev.4.0)

### -nrl arpa_ngram_file

A backward, right-to-left N-gram language model in standard
ARPA format. When both a forward N-gram and backward N-gram are
specified, Julius uses the forward 2-gram for the 1st pass, and
this backward N-gram for the 2nd pass.

Since ARPA file often gets huge and requires a lot of time to
load, it may be better to convert the ARPA file to Julius
binary format by mkbingram. Note that if both forward and
backward N-gram is used for recognition, they together will be
converted to a single binary.

When only a backward N-gram is specified by this option and no
forward N-gram specified by `-nlr`, Julius performs recognition
with only the backward N-gram. The 1st pass will use the
forward 2-gram probability computed from the backward 2-gram
using Bayes rule. The 2nd pass fully use the given backward
N-gram. (Rev.4.0)

## Grammar options (category `LM`)

Multiple grammars can be specified by repeating `-gram` and
`-gramlist`. Note that this is unusual behavior from other options
(in normal Julius option, last one will override previous ones).
You can also use `-nogram` to reset the grammars already specified before
the point.

### -gram gramprefix1[,gramprefix2[,gramprefix3,...]]

Comma-separated list of grammars to be used. the argument
should be a prefix of a grammar, i.e. if you have foo.dfa and
foo.dict, you should specify them with a single argument foo.
Multiple grammars can be specified at a time as a
comma-separated list.

### -gramlist list_file

Specify a grammar list file that contains list of grammars to
be used. The list file should contain the prefixes of grammars,
each per line. A relative path in the list file will be treated
as relative to the file, not the current path or configuration
file.

### -dfa dfa_file -v dict_file

An old way of specifying grammar files separately. This is
bogus, and should not be used any more.

### -nogram

Remove the current list of grammars already specified by `-gram`,
`-gramlist`, `-dfa` and `-v`.

## Word Dictionary options for N-gram and DFA (category `LM`)

### -v dict_file

Word dictionary file.

### -silhead word_string -siltail word_string

Silence word defined in the dictionary, for silences at the
beginning of sentence and end of sentence. (default: "{`<s>`",
"`</s`>")

### -mapunk word_string

Specify unknown word. Default is "`<unk>`" and "`<UNK>`". This will
be used to assign word probability on unknown words, i.e. words
in dictionary that are not in N-gram vocabulary.

### -forcedict

Skip error words in dictionary and force running.

### -iwspword

Add a word entry to the dictionary that should correspond to
inter-word pauses. This may improve recognition accuracy in
some language model that has no explicit inter-word pause
modeling. The word entry to be added can be changed by
`-iwspentry`.

### -iwspentry word_entry_string

Specify the word entry that will be added by `-iwspword`.
(default: "`<UNK> [sp] sp sp`")

### -sepnum number

Number of high frequency words to be isolated from the lexicon
tree, to ease approximation error that may be caused by the
one-best approximation on 1st pass. (default: 150)

### -adddict dicfile

Load grammars in additional on startup.

### -addentry entry_string

Load entry of words in additional on startup.

## Isolated word recognition options (category `LM`)

Dictionary can be specified by using `-w` and `-wlist`. When you
specify multiple times, all of them will be read at startup. You
can use `-nogram` to reset the already specified dictionaries at that
point.

### -w dict_file

Word dictionary for isolated word recognition. File format is
the same as other LM. (Rev.4.0)

### -wlist list_file

Specify a dictionary list file that contains list of
dictionaries to be used. The list file should contain the file
name of dictionaries, each per line. A relative path in the
list file will be treated as relative to the list file, not the
current path or configuration file. (Rev.4.0)

### -wsil head_sil_model_name tail_sil_model_name sil_context_name

On isolated word recognition, silence models will be appended
to the head and tail of each word at recognition. This option
specifies the silence models to be appended. sil_context_name
is the name of the head sil model and tail sil model as a
context of word head phone and tail phone. For example, if you
specify "`-wsil silB silE sp`", a word with phone sequence "`b eh t`"
will be translated as "`silB sp-b+eh b-eh+t eh-t+sp silE`".
(Rev.4.0)

## User-defined LM options (category `LM`)

### -userlm

Declare to use user LM functions in the program. This option
should be specified if you use user-defined LM functions.
(Rev.4.0)

## Acoustic HMM options (category `-AM` / `-AM_GMM`)

### -h hmmdef_file

Acoustic HMM definition file. It should be in HTK ascii format,
or Julius binary format. You can convert HTK ascii format to
Julius binary format using mkbinhmm.

### -hlist hmmlist_file

HMMList file for phone mapping. This file provides mapping
between logical triphone names generated in the dictionary and
the defined HMM names in hmmdefs. This option should be
specified for context-dependent model.

### -tmix number

Specify the number of top Gaussians to be calculated in a
mixture code book. Small number will speed up the acoustic
computation, but AM accuracy may get worse with too small
value. See also -gprune. (default: 2)

### -spmodel name

Specify HMM model name that corresponds to short-pause in an
utterance. The short-pause model name will be used in
recognition: short-pause skipping on grammar recognition,
word-end short-pause model insertion with `-iwsp` on N-gram, or
short-pause segmentation (`-spsegment`). (default: "sp")

### -multipath

Enable multi-path mode. To make decoding faster, Julius by
default impose a limit on HMM transitions that each model
should have only one transition from initial state and to end
state. On multi-path mode, Julius does extra handling on
inter-model transition to allows model-skipping transition and
multiple output/input transitions. Note that specifying this
option will make Julius a bit slower, and the larger beam width
may be required.

This function was a compilation-time option on Julius 3.x, and
now becomes a run-time option. By default (without this
option), Julius checks the transition type of specified HMMs,
and enable the multi-path mode if required. You can force
multi-path mode with this option. (rev.4.0)

### -gprune {safe|heuristic|beam|none|default}

Set Gaussian pruning algorithm to use. For tied-mixture model,
Julius performs Gaussian pruning to reduce acoustic
computation, by calculating only the top N Gaussians in each
code book at each frame. The default setting will be set
according to the model type and engine setting. `default` will
force accepting the default setting. Set this to `none` to
disable pruning and perform full computation. `safe` guarantees
the top N Gaussians to be computed.  `heuristic` and `beam` do more
aggressive computational cost reduction, but may result in
small loss of accuracy model (default: `safe` (standard), `beam`
(fast) for tied mixture model, `none` for non tied-mixture
model).

### -iwcd1 {max|avg|best number}

Select method to approximate inter-word triphone on the head
and tail of a word in the first pass.

`max` will apply the maximum likelihood of the same context
triphones. `avg` will apply the average likelihood of the same
context triphones. `best` number will apply the average of top
N-best likelihoods of the same context triphone.

Default is best 3 for use with N-gram, and avg for grammar and
word. When this AM is shared by LMs of both type, latter one
will be chosen.

### -iwsppenalty float

Insertion penalty for word-end short pauses appended by `-iwsp`.

### -gshmm hmmdef_file

If this option is specified, Julius performs Gaussian Mixture
Selection for efficient decoding. The hmmdefs should be a
monophone model generated from an ordinary monophone HMM model,
using mkgshmm.

### -gsnum number

On GMS, specify number of monophone states to compute
corresponding triphones in detail. (default: 24)

### -notypecheck

By default, Julius checks the input parameter type whether it
matches the AM or not. This option will disable the check and
force engine to use the input vector as is.

## Speech analysis options (category `-AM` / `-AM_GMM`)

Only MFCC feature extraction is supported in current Julius. Thus
when recognizing a waveform input from file or microphone, AM must
be trained by MFCC. The parameter condition should also be set as
exactly the same as the training condition by the options below.

When you give an input in HTK Parameter file, you can use any
parameter type for AM. In this case Julius does not care about the
type of input feature and AM, just read them as vector sequence and
match them to the given AM. Julius only checks whether the
parameter types are the same. If it does not work well, you can
disable this checking by `-notypecheck`.

In Julius, the parameter kind and qualifiers (as `TARGETKIND` in HTK)
and the number of cepstral parameters (`NUMCEPS`) will be set
automatically from the content of the AM header, so you need not
specify them by options.

Other parameters should be set exactly the same as training
condition. You can also give a HTK Config file which you used to
train AM to Julius by `-htkconf`. When this option is applied, Julius
will parse the Config file and set appropriate parameter.

You can further embed those analysis parameter settings to a binary
HMM file using mkbinhmm.

If options specified in several ways, they will be evaluated in the
order below. The AM embedded parameter will be loaded first if any.
Then, the HTK config file given by `-htkconf` will be parsed. If a
value already set by AM embedded value, HTK config will override
them. At last, the direct options will be loaded, which will
override settings loaded before. Note that, when the same options
are specified several times, later will override previous, except
that `-htkconf` will be evaluated first as described above.

### -smpPeriod period

Sampling period of input speech, in unit of 100 nanoseconds.
Sampling rate can also be specified by `-smpFreq`. Please note
that the input frequency should be set equal to the training
conditions of AM. (default: 625, corresponds to 16,000Hz)

This option corresponds to the HTK Option `SOURCERATE`. The same
value can be given to this option.

When using multiple AM, this value should be the same among all
AMs.

### -smpFreq Hz

Set sampling frequency of input speech in Hz. Sampling rate can
also be specified using `-smpPeriod`. Please note that this
frequency should be set equal to the training conditions of AM.
(default: 16,000)

When using multiple AM, this value should be the same among all
AMs.

### -fsize sample_num

Window size in number of samples. (default: 400)

This option corresponds to the HTK Option `WINDOWSIZE`, but value
should be in samples (HTK value / smpPeriod).

When using multiple AM, this value should be the same among all
AMs.

### -fshift sample_num

Frame shift in number of samples. (default: 160)

This option corresponds to the HTK Option `TARGETRATE`, but value
should be in samples (HTK value / smpPeriod).

When using multiple AM, this value should be the same among all
AMs.

### -preemph float

Pre-emphasis coefficient. (default: 0.97)

This option corresponds to the HTK Option `PREEMCOEF`. The same
value can be given to this option.

### -fbank num

Number of filterbank channels. (default: 24)

This option corresponds to the HTK Option `NUMCHANS`. The same
value can be given to this option. Be aware that the default
value not the same as in HTK default (22).

### -ceplif num

Cepstral liftering coefficient. (default: 22)

This option corresponds to the HTK Option `CEPLIFTER`. The same
value can be given to this option.

#### -rawe, -norawe

Enable/disable using raw energy before pre-emphasis (default:
disabled)

This option corresponds to the HTK Option `RAWENERGY`. Be aware
that the default value differs from HTK (enabled at HTK,
disabled at Julius).

### -enormal, -noenormal

Enable/disable normalizing log energy. On live input, this
normalization will be approximated from the average of last
input. (default: disabled)

This option corresponds to the HTK Option `ENORMALISE`. Be aware
that the default value differs from HTK (enabled at HTK,
disabled at Julius).

### -escale float_scale

Scaling factor of log energy when normalizing log energy.
(default: 1.0)

This option corresponds to the HTK Option `ESCALE`. Be aware that
the default value differs from HTK (0.1).

### -silfloor float

Energy silence floor in dB when normalizing log energy.
(default: 50.0)

This option corresponds to the HTK Option `SILFLOOR`.

#### -delwin frame

Delta window size in number of frames. (default: 2)

This option corresponds to the HTK Option `DELTAWINDOW`. The same
value can be given to this option.

### -accwin frame

Acceleration window size in number of frames. (default: 2)

This option corresponds to the HTK Option `ACCWINDOW`. The same
value can be given to this option.

#### -hifreq Hz

Enable band-limiting for MFCC filterbank computation: set upper
frequency cut-off. Value of -1 will disable it. (default: -1)

This option corresponds to the HTK Option `HIFREQ`. The same
value can be given to this option.

### -lofreq Hz

Enable band-limiting for MFCC filterbank computation: set lower
frequency cut-off. Value of -1 will disable it. (default: -1)

This option corresponds to the HTK Option `LOFREQ`. The same
value can be given to this option.

### -zmeanframe, -nozmeanframe

With speech input, this option enables/disables frame-wise DC
offset removal. This corresponds to HTK configuration
`ZMEANSOURCE`. This cannot be used together with -zmean.
(default: disabled)

### -usepower

Use power instead of magnitude on filterbank analysis.
(default: disabled)

## Normalization options (category `-AM` / `-AM_GMM`)

Julius can perform cepstral mean normalization (CMN) for inputs.
CMN will be activated when the given AM was trained with CMN (i.e.
has "`_Z`" qualifier in the header).

The cepstral mean will be estimated in different way according to
the input type. On file input, the mean will be computed from the
whole input. On live input such as microphone and network input,
the cepstral mean of the input is unknown at the start. So MAP-CMN
will be used. On MAP-CMN, a generic mean vector will be applied at
the beginning, and the mean vector will be smeared to the mean of
the incrementing input vector as input goes. Options below can
control the behavior of MAP-CMN.

### -cvn

Enable cepstral variance normalization. At file input, the
variance of whole input will be calculated and then applied. At
live microphone input, variance of the last input will be
applied. CVN is only supported for an audio input.  When specified,
the save/load of the CMN file by "-cmnload" and "-cmnsave"
also contains variance information to give its initial generic values.

### -vtln alpha lowCut hiCut

Do frequency warping, typically for a vocal tract length
normalization (VTLN). Arguments are warping factor, high
frequency cut-off and low freq. cut-off. They correspond to HTK
Config values, `WARPFREQ`, `WARPHCUTOFF` and `WARPLCUTOFF`.

### -cmnload file

Load generic cepstral mean vector (and variance when invoked with -cvn) from file on startup. The
file should be one saved by `-cmnsave`. Loading an generic
cepstral mean enables Julius to better recognize the first
utterance on a real-time input.  It is also required to specify this option when using `-cmnstatic` or `-cvnstatic`. When used together with
-cmnnoupdate, the value in the file will be used for all input.

### -cmnsave file

Save the calculated cepstral mean vector into file. The
parameters will be saved at each input end. If the output file
already exists, it will be overridden.

### -cmnupdate, -cmnnoupdate

Control whether to update the generic cepstral mean on
real-time input.  The update is enabled by default, the stored cepstral
mean will be updated using the last 5 seconds of last input, and the
updated mean will be used as the initial value on the next input for MAP-CMN.  Disabling the update
by `-cmnnoupdate` causes Julius to always start its MAP-CMN
at every input from the initial generic value, either 0 or the one given
by `-cmnload`. Enabled by default.

### -cmnmapweight float

Specify the weight of initial cepstral mean for MAP-CMN.
Specify larger value to retain the initial cepstral mean for a
longer period, and smaller value to make the cepstral mean rely
more on the current input. (default: 100.0)

### -cmnstatic

Perform static mean and variance normalization.  When specified, Julius always use the loaded mean and variance for all input and all frames.  This option works for both buffered input and stream input.  Requires the static mean and variance to be
loaded by `-cmnload`.

### -cvnstatic

Perform static variance normalization only.  Like `-cmnstatic` but only variance is static and mean will be computed as normal.  Requires the static variance to be loaded by `-cmnload`.  If static mean exists in the loaded file, it will be ignored.

## Front-end processing options (category `-AM` / `-AM_GMM`)

Julius can perform spectral subtraction to reduce some stationary
noise from audio input. Though it is not a powerful method, but it
may work on some situation. Julius has two ways to estimate noise
spectrum. One way is to assume that the first short segment of an
speech input is noise segment, and estimate the noise spectrum as
the average of the segment. Another way is to calculate average
spectrum from noise-only input using other tool mkss, and load it
in Julius. The former one is popular for speech file input, and
latter should be used in live input. The options below will switch
/ control the behavior.

### -sscalc

Perform spectral subtraction using head part of each file as
silence part. The head part length should be specified by
`-sscalclen`. Valid only for file input. Conflict with -ssload.

### -sscalclen msec

With `-sscalc`, specify the length of head silence for noise
spectrum estimation in milliseconds. (default: 300)

### -ssload file

Perform spectral subtraction for speech input using
pre-estimated noise spectrum loaded from file. The noise
spectrum file can be made by mkss. Valid for all speech input.
Conflict with `-sscalc`.

### -ssalpha float

Alpha coefficient of spectral subtraction for `-sscalc` and
-ssload. Noise will be subtracted stronger as this value gets
larger, but distortion of the resulting signal also becomes
remarkable. (default: 2.0)

### -ssfloor float

Flooring coefficient of spectral subtraction. The spectral
power that goes below zero after subtraction will be
substituted by the source signal with this coefficient
multiplied. (default: 0.5)

## Misc. AM options (category `-AM` / `-AM_GMM`)

### -htkconf file

Parse the given HTK Config file, and set corresponding
parameters to Julius. When using this option, the default
parameter values are switched from Julius defaults to HTK
defaults.

## 1st pass parameter options (category `SR`)

### -lmp weight penalty

(N-gram) Language model weights and word insertion penalties
for the first pass.

### -penalty1 penalty

(Grammar) word insertion penalty for the first pass. (default:
0.0)

### -b width

Beam width in number of HMM nodes for rank beaming on the first
pass. This value defines search width on the 1st pass, and has
dominant effect on the total processing time. Smaller width
will speed up the decoding, but too small value will result in
a substantial increase of recognition errors due to search
failure. Larger value will make the search stable and will lead
to failure-free search, but processing time will grow in
proportion to the width.

The default value is dependent on acoustic model type: 400
(monophone), 800 (triphone), or 1000 (triphone, setup=v2.1)

### -bs width

Score width for score pruning on first pass. This option can be
used together with rank beaming (`-b width`). The default state
is not active.

### -nlimit num

Upper limit of token per node. This option is valid when
`--enable-wpair` and `--enable-wpair-nlimit` are enabled at
compilation time.

### -progout

Enable progressive output of the partial results on the first
pass.

### -proginterval msec

Set the time interval for `-progout` in milliseconds. (default:
300)

## 2nd pass parameters options (category `SR`)

### -lmp2 weight penalty

(N-gram) Language model weights and word insertion penalties
for the second pass.

### -penalty2 penalty

(Grammar) word insertion penalty for the second pass. (default:
0.0)

### -b2 width

Envelope beam width (number of hypothesis) at the second pass.
If the count of word expansion at a certain hypothesis length
reaches this limit while search, shorter hypotheses are not
expanded further. This prevents search to fall in
breadth-first-like situation stacking on the same position, and
improve search failure mostly for large vocabulary condition.
(default: 30)

### -sb float

Score envelope width for enveloped scoring. When calculating
hypothesis score for each generated hypothesis, its trellis
expansion and Viterbi operation will be pruned in the middle of
the speech if score on a frame goes under the width. Giving
small value makes the second pass faster, but computation error
may occur. (default: 80.0)

### -s num

Stack size, i.e. the maximum number of hypothesis that can be
stored on the stack during the search. A larger value may give
more stable results, but increases the amount of memory
required. (default: 500)

### -m count

Number of expanded hypotheses required to discontinue the
search. If the number of expanded hypotheses is greater then
this threshold then, the search is discontinued at that point.
The larger this value is, The longer Julius gets to give up
search. (default: 2000)

### -n num

The number of candidates Julius tries to find. The search
continues till this number of sentence hypotheses have been
found. The obtained sentence hypotheses are sorted by score,
and final result is displayed in the order (see also the
`-output`). The possibility that the optimum hypothesis is
correctly found increases as this value gets increased, but the
processing time also becomes longer. The default value depends
on the engine setup on compilation time: 10 (standard) or 1
(fast or v2.1)

### -output num

The top N sentence hypothesis to be output at the end of
search. Use with `-n` (default: 1)

### -lookuprange frame

Set the number of frames before and after to look up next word
hypotheses in the word trellis on the second pass. This
prevents the omission of short words, but with a large value,
the number of expanded hypotheses increases and system becomes
slow. (default: 5)

### -looktrellis

(Grammar) Expand only the words survived on the first pass
instead of expanding all the words predicted by grammar. This
option makes second pass decoding faster especially for large
vocabulary condition, but may increase deletion error of short
words. (default: disabled)

## Short-pause segmentation / decoder-VAD options (category `SR`)

When compiled with `--enable-decoder-vad`, the short-pause
segmentation will be extended to support decoder-based VAD.

### -spsegment

Enable short-pause segmentation mode. Input will be segmented
when a short pause word (word with only silence model in
pronunciation) gets the highest likelihood at certain
successive frames on the first pass. When detected segment end,
Julius stop the 1st pass at the point, perform 2nd pass, and
continue with next segment. The word context will be considered
among segments. (Rev.4.0)

When compiled with `--enable-decoder-vad`, this option enables
decoder-based VAD, to skip long silence.

### -spdur frame

Short pause duration length to detect end of input segment, in
number of frames. (default: 10)

### -pausemodels string

A comma-separated list of pause model names to be used at
short-pause segmentation. The word whose pronunciation consists
of only the pause models will be treated as "pause word" and
used for pause detection. If not specified, name of -spmodel,
`-silhead` and `-siltail` will be used. (Rev.4.0)

### -spmargin frame

Back step margin at trigger up for decoder-based VAD. When
speech up-trigger found by decoder VAD, Julius will rewind the
input parameter by this value, and start recognition at the
point. (Rev.4.0)

This option will be valid only if compiled with
`--enable-decoder-vad`.

### -spdelay frame

Trigger decision delay frame at trigger up for decoder-based
VAD. (Rev.4.0)

This option will be valid only if compiled with
`--enable-decoder-vad`.

## Word lattice / confusion network output options (category `SR`)

### -lattice, -nolattice

Enable / disable generation of word graph. Search algorithm
also has changed to optimize for better word graph generation,
so the sentence result may not be the same as normal N-best
recognition. (Rev.4.0)

### -confnet, -noconfnet

Enable / disable generation of confusion network. Enabling this
will also activates `-lattice` internally. (Rev.4.0)

### -graphrange frame

Merge same words at neighbor position at graph generation. If
the beginning time and ending time of two word candidates of
the same word is within the specified range, they will be
merged. The default is 0 (allow merging same words on exactly
the same location) and specifying larger value will result in
smaller graph output. Setting this value to -1 will disable
merging, in that case same words on the same location of
different scores will be left as they are. (default: 0)

### -graphcut depth

Cut the resulting graph by its word depth at post-processing
stage. The depth value is the number of words to be allowed at
a frame. Setting to -1 disables this feature. (default: 80)

### -graphboundloop count

Limit the number of boundary adjustment loop at post-processing
stage. This parameter prevents Julius from blocking by infinite
adjustment loop by short word oscillation. (default: 20)

### -graphsearchdelay, -nographsearchdelay

When this option is enabled, Julius modifies its graph
generation algorithm on the 2nd pass not to terminate search by
graph merging, until the first sentence candidate is found.
This option may improve graph accuracy, especially when you are
going to generate a huge word graph by setting broad search.
Namely, it may result in better graph accuracy when you set
wider beams on both 1st pass `-b` and 2nd pass `-b2`, and larger
value for `-n`. (default: disabled)

## Multi-gram / multi-dic recognition options (category `SR`)

### -multigramout, -nomultigramout

On grammar recognition using multiple grammars, Julius will
output only the best result among all grammars. Enabling this
option will make Julius to output result for each grammar.
(default: disabled)

## Forced alignment options (category `SR`)

### -walign

Do viterbi alignment per word units for the recognition result.
The word boundary frames and the average acoustic scores per
frame will be calculated.

### -palign

Do viterbi alignment per phone units for the recognition
result. The phone boundary frames and the average acoustic
scores per frame will be calculated.

### -salign

Do viterbi alignment per state for the recognition result. The
state boundary frames and the average acoustic scores per frame
will be calculated.

## Misc. search options (category `SR`)

### -inactive

Start this recognition process instance with inactive state.
(Rev.4.0)

### -1pass

Perform only the first pass.

### -fallback1pass

When 2nd pass fails, Julius finish the recognition with no
result. This option tell Julius to output the 1st pass result
as a final result when the 2nd pass fails. Note that some score
output (confidence etc.) may not be useful. This was the
default behavior of Julius-3.x.

### -no_ccd, -force_ccd

Explicitly switch phone context handling at search. Normally
Julius determines whether the using AM is a context-dependent
model or not from the model names, i.e., whether the names
contain character + and -. This option will override the
automatic detection.

### -cmalpha float

Smoothing parameter for confidence scoring. (default: 0.05)

### -iwsp

(Multi-path mode only) Enable inter-word context-free short
pause insertion. This option appends a skippable short pause
model for every word end. The short-pause model can be
specified by `-spmodel`.

### -transp float

Additional insertion penalty for transparent words. (default:
0.0)

### -demo

Equivalent to `-progout -quiet`.

### -mbr

### -nombr

### -mbr_wwer

### -mbr_weight
