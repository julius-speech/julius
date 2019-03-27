# Rejection

Julius can detect and reject an invalid input based on several schemes. Unlike the [front-end VAD](VAD.md), the rejection is input-wise: rejection will be judged at the **end of input after processing** using information of the whole input.  When an input was rejected, Julius does not output recognition result for the input and just output information that the last input was rejected.

Several criteria are implemented, as described below.

## Rejection by Input length

Too short input can be rejected by specifying an option [-rejectshort](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-rejectshort-msec).  An audio input which is shorter than the specified length will be rejected.  This feature can be used to avoid mis-triggering of Julius by an impulsive noise.  Also too long input can be rejected by [-rejectlong](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-rejectlong-msec).

-powerthres threshold
Reject the inputted segment by its average energy. If the average energy of the last recognized input is below the threshold, Julius will reject the input. (Rev.4.0)

This option is valid when --enable-power-reject is specified at compilation time.

## Rejection by Average Power

Average power of the detected segment can be used for rejection.  This featurecan be enabled by specifying  `--enable-power-reject` for configure option at compilation.  This option is only valid for MFCC feature with absolute power coefficient, and can be applied to stream input only.  When enabled, an option [-powerthres](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-powerthres-threshold) can be used to set the threshold.

## Rejection by GMM score

GMM-based input rejection can be applied when given voice / noise GMM.  This feature is exclusive with [GMM-based VAD](https://github.com/julius-speech/julius/blob/master/doc/VAD.md#static-gmm-based-detector): when Julius is built with configuration option `--enable-gmm-vad`, the given GMM will be used at [front-end voice activity detection](https://github.com/julius-speech/julius/blob/master/doc/VAD.md#static-gmm-based-detector).  If not compiled with that option, the given GMM will be used for input rejection as described below.

When a GMM model is given with [-gmm](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-gmm-hmmdefs_file) option, the input feature will be fed into all GMMs at each frame concurrently with the recognition process.  The scores are accumulated among the whole input, and when the end of the input is detected, the average scores of all the models are compared.
Then, if the name of the model which got the maximum score is included in the string specified by [-gmmreject](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-gmmreject-string), the input will be rejected.

The GMM model should be given as a 3-state (1 output state) HMM in the same format as acoustic model.  Any number of voice GMMs and noise GMMs can be defined in the model, and the list of the names of GMMs to be rejected should be given by [-gmmreject](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-gmmreject-string).  If the feature parameter type for the GMM is different from the acoustic model being used together, the proper feature parameters for the GMM should be explicitly specified in a special configuration section [-AM_GMM](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-am_gmm).

When a GMM is given, Julius just compute their scores per input frame and outputs the name of the best matched model at each input.  Thus this scheme can also be used for sound recognition concurrently run with the recognition process, such as laughter detection, unintentional sound input rejection, and so on.
