jconf (5 )                                                          jconf (5 )



NAME
       jconf - Jconf configuration file specification

DESCRIPTION
       The  variables  that can be written in Jconf file are organized as fol-
       lows.

       o Global options

       o Instance declaration

       o Language model instance

       o Acoustic model and speech analysis instance

       o Recognizer and search instance

       The details are described in the followings.

EXAMPLE
       These are examples of jconf file.

       First example is a simple one with no  instance  declaration.  When  no
       instance declaration is found, Julius assumes there are only one AM, LM
       and recognition process instance. In this case,  the  default  instance
       will  be  named  "_default",  and option order does not matter. This is
       equivalent to older version of Julius, except  for  GMM  handling  (see
       below).

       Example of Jconf file: no instance declaration


             -C jconffile
             (Other global options...)
             (AM and analysis options...)
             (LM options...)
             (Search options...)


       This  is an example using two acoustic models and three language models
       of different types. Three recognition process instance is  defined  for
       each  combination of AM and LM. The LM type (ngram / grammar / word) is
       determined by the arguments. The Global options are placed at  the  top
       in the example, but actually it can be placed anywhere in the file.

       Example of Jconf file: multi model decoding


             -C jconffile
             (Other global options...)
             -AM am1
             (AM and analysis options for am1...)
             -AM am2
             (AM and analysis options for am2...)
             -LM lm_ngram
             -d ngram -v dictfile
             (LM options for lm1...)
             -LM lm_grammar
             -gram grammarprefix
             (LM options for lm2...)
             -LM lm_word
             -w dictfile
             (LM options for lm3...)
             -SR recog_ngram am1 lm_ngram
             (Search options for recog_ngram...)
             -SR recog_grammar am1 lm_grammar
             (Search options for recog_ngram...)
             -SR recog_word am2 lm_word
             (Search options for recog_ngram...)


       This  is  another  example using GMM for frontend processing. Note that
       from Rev.4.0 Julius has independent MFCC calculation  scheme  for  GMM.
       This  means  that  you  should explicitly specify the acoustic analysis
       condition for GMM, not only the AM.

       Option -AM_GMM switch the current AM configuration to the one  prepared
       internally  for GMM. You can place AM configuration after the option to
       specify MFCC computation parameter for GMM.  If you define exactly  the
       same  condition  as  AM  for  recognition,  the  same  MFCC calculation
       instance will be shared among AM and GMM.  Else, each MFCC will be com-
       puted independently.

       Example with GMM


             -C jconffile
             (Other global options...)
             -gmm gmmdefs -gmmreject noise
             -AM_GMM
             (analysis options for GMM...)
             -AM am1
             (AM and analysis options for am1...)
             -LM lm_ngram
             -d ngram -v dictfile
             (LM options for lm1...)
             -SR recog_ngram am1 lm_ngram


JCONF VARIABLES
       The  full  list of options and variables that can be specified in jconf
       file is listed below.

   GLOBAL OPTIONS
   Misc. options
       -C jconffile
              Load a jconf file. The options written in the file are  expanded
              at the point. This option can be used within other jconf file.

       -version
              Print version information to standard error, and exit.

       -setting
              Print engine setting information to standard error, and exit.

       -quiet Output less log. For result, only the best word sequence will be
              printed.

       -debug (For debug) output enoumous internal message and debug  informa-
              tion to log.

       -check {wchmm|trellis|triphone}
              For debug, enter interactive check mode.

   Audio input
       -input {mic|rawfile|mfcfile|adinnet|stdin|netaudio}
              Choose  speech  input  source.  'file' or 'rawfile' for waveform
              file, 'htkparam' or 'mfcfile' for HTK parameter file. Users will
              be  prompted  to  enter the file name from stdin, or you can use
              "-filelist" option to specify list of files to process.

              'mic' is to get audio input from  live  microphone  device,  and
              'adinnet'  means  receiving waveform data via tcpip network from
              an adinnet client. 'netaudio' is  from  DatLink/NetAudio  input,
              and 'stdin' means data input from standard input.

              For  waveform  file  input,  only  WAV  (no compression) and RAW
              (noheader, 16bit, big endian) are supported  by  default.  Other
              format  can  be  read  when compiled with libsnd library. To see
              what format is actually supported, see the  help  message  using
              option  "-help". For stdin input, only WAV and RAW is supported.
              (default: mfcfile)

       -filelist filename
              (With -input rawfile|mfcfile) perform recognition on  all  files
              listed  in  the  file. The file should contain an input file per
              line. Engine ends when all of the files are processed.

       -notypecheck
              By default, Julius checks the input parameter  type  whether  it
              matches  the  AM  or not. This option will disable the check and
              use the input vector as is.

       -48    Record input with 48kHz sampling, and down-sample  it  to  16kHz
              on-the-fly.  This  option  is  valid  for  16kHz model only. The
              down-sampling routine was ported from sptk.  (Rev. 4.0)

       -NA devicename
              Host name for DatLink server input (-input netaudio).

       -adport port_number
              With -input adinnet, specify  adinnet  port  number  to  listen.
              (default: 5530)

       -nostrip
              Julius  by  default  removes  successive  zero  samples in input
              speech data. This option inhibits this removal.

       -zmean , -nozmean
              This option enables/disables DC offset removal  of  input  wave-
              form.  Offset will be estimated from the whole input. For micro-
              phone / network input, zero mean of the first 48000  samples  (3
              seconds  in  16kHz  sampling)  will  be used for the estimation.
              (default: disabled)

              This option uses static offset for the channel. See also -zmean-
              source for frame-wise offset removal.

   Speech segment detection by level and zero-cross
       -cutsilence , -nocutsilence
              Turn  on  /  off  the  speech detection by level and zero-cross.
              Default is on for mic / adinnet input, off for files.

       -lv thres
              Level threshold for speech input  detection.  Values  should  be
              from 0 to 32767.

       -zc thres
              Zero  crossing  threshold  per second. Only waves over the level
              threshold (-lv) will be counted. (default: 60)

       -headmargin msec
              Silence margin at the start of speech segment  in  milliseconds.
              (default: 300)

       -tailmargin msec
              Silence  margin  at  the  end of speech segment in milliseconds.
              (default: 400)

       -rejectshort msec
              Reject input shorter than specified milliseconds. Search will be
              terminated and no result will be output.

   Input rejection by average power
       This  feature  will be enabled by --enable-power-reject on compilation.
       Should be used with Decoder VAD or GMM VAD. Valid for  real-time  input
       only.

       -powerthres thres
              Reject  the inputted segment by its average energy. If the aver-
              age energy of the last recognized input is below the  threshold,
              Julius will reject the input. (Rev.4.0)

              This  option is valid when --enable-power-reject is specified at
              compilation time.

   Gaussian mixture model
       GMM will be used for input  rejection  by  accumurated  score,  or  for
       GMM-based frontend VAD when --enable-gmm-vad is specified.

       NOTE:  You  should also set the proper MFCC parameters required for the
       GMM,  specifying  the  acoustic  parameters  described  in  AM  section
       -AM_GMM.

       -gmm hmmdefs_file
              GMM definition file in HTK format. If specified, GMM-based input
              verification will be performed concurrently with the  1st  pass,
              and  you  can reject the input according to the result as speci-
              fied by -gmmreject. The GMM should be defined as one-state HMMs.

       -gmmnum number
              Number  of  Gaussian  components to be computed per frame on GMM
              calculation. Only the N-best  Gaussians  will  be  computed  for
              rapid  calculation.  The  default  is  10 and specifying smaller
              value will speed up GMM calculation, but too small value  (1  or
              2) may cause degradation of identification performance.

       -gmmreject string
              Comma-separated  list  of  GMM  names  to be rejected as invalid
              input. When recognition, the log likelihoods of GMMs accumulated
              for  the entire input will be computed concurrently with the 1st
              pass. If the GMM name  of  the  maximum  score  is  within  this
              string,  the 2nd pass will not be executed and the input will be
              rejected.

       -gmmmargin frames
              Head margin for GMM-based VAD in frames. (Rev.4.0)

              This   option   will   be   valid   only   if   compiled    with
              --enable-gmm-vad.

   Decoding option
       Real-time  processing  means  concurrent processing of MFCC computation
       1st pass decoding. By default, real-time processing on the pass  is  on
       for microphone / adinnet / netaudio input, and for others.

       -realtime , -norealtime
              Explicitly  switch  on / off real-time (pipe-line) processing on
              the first pass. The default is off for file input,  and  on  for
              microphone,  adinnet  and NetAudio input. This option relates to
              the way CMN and energy normalization is performed: if off,  they
              will  be  done  using  average  features  of whole input. If on,
              MAP-CMN and energy normalization to do rea-time processing.

   INSTANCE DECLARATION FOR MULTI DECODING
       The following arguments  will  create  a  new  configuration  set  with
       default  parameters,  and  switch  current  set to it. Jconf parameters
       specified after the option will be set into the current set.

       To do multi-model decoding, these argument should be specified  at  the
       first  of  each  model  /  search  instances with different names.  Any
       options before the first instance definition will be IGNORED.

       When no instance definition is found (as older version of Julius),  all
       the options are assigned to a default instance named "_default".

       Please  note  that  decoding  with  a single LM and multiple AMs is not
       fully supported. For example, you may want to construct the jconf  file
       as following.


        -AM am_1 -AM am_2
        -LM lm (LM spec..)
        -SR search1 am_1 lm
        -SR search2 am_2 lm

       This  type of model sharing is not supported yet, since some part of LM
       processing depends on the assigned AM. Instead, you can  get  the  same
       result by defining the same LMs for each AM, like this:


        -AM am_1 -AM am_2
        -LM lm_1 (LM spec..)
        -LM lm_2 (same LM spec..)
        -SR search1 am_1 lm_1
        -SR search2 am_2 lm_2


       -AM name
              Create a new AM configuration set, and switch current to the new
              one. You should give a unique name. (Rev.4.0)

       -LM name
              Create a new LM configuration set, and switch current to the new
              one. You should give a unique name. (Rev.4.0)

       -SR name am_name lm_name
              Create a new search configuration set, and switch current to the
              new one. The specified AM and LM will be assigned  to  it.   The
              am_name  and lm_name can be either name or ID number. You should
              give a unique name. (Rev.4.0)

       -AM_GMM
              A special command to switch AM configuration set for  specifying
              speech  analysis  parameters  of  GMM.  The  current  AM will be
              switched to the GMM specific one already reserved, so be careful
              not to confuse with normal AM configurations.  (Rev.4.0)

   LANGUAGE MODEL (-LM)
       Only  one  type  of LM can be specified for a LM configuration.  If you
       want to use multi model, you should define them one by one, each  as  a
       new LM.

   N-gram
       -d bingram_file
              Use  binary  format N-gram. An ARPA N-gram file can be converted
              to Julius binary format by mkbingram.

       -nlr arpa_ngram_file
              A forward, left-to-right N-gram language model in standard  ARPA
              format. When both a forward N-gram and backward N-gram are spec-
              ified, Julius uses this forward 2-gram for the 1st pass, and the
              backward N-gram for the 2nd pass.

              Since  ARPA  file  often gets huge and requires a lot of time to
              load, it may be better to convert the ARPA file to Julius binary
              format  by  mkbingram.  Note  that  if both forward and backward
              N-gram is used for recognition, they  together  should  be  con-
              verted to a single binary.

              When  only  a  forward N-gram is specified by this option and no
              backward N-gram specified by -nrl, Julius  performs  recognition
              with  only  the forward N-gram. The 1st pass will use the 2-gram
              entry in the given N-gram, and The 2nd pass will use  the  given
              N-gram, with converting forward probabilities to backward proba-
              bilities by Bayes rule.  (Rev.4.0)

       -nrl arpa_ngram_file
              A backward, right-to-left N-gram language model in standard ARPA
              format. When both a forward N-gram and backward N-gram are spec-
              ified, Julius uses the forward 2-gram for the 1st pass, and this
              backward N-gram for the 2nd pass.

              Since  ARPA  file  often gets huge and requires a lot of time to
              load, it may be better to convert the ARPA file to Julius binary
              format  by  mkbingram.  Note  that  if both forward and backward
              N-gram is used for recognition, they  together  should  be  con-
              verted to a single binary.

              When  only  a backward N-gram is specified by this option and no
              forward N-gram specified by -nlr,  Julius  performs  recognition
              with only the backward N-gram. The 1st pass will use the forward
              2-gram probability computed from the backward 2-gram using Bayes
              rule.  The  2nd  pass  fully  use  the  given  backward  N-gram.
              (Rev.4.0)

       -v dict_file
              Word dictionary file.

       -silhead word_string -siltail word_string
              Silence word defined in the  dictionary,  for  silences  at  the
              beginning  of  sentence  and  end  of sentence. (default: "<s>",
              "</s>")

       -iwspword
              Add a word entry to the dictionary  that  should  correspond  to
              inter-word pauses. This may improve recognition accuracy in some
              language model that has no explicit inter-word  pause  modeling.
              The word entry to be added can be changed by -iwspentry.

       -iwspentry word_entry_string
              Specify  the  word  entry  that  will  be  added  by  -iwspword.
              (default: "<UNK> [sp] sp sp")

       -sepnum number
              Number of high frequency words to be isolated from  the  lexicon
              tree,  to  ease  approximation  error  that may be caused by the
              one-best approximation on 1st pass. (default: 150)

   Grammar
       Multiple grammars can be specified by using -gram and  -gramlist.  When
       you  specify  grammars  using these options multiple times, all of them
       will be read at startup. Note that this is unusual behavior from  other
       options (in normal Julius option, last one override previous ones). You
       can use -nogram to reset the already specified grammars at that  point.

       -gram gramprefix1[,gramprefix2[,gramprefix3,...]]
              Comma-separated list of grammars to be used. the argument should
              be prefix of a grammar, i.e. if you have foo.dfa  and  foo.dict,
              you  can  specify them by single argument foo. Multiple grammars
              can be specified at a time as a comma-separated list.

       -gramlist list_file
              Specify a grammar list file that contains list of grammars to be
              used.  The  list  file  should contain the prefixes of grammars,
              each per line. A relative path in the list file will be  treated
              as relative to the list file, not the current path or configura-
              tion file.

       -dfa dfa_file -v dict_file
              An old way of specifying grammar files separately.

       -nogram
              Remove the current list of grammars already specified by  -gram,
              -gramlist, -dfa and -v.

   Isolated word
       Multiple  dictionary  can be specified by using -w and -wlist. When you
       specify multiple times, all of them will be read at  startup.  You  can
       use  -nogram to reset the already specified dictionaries at that point.

       -w dict_file
              Word dictionary for isolated word recognition.  File  format  is
              the same as other LM. (Rev.4.0)

       -wlist list_file
              Specify  a dictionary list file that contains list of dictionar-
              ies to be used. The list file should contain the  file  name  of
              dictionaries,  each  per  line. A relative path in the list file
              will be treated as relative to the list file,  not  the  current
              path or configuration file. (Rev.4.0)

       -nogram
              Remove  the current list of dictionaries already specified by -w
              and -wlist.

       -wsil head_sil_model_name tail_sil_model_name sil_context_name
              On isolated word recognition, silence models will be appended to
              the head and tail of each word at recognition. This option spec-
              ifies the silence models to be  appended.   sil_context_name  is
              the  name  of the head sil model and tail sil model as a context
              of word head phone and tail phone. For example, if  you  specify
              -wsil  silB  silE  sp, a word with phone sequence b eh t will be
              translated as silB sp-b+eh b-eh+t eh-t+sp silE.  (Rev.4.0)

   User-defined LM
       -userlm
              Declare to use user LM defined in program. This option should be
              specified if you use user-defined LM function. (Rev.4.0)

   Misc LM options
       -forcedict
              Ignore  dictionary  errors  and force running. Words with errors
              will be skipped at startup.

   ACOUSTIC MODEL AND SPEECH ANALYSIS (-AM) (-AM_GMM)
       Acoustic analysis parameters are included in this section, since the AM
       defines  the  required  parameter.  You can use different MFCC type for
       each AM. For GMM, the same parameter should be specified after -AM_GMM

       When using multiple AM, the values of -smpPeriod, -smpFreq, -fsize  and
       -fshift should have the same value among all AMs.

   acoustic HMM and parameters
       -h hmmdef_file
              Acoustic  HMM  definition file. File should be in HTK ascii for-
              mat, or Julius binary format. You can convert HTK ascii  hmmdefs
              to Julius binary format by mkbinhmm.

       -hlist hmmlist_file
              HMMList  file  for  phone mapping. This options is required when
              using a triphone model. This file  provides  a  mapping  between
              logical  triphone  names  genertated  from  the  dictionary  and
              defined HMM names in hmmdefs.

       -tmix number
              Specify the number of top Gaussians to be calculted in a mixture
              codebook.  Small  number  will speed up the acoustic computation
              namely in a tied-mixture model, but AM accuracy may get worse on
              too small value. (default: 2)

       -spmodel name
              Specify  an  HMM  name  that corresponds to short-pause model in
              HMM. This option will affect  various  aspects  in  recognition:
              short-pause  skipping  process  on grammar recognition, word-end
              short-pause model insertion with -iwsp on N-gram recognition, or
              short-pause segmentation (-spsegment). (default: "sp")

       -multipath
              Enable  multi-path mode. Multi-path mode expand state transition
              availability to allow model-skipping, or  multiple  output/input
              transitions  in  HMMs.  However,  since defining additional word
              begin / end node and perform extra transition check on decoding,
              the  beam  width  may  be required to set larger and recognition
              becomes a bit slower.

              By default (without this option), Julius automatically check the
              transition  type  of  specified  HMMs, and enable the multi-path
              mode if required. You can force Julius to enable multi-path mode
              with this option. (rev.4.0)

       -gprune {safe|heuristic|beam|none|default}
              Set  Gaussian pruning algotrihm to use. The default setting will
              be  set  according  to  the  model  type  and  engine   setting.
              "default"  will force accepting the default setting. Set this to
              "none" to disable pruning and perform full  computation.  "safe"
              gualantees  the  top N Gaussians to be computed. "heuristic" and
              "beam" do more aggressive computational cosst reduction, but may
              result  in  small loss of accuracy model (default: 'safe' (stan-
              dard), 'beam' (fast) for tied  mixture  model,  'none'  for  non
              tied-mixture model).

       -iwcd1 {max|avg|best number}
              Select method to approximate inter-word triphone on the head and
              tail of a word in the first pass.

              "max" will apply the maximum likelihood of the same context tri-
              phones. "avg" will apply the average likelihood of the same con-
              text triphones. "best number" will  apply  the  average  of  top
              N-best likelihoods of the same context triphone.

              Default  is  "best 3" for use with N-gram, and "avg" for grammar
              and word. When this AM is shared by LMs of both type, latter one
              will be chosen.

       -iwsppenalty float
              Short  pause  insertion  penalty  for  appended  short pauses by
              -iwsp.

       -gshmm hmmdef_file
              If this option is specified, Julius  performs  Gaussian  Mixture
              Selection  for efficient decoding. The hmmdefs should be a mono-
              phone model generated from  an  ordinary  monophone  HMM  model,
              using mkgshmm.

       -gsnum number
              On  GMS,  specify  number of monophone state from top to compute
              the detailed corresponding triphones. (default: 24)

   Speech analysis parameters
       -smpPeriod period
              Set sampling frequency of input speech by its  sampling  period,
              in  unit of 100 nanoseconds. Sampling rate can also be specified
              by -smpFreq. Please note that the input frequency should be  the
              same  as trained conditions of acoustic model you use. (default:
              625 = 16000Hz)

              This option corresponds to the  HTK  Option  "SOURCERATE".   The
              same value can be given to this option.

              When  using multiple AM, this value should be the same among all
              AMs.

       -smpFreq Hz
              Set sampling frequency of input speech in Hz. Sampling rate  can
              also be specified using "-smpPeriod". Please note that this fre-
              quency should be the same as the trained conditions of  acoustic
              model you use. (default: 16000)

              When  using multiple AM, this value should be the same among all
              AMs.

       -fsize sample_num
              Window size in number of samples. (default: 400)

              This option corresponds to  the  HTK  Option  "WINDOWSIZE",  but
              value should be in samples (HTK value / smpPeriod).

              When  using multiple AM, this value should be the same among all
              AMs.

       -fshift sample_num
              Frame shift in number of samples. (default: 160)

              This option corresponds to  the  HTK  Option  "TARGETRATE",  but
              value should be in samples (HTK value / smpPeriod).

              When  using multiple AM, this value should be the same among all
              AMs.

       -preemph float
              Pre-emphasis coefficient. (default: 0.97)

              This option corresponds to the HTK Option "PREEMCOEF".  The same
              value can be given to this option.

       -fbank num
              Number of filterbank channels. (default: 24)

              This  option corresponds to the HTK Option "NUMCHANS".  The same
              value can be given to this option.  Be aware  that  the  default
              value differs from HTK (22).

       -ceplif num
              Cepstral liftering coefficient. (default: 22)

              This option corresponds to the HTK Option "CEPLIFTER".  The same
              value can be given to this option.

       -rawe , -norawe
              Enable/disable using raw energy  before  pre-emphasis  (default:
              disabled)

              This option corresponds to the HTK Option "RAWENERGY".  Be aware
              that the default value differs from HTK (enabled  at  HTK,  dis-
              abled at Julius).

       -enormal , -noenormal
              Enable/disable  normalizing log energy. On live input, this nor-
              malization will be approximated from the average of last  input.
              (default: disabled)

              This  option  corresponds  to  the  HTK Option "ENORMALISE".  Be
              aware that the default value differs from HTK (enabled  at  HTK,
              disabled at Julius).

       -escale float_scale
              Scaling  factor  of  log  energy  when  normalizing  log energy.
              (default: 1.0)

              This option corresponds to the HTK Option  "ESCALE".   Be  aware
              that the default value differs from HTK (0.1).

       -silfloor float
              Energy   silence  floor  in  dB  when  normalizing  log  energy.
              (default: 50.0)

              This option corresponds to the HTK Option "SILFLOOR".

       -delwin frame
              Delta window size in number of frames. (default: 2)

              This option corresponds to the HTK  Option  "DELTAWINDOW".   The
              same value can be given to this option.

       -accwin frame
              Acceleration window size in number of frames. (default: 2)

              This option corresponds to the HTK Option "ACCWINDOW".  The same
              value can be given to this option.

       -hifreq Hz
              Enable band-limiting for MFCC filterbank computation: set  upper
              frequency cut-off. Value of -1 will disable it.  (default: -1)

              This  option  corresponds  to the HTK Option "HIFREQ".  The same
              value can be given to this option.

       -lofreq Hz
              Enable band-limiting for MFCC filterbank computation: set  lower
              frequency cut-off. Value of -1 will disable it.  (default: -1)

              This  option  corresponds  to the HTK Option "LOFREQ".  The same
              value can be given to this option.

       -zmeanframe , -nozmeanframe
              With speech input, this option  enables/disables  frame-wise  DC
              offset  removal.  This  corresponds  to HTK configuration ZMEAN-
              SOURCE. This cannot be used with "-zmean".  (default: disabled)

   Real-time cepstral mean normalization
       -cmnload file
              Load initial cepstral mean vector from file on startup. The file
              shoudld  be  one saved by -cmnsave.  Loading an initial cepstral
              mean enables Julius to better recognize the first utterance on a
              microphone / network input.

       -cmnsave file
              Save  cepstral mean vector at each input. The parameters will be
              saved to the file at each input end, so the output  file  always
              keeps  the  last cepstral mean. If output file already exist, it
              will be overridden.

       -cmnupdate -cmnnoupdate
              Control whether to update the cepstral mean  at  each  input  on
              microphone  / network input. Disabling this and specifying -cmn-
              load will make engine to use the initial  cepstral  mean  parma-
              nently.

       -cmnmapweight float
              Specify  weight  of  initial  cepstral mean for MAP-CMN. Specify
              larger value to retain the initial cepstral mean  for  a  longer
              period,  and  smaller  value  to rely more on the current input.
              (default: 100.0)

   Spectral subtraction
       -sscalc
              Perform spectral subtraction  using  head  part  of  each  file.
              Valid only for raw speech file input. Conflict with -ssload.

       -sscalclen msec
              With  -sscalc,  specify  the length of head part silence in mil-
              liseconds. (default: 300)

       -ssload file
              Perform spectral subtraction for speech  input  using  pre-esti-
              mated  noise  spectrum  from  file. The noise spectrum should be
              computed beforehand by mkss.  Valid for all speech  input.  Con-
              flict with -sscalc.

       -ssalpha float
              Alpha  coefficient  of  spectral  subtraction  for  -sscalc  and
              -ssload.  Noise will be subtracted stronger as this  value  gets
              larger,  but  distortion  of  the  resulting signal also becomes
              remarkable. (default: 2.0)

       -ssfloor float
              Flooring coefficient of spectral subtraction. The spectral power
              that  goes  below  zero after subtraction will be substituted by
              the source signal with this  coefficient  multiplied.  (default:
              0.5)

   Misc AM options
       -htkconf file
              Parse  the  given HTK Config file, and set corresponding parame-
              ters to Julius. When using this option,  the  default  parameter
              values are switched from Julius defaults to HTK defaults.

   RECOGNIZER AND SEARCH (-SR)
       Default  values  for beam width and LM weights will change according to
       compile-time setup of JuliusLib and model specification. Please see the
       startup log for the actual values.

   General parameters
       -inactive
              Start  this  recognition  process  instance with inactive state.
              (Rev.4.0)

       -1pass Perform only the first pass. This mode is automatically  set  at
              isolated word recognition.

       -no_ccd , -force_ccd
              Normally  Julius determines whether the specified acoustic model
              is a context-dependent model from the model names, i.e., whether
              the  model  names  contain character + and -. You can explicitly
              specify by these options to avoid  mis-detection.  These  option
              will override automatic detection.

       -cmalpha float
              Smoothing patemeter for confidence scoring. (default: 0.05)

       -iwsp  (Multi-path  mode  only)  Enable  inter-word  context-free short
              pause handling. This option  appends  a  skippable  short  pause
              model  for  every  word  end. The added model will be skipped on
              inter-word context handling. The HMM model to be appended can be
              specified by -spmodel.

       -transp float
              Additional  insertion  penalty  for transparent words. (default:
              0.0)

       -demo  Equivalent to -progout -quiet.

   1st pass parameters
       -lmp weight penalty
              (N-gram) Language model weights and word insertion penalties for
              the first pass.

       -penalty1 penalty
              (Grammar)  word  insertion penalty for the first pass. (default:
              0.0)

       -b width
              Beam width for rank beam in number of HMM  nodes  on  the  first
              pass.  This  value defines search width on the 1st pass, and has
              great effect on the total processing time.  Smaller  width  will
              speed up the decoding, but too small value will result in a sub-
              stantial increase of recognition errors due to  search  failure.
              Larger  value will make the search stable and will lead to fail-
              ure-free search, but processing time and memory usage will  grow
              in proportion to the width.

              The  default  value  is  dependent  on  acoustic model type: 400
              (monophone), 800 (triphone), or 1000 (triphone, setup=v2.1)

       -nlimit num
              Upper limit of  token  per  node.  This  option  is  valid  when
              --enable-wpair and --enable-wpair-nlimit are enabled at compila-
              tion time.

       -progout
              Enable progressive output of the partial results  on  the  first
              pass.

       -proginterval msec
              Set the output time interval of -progout in milliseconds.

   2nd pass parameters
       -lmp2 weight penalty
              (N-gram) Language model weights and word insertion penalties for
              the second pass.

       -penalty2 penalty
              (Grammar) word insertion penalty for the second pass.  (default:
              0.0)

       -b2 width
              Envelope  beam  width  (number of hypothesis) in second pass. If
              the count of word expantion at a certain  length  of  hypothesis
              reaches  this  limit  while  search,  shorter hypotheses are not
              expanded   further.   This   prevents   search   to   fall    in
              breadth-first-like  status  stacking  on  the same position, and
              improve search failure. (default: 30)

       -sb float
              Score envelope width for  enveloped  scoring.  When  calculating
              hypothesis  score  for  each  generated  hypothesis, its trellis
              expansion and viterbi operation will be pruned in the middle  of
              the  speech  if  score  on a frame goes under the width.  Giving
              small value makes the second pass faster, but computation  error
              may occur. (default: 80.0)

       -s num Stack  size,  i.e.  the maximum number of hypothesis that can be
              stored on the stack during the search. A larger value  may  give
              more   stable  results,  but  increases  the  amount  of  memory
              required. (default: 500)

       -m count
              Number  of  expanded  hypotheses  required  to  discontinue  the
              search.  If  the  number  of expanded hypotheses is greater then
              this threshold then, the search is discontinued at  that  point.
              The  larger  this  value  is,  The longer Julius gets to give up
              search. (default: 2000)

       -n num The number of candidates Julius tries to find. The  search  con-
              tinues  till this number of sentence hypotheses have been found.
              The obtained sentence hypotheses are sorted by score, and  final
              result  is  displayed  in  the order (see also the -output). The
              possibility that  the  optimum  hypothesis  is  correctly  found
              increases  as this value gets increased, but the processing time
              also becomes longer. The default value  depends  on  the  engine
              setup on compilation time: 10 (standard) or 1 (fast or v2.1)

       -output num
              The top N sentence hypothesis to be output at the end of search.
              Use with -n (default: 1)

       -lookuprange frame
              When performing word expansion on the second pass,  this  option
              sets  the number of frames before and after to look up next word
              hypotheses in the word trellis. This prevents  the  omission  of
              short  words,  but  with  a  large value, the number of expanded
              hypotheses increases and system becomes slow. (default: 5)

       -looktrellis
              (Grammar) Expand only the  words  survived  on  the  first  pass
              instead  of  expanding  all the words predicted by grammar. This
              option makes second pass decoding slightly faster especially for
              large  vocabulary  condition, but may increase deletion error of
              short words. (default: disabled)

   Short-pause segmentation
       When compiled with --enable-decoder-vad, the  short-pause  segmentation
       will be extended to support decoder-based VAD.

       -spsegment
              Enable  short-pause  segmentation  mode. Input will be segmented
              when a short pause word (word with only silence model in pronun-
              ciation)  gets  the  highest  likelihood  at  certain successive
              frames on the first pass. When detected segment end, Julius stop
              the  1st  pass at the point, perform 2nd pass, and continue with
              next segment. The word context will  be  considered  among  seg-
              ments. (Rev.4.0)

              When  compiled  with  --enable-decoder-vad,  this option enables
              decoder-based VAD, to skip long silence.

       -spdur frame
              Short pause duration length to detect end of input  segment,  in
              number of frames. (default: 10)

       -pausemodels string
              A  comma-separated  list  of  pause  model  names  to be used at
              short-pause segmentation. The word with only  the  pause  models
              will  be  treated  as "pause word" for pause detectionin. If not
              specified, name of -spmodel, -silhead and -siltail will be used.
              (Rev.4.0)

       -spmargin frame
              Backstep margin at trigger up for decoder-based VAD. (Rev.4.0)

              This    option   will   be   valid   only   if   compiled   with
              --enable-decoder-vad.

       -spdelay frame
              Trigger decision delay frame at  trigger  up  for  decoder-based
              VAD. (Rev.4.0)

              This    option   will   be   valid   only   if   compiled   with
              --enable-decoder-vad.

   Lattice / confusion network output
       -lattice , -nolattice
              Enable / disable generation of word graph. Search algorithm also
              has changed to optimize for better word graph generation, so the
              sentence result may not be the same as  normal  N-best  recogni-
              tion. (Rev.4.0)

       -confnet , -noconfnet
              Enable  / disable generation of confusion network. Enabling this
              will also activates -lattice internally.  (Rev.4.0)

       -graphrange frame
              Merge same words at neighbor position at  graph  generation.  If
              the position of same words differs smaller than this value, they
              will be merged. The default is 0 (allow merging on  exactly  the
              same  location)  and  specifying  larger  value  will  result in
              smaller graph output. Setting to -1  will  disable  merging,  in
              that  case  same  words on the same location of different scores
              will be left as they are. (default: 0)

       -graphcut depth
              Cut the resulting graph by its  word  depth  at  post-processing
              stage. The depth value is the number of words to be allowed at a
              frame. Setting to -1 disables this feature. (default: 80)

       -graphboundloop count
              Limit the number of boundary adjustment loop at  post-processing
              stage.  This parameter prevents Julius from blocking by infinite
              adjustment loop by short word oscillation. (default: 20)

       -graphsearchdelay , -nographsearchdelay
              When "-graphsearchdelay" option  is  set,  Julius  modifies  its
              graph  generation  alogrithm  on  the  2nd pass not to terminate
              search by graph merging, until the first sentence  candidate  is
              found.  This  option may improve graph accuracy, especially when
              you are going to generate a huge word  graph  by  setting  broad
              search.  Namely, it may result in better graph accuracy when you
              set wide beams on both 1st pass -b and 2nd pass -b2,  and  large
              number for -n. (default: disabled)

   Multi-gram / multi-dic output
       -multigramout , -nomultigramout
              On grammar recognition using multiple grammars, Julius will out-
              put only the best  result  among  all  grammars.  Enabling  this
              option  will  make  Julius  to  output  result for each grammar.
              (default: disabled)

   Forced alignment
       -walign
              Do viterbi alignment per word units for the recognition  result.
              The  word  boundary  frames  and the average acoustic scores per
              frame will be calculated.

       -palign
              Do viterbi alignment per phone units for the recognition result.
              The  phone  boundary  frames and the average acoustic scores per
              frame will be calculated.

       -salign
              Do viterbi alignment per state for the recognition result.   The
              state  boundary frames and the average acoustic scores per frame
              will be calculated.



                                                                    jconf (5 )
