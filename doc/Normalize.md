# Normalization / Subtraction

Audio normalization and noise reduction is a vital part for robust speech
recognition in various environment.  Unfortunately Julius has no modern
normalization or noise reduction techniques in it, just offers some basic methods.

Note that *the recognition results may not be the same* between live audio input
and file input, even for equivalent audio data.  It is due to the approximation
of cepstral mean normalization at live audio input as described below.

Here is a brief summary of currently implemented methods.

## DC offset removal

Julius has two different implementation of DC offset removal.

- Input-wise  ([-zmean](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-zmean--nozmean))
- Frame-wise  ([-zmeanframe](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-zmeanframe--nozmeanframe))

[-zmean](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-zmean--nozmean)
enables input-wise offset removal.  For buffered processing
(i.e. file input), the offset is estimated for each input by computing the mean
of the whole input. For stream proccessing (live audio input and network input), the
offset will be determined from the mean of the first 48,000 samples (3 seconds
in 16kHz sampling), and removal will be performed for the rest samples.  This
process is performed prior to all audio processing.

[-zmeanframe](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-zmeanframe--nozmeanframe)
enables frame-wise offset removal.  The offset is estimated per windowed samples
and subtracted within the frame.  This behavior is equivalent to HTK's
`ZMEANSOURCE`, and performed at the first stage of feature extraction.

In most cases `-zmeanframe` is sufficient. This works the same in both buffered
processing and stream processing.

## Spectral subtraction

A simple spectral subtraction can be used to reduce effect of stationary noise.
No dynamic update of the noise spectrum is implemented, i.e., the noise spectrum should be
estimated statically before speech process.  There are two ways.

- Compute the average spectrum of first 300 msec of every input and subtract it
  from the rest input as noise spectrum
  ([-sscalc](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-sscalc)).
  Available for buffered processing only.
- Compute average spectrum from noise-only audio beforehand by the tool
  [mkss](https://github.com/julius-speech/julius/tree/master/mkss), and apply it
  with
  [-ssload](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-ssload-file).
  Available for both buffered and stream input.

Tweaking options of spectral subtractions are
[-sscalclen](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-sscalclen-msec),
[-ssalpha](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-ssalpha-float)
and
[-ssfloor](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-ssfloor-float).

Note that spectral subtraction is performed inside AM instance: those options are
categorized as `AM`. When using multiple acoustic model, the spectral
subtraction should be set up at each module.

## Cepstral mean / variance normalization

Cepstral mean normalization (CMN) and variance normalization (CVN) are standard
techniques to compensate long-term spectral effects caused by different
microphones and audio channels.  CMN subtracts the mean vector from each input
frame.  Additionally, CVN will normalize the variance.  When CMN and CVN are
enabled, features are normalized to $N(0, 1)$.
Note that mean normalization will be performed for static coefficients, whereas
variance normalization will be for all vector dimensions.

 The CMN will be enabled automatically when using acoustic model trained with
 CMN, i.e., the feature parameter of acoustic model has CMN qualifier "`_Z`".
 On the other hand, variance normalization should be enabled manually by option
 [-cvn](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-cvn).
 Of course when using CMN/CVN you should have acoustic model trained with CMN/CVN.

### The algorithm

For buffered processing like file input, normalization will be done **per
input**: all samples of an input file are read into memory first, and then their
mean and variance of the whole input are computed.  Then normalization will be
performed using the values.

For stream processing like live audio input, on the other hand, the true mean
and variance are unknown at the time of processing each input frame.  Thus a
modified version of CMN called "**MAP-CMN**" is applied at stream processing. It
starts with a pre-set "generic mean/variance", and it updates the current mean
at every frame by smearing the generic value with the mean of recently processed
frames.  The algorithm  is as follows:

<!--
- Load generic mean and variance ($\bm{\mu_g}$ and $\bm{\sigma_g^2}$).
- At each speech input,
  - Set initial values:  $\bm{\hat{\mu}} = \bm{\mu_g}$, $\bm{\hat{\sigma^2}} = \bm{\sigma_g^2}$.
  - For every frame $t$,
    - normalize the current frame with $\bm{\hat{\mu}}$ and $\bm{\hat{\sigma^2}}$.
    - update mean $\bm{\hat{\mu}}$ with a weight $w$:

$$
\bm{\hat{\mu}} = \frac{w \cdot \mu_g +  \sum_{i=1}^t\bm{O}(i)}{w + t}
$$
-->

![MAP-CMN algorithm](image/map-cmn.png)

The weight $w$ can be changed by option
[-cmnmapweight](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-cmnmapweight-float).
Note that only mean is updated at each frame, and variance will be kept within
an input.

### Update / load / save generic mean and variance

By default, after each input has been processed and recognition result was
obtained, the generic mean/variance values will be re-computed to fit the
current environment by calculating the mean/variance of the recent five seconds
of the input. The re-computed generic mean/variance will be then applied as the
initial values for the next input.  Option
[-cmnnoupdate](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-cmnupdate--cmnnoupdate)
disable this generic mean/variance updates.

The initial generic mean and variance can be loaded from file by option
[-cmnload](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-cmnload-file),
or can be saved to file by option
[-cmnsave](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-cmnsave-file).
The file format is the same as HTK's mean and variance estimation file that can
be generated by `CMEANDIR` and `VARSCALEDIR`, like this:

```text
<CEPSNORM> <>
<MEAN> 13
  (value 1)
  ...
  (value 13)
<VARIANCE> 39
  (value 1)
  ...
  (value 39)
```

When `-cmnload` is not set, Julius start with zero mean and variance, so the
first utterance will not be recognized correctly.  When `-cmnsave` is specified,
the generic mean and variance, being computed from the last 5 seconds, will be
saved to file at every input ends.

### Using static mean/variance

You may want to avoid MAP-CMN and apply a set of mean and variance unchanged for
all frames without updating anything. Setting Option
[-cmnstatic](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-cmnstatic)
can disable MAP-CMN totally even on a live input, and applying the initially
given mean and variance for all frames.  In this case the initial mean and
variance should be given with
[-cmnload](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-cmnload-file).

## Frequency warping

Option
[-vtln](https://github.com/julius-speech/julius/blob/master/julius/Options.md#-vtln-alpha-lowcut-hicut)
enables a frequency warping function, so called vocal tract length normalization
(VTLN). This can be applied as a simple speaker normalization that compensates
speaker's vocal tract size.

It is a simple linear function, controlled by a warping factor $\alpha$ and
warping frequency boundaries.  The algorithm and parameter values are the same
as HTK's VTLN feature. HTK's configuration variables `WARPFREQ`, `WARPHCUTOFF`
and `WARPLCUTOFF` can be applied as is.

Unfortunately Julius has no searching scheme to find a working warping factor
for given input.
