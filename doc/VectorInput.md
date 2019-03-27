# Vector Input

Instead of giving audio data, you can bypass the audio feature extraction and directly give Julius feature vector input computed by other tools, as file or network stream.

## Feature vector file as input

Set [-input htkparam](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-input-micfilerawfilemfcfileoutprobadinnetvecnetstdinnetaudioalsaossesdpulseaudio) to give the input in HTK parameter file format.  You can also use `-input mfcfile` and `-input mfc`, they are equivalent.  The input file names can be given from standard input, or as a list of file names can be given by [-filelist](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-filelist-filename) option.

Note that the input feature vector **should be extracted with the same training condition of the acoustic model** being applied. Julius can not automatically check their compatibility since headers include not enough information.

Base type check and delta and delta-delta coefficient modification will be tried.  When reading vector input, it checks if the base type matches, and if needed, tries to extract or suppress its delta and delta-delta values.  An input file that can not pass this check may be rejected and will not be processed. This modification process can be disabled by option[-notypecheck](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-notypecheck).

## HMM State Output Probabilities as input

Using [-input outprob](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-input-micfilerawfilemfcfileoutprobadinnetvecnetstdinnetaudioalsaossesdpulseaudio), you can give Julius state output probabilities pre-computed by other tool.   When this option is given, Julius treats the input vector as a list of output state log probabilities at each frame, and apply the values as log state output probabilities of HMM states, instead of computing acoustic probability by GMM or DNN against the input vector inside Julius.  Of course in this case the feature vector length of the input should be equal to the number of HMM states in the acoustic model, and the order should be kept between them, as is the case in DNN-HMM recognition.

## Streaming Vector Inputs

 Option [-input vecnet](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-input-micfilerawfilemfcfileoutprobadinnetvecnetstdinnetaudioalsaossesdpulseaudio) enables vector input via network stream. It can perform on-the-fly recognition for incoming vector frame chunks.  This is how a sending client should work

1. Start julius with `-input vecnet`
2. Connect to Julius.
3. Send header information.
4. Repeat:
Send feature vector chunks in turn.
Send end-of-segment code to segment input and output result.
5. Send end-of-session code to close connection.

The sample codes is in [adintool](https://github.com/julius-speech/julius/blob/master/adintool), check `vecnet_*` functions in `adintool.c`.

Both feature vector input and output probability input is supported in [-input vecnet](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-input-micfilerawfilemfcfileoutprobadinnetvecnetstdinnetaudioalsaossesdpulseaudio).  They are automatically determined at Julius side from the header information, so just give `-input vecnet` for Julius, and send appropriate header information from client.
