
# Specifications

## Audio input

[Audio.md](Audio.md).

## Voice Activity Detection

[VAD.md](VAD.md)

## Feature extraction

`julius` can extract features from waveform input.  The module is built to have compatibility with HTK.  Supported base types are FBANK (log-scale filterbank parameter), MELSPEC (mel-scale filterbank parameter) and MFCC (mel-frequency cepstral coefficients).  Supported qualifiers are `_E`, `_N`, `_D`, `_A`, `_Z` and `_0`.  All configuration parameter's default are

The feature extraction parameters should be set up as the same as training condition of acoustic model.  The following table shows all the feature extraction parameters, with their default values and how to set.

|HTK Option|Description|HTK default|Julius default|How to set|
|:--|:--|:--|:--|:--|
`TARGETKIND`|Parameter kind|ANON|MFCC|automatically set from HMM header
`NUMCEPS`|Number of cepstral parameters|12|-|automatically set from HMM header
`SOURCERATE`|Sample rate of source waveform in 100ns units|0.0|625|"`-smpPeriod value`"
`TARGETRATE`|Sample rate of target vector (= window shift) in 100ns units|0.0|160|"`-fshift samples`" (*)
`WINDOWSIZE`|Analysis window size in 100ns units|256000.0|400|"`-fsize samples`" (*)
`ZMEANSOURCE`|Zero mean source waveform before analysis (frame-wise)|F|F|"`-zmeanframe`" to enable, "`-nozmeanframe`" to disable.
`PREEMCOEF`|Set pre-emphasis coefficient|0.97|0.97|"`-preemph value`"
`USEHAMMING`|Use a Hamming window|T|T|Fixed
`NUMCHANS`|Number of filerbank channels|20|24|"`-fbank value`"
`CEPLIFTER`|Cepstral liftering coefficient|22|22|"`-ceplif value`"
`DELTAWINDOW`|Delta window size in frame|2|2|"`-delwin value`"
`ACCWINDOW`|Acceleration window size in frame|2|2|"`-accwin value`"
`LOFREQ`|Low frequency cut-off in fbank analysis|-1.0|-1.0|"`-lofreq value`", or -1 to disable
`HIFREQ`|High frequency cut-off in fbank analysis|-1.0|-1.0|"`-hifreq value`", or -1 to disable
`RAWENERGY`|Use raw energy|T|F|"`-rawe`" / "`-norawe`"
`ENORMALISE`|Normalise log energy|T|F|"`-enormal`" / "`-noenormal`" (**)
`ESCALE`|Scale log energy|0.1|1.0|"`-escale value`"
`SILFLOOR`|Energy silence floor in Dbs|50.0|50.0|"`-silfloor value`"

(*) samples = HTK value (in 100ns units) / smpPeriod
(**) Normalise log energy should not be specified on live input, at both training and recognition (see sec. 5.9 "Direct Audio Input/Output" in HTKBook).
****

### Normalization / Subtraction

Waveform: sub zeromean, spectral subtraction.
Feature: CMN, CVN, VTLN
Notes on real-time recognition (cmn/cvn)

## Feature input

HTK Parameter file.
adintool taiou

## Input rejection

GMM

## Acoustic Model

GMM-HMM / DNN-HMM

Duration model is not supported

Inverse diagonal covariance is only supported.

## Language Model

### N-gram

### Grammar

### Word list

## Search Algorithm

## Module mode

## Embedding
