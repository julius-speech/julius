# Feature Extraction

Julius extracts features from waveform input.  A feature extraction module is
built to have compatibility with HTK.  Supported base types are "FBANK"
(log-scale filterbank parameter), "MELSPEC" (mel-scale filterbank parameter) and
"MFCC" (mel-frequency cepstral coefficients).  Supported qualifiers are `_E`,
`_N`, `_D`, `_A`, `_Z` and `_0`.

## Configurations

The feature extraction parameters **should be set up as the same as training
condition of acoustic model**.  The following table shows all the feature
extraction parameters, with their default values and how to set.  Note that HTK
and Julius has **different default values** for some parameters, so you have to
set carefully to match the feature extraction condition.  Also note that the
"parameter kind" (`TARGETKIND` in HTK) and the "number of cepstral parameters"
(`NUMCEPS` in HTK) will be detected from the header information of acoustic HMM
at run time, so unlike HTK, you need not to set them manually.

|HTK Option|Description|HTK default|Julius default|Option|
|:--|:--|:--|:--|:--|
`TARGETKIND`|Parameter kind|ANON|MFCC|- (auto-set from HMM header)
`NUMCEPS`|Number of cepstral parameters|12|-|- (auto-set from HMM header)
`SOURCERATE`|Sample rate of source waveform in 100ns units|0.0|625|"`-smpPeriod value`"
`TARGETRATE`|Sample rate of target vector (= window shift) in 100ns units|0.0|160|"`-fshift samples`" (*)
`WINDOWSIZE`|Analysis window size in 100ns units|256000.0|400|"`-fsize samples`" (*)
`ZMEANSOURCE`|Zero mean source waveform before analysis (frame-wise)|F|F|"`-zmeanframe`" to enable, "`-nozmeanframe`" to disable.
`PREEMCOEF`|Set pre-emphasis coefficient|0.97|0.97|"`-preemph value`"
`USEHAMMING`|Use a Hamming window|T|T|- (Fixed)
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

> (*) samples = HTK value (in 100ns units) / smpPeriod
> (**) Normalise log energy should not be specified on live input, at both training and recognition (see sec. 5.9 "Direct Audio Input/Output" in HTKBook).

## Reading HTK Config file

Instead of using options directly, Julius can read in a HTK format config file
by option
[-htkconf](https://github.com/julius-speech/julius/blob/master/doc/Options.md#-htkconf-file).
When specified, the parameters in the given HTK conig file will be translated to
corresponding values while reading within Julius.  At the translation, values
not explicitly specified in the HTK config file will be assumed to be the HTK's
default value.

## Parameter embedding

Struggling to set the exact feature parameters for an acoustic model?  Since
feature extraction parameters are purely subject to acoustic model, it is
natural that an acoustic model should have full information for the feature
types and parameters it requires.  Thus the tool
[mkbinhmm](https://github.com/julius-speech/julius/tree/master/mkbinhmm), that
converts HTK definition file in ascii format to binary HMM, can embed feature
extraction parameters into the header of the output binary HMM file.

Specify feature parameters to `mkbinhmm` just as the same way as Julius, using
direct options or HTK Config file as described above,and it will embed the
determined parameters at the header of the output binary HMM.  When using the
binary HMM file in Julius, the parameter settings in the header of the file will
be read into Julius.

## When Value Conflicts

There may be some case when both HTK Config file and other direct options
specifies different values, or embedded values in the given HMM has different
values from the specified ones?  The rule is that "*explicit value supercedes
implicit values*".  Precisely, the parameter priority to solve the conflict is
as follows:

1. Direct option values
2. HTK Config values given by `-htkconf`
3. Embedded values inside binary HMM

The values of direct options always supercedes others, and HTK Config value
supercedes the embedded values.  Note that this rule will be applied
**regardless of the option order**.  This behavior is not common to other Julius
parameters in which the latter option always supercedes former.
