Julius: Open-Source Large Vocabulary Continuous Speech Recognition Engine
==========================================================================
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.2530396.svg)](https://doi.org/10.5281/zenodo.2530396)

Copyright (c) 1991-2020 [Kawahara Lab., Kyoto University](http://sap.ist.i.kyoto-u.ac.jp/)  
Copyright (c) 2005-2020 [Julius project team, Lee Lab., Nagoya Institute of Technology](http://www.slp.nitech.ac.jp/)  
Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan  
Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology  

# About Julius

"Julius" is a high-performance, small-footprint large vocabulary continuous speech recognition (LVCSR) decoder software for speech-related researchers and developers. Based on word N-gram and context-dependent HMM, it can perform real-time decoding on various computers and devices from micro-computer to cloud server. The algorithm is based on 2-pass tree-trellis search, which fully incorporates major decoding techniques such as tree-organized lexicon, 1-best / word-pair context approximation, rank/score pruning, N-gram factoring, cross-word context dependency handling, enveloped beam search, Gaussian pruning, Gaussian selection, etc. Besides search efficiency, it is also modularized to be independent from model structures, and wide variety of HMM structures are supported such as shared-state triphones and tied-mixture models, with any number of mixtures, states, or phone sets. It also can run multi-instance recognition, running dictation, grammar-based recognition or isolated word recognition simultaneously in a single thread.  Standard formats are adopted for the models to cope with other speech / language modeling toolkit such as HTK, SRILM, etc.  Recent version also supports Deep Neural Network (DNN) based real-time decoding.

The main platform is Linux and other Unix-based system, as well as Windows, Mac, Androids and other platforms.

Julius has been developed as a research software for Japanese LVCSR since 1997, and the work was continued under IPA Japanese dictation toolkit project (1997-2000), Continuous Speech Recognition Consortium, Japan (CSRC) (2000-2003) and Interactive Speech Technology Consortium (ISTC).

The main developer / maintainer is Akinobu Lee (ri@nitech.ac.jp).

# Features

- An open-source LVCSR software (BSD 3-clause license).
- Real-time, hi-speed, accurate recognition based on 2-pass strategy.
- Low memory requirement: less than 32MBytes required for work area (<64MBytes for 20k-word dictation with on-memory 3-gram LM).
- Supports LM of N-gram with arbitrary N.  Also supports rule-based grammar, and word list for isolated word recognition.
- Language and unit-dependent: Any LM in ARPA standard format and AM in HTK ascii hmm definition format can be used.
- Highly configurable: can set various search parameters. Also alternate decoding algorithm (1-best/word-pair approx., word trellis/word graph intermediates, etc.) can be chosen.
- List of major supported features:
  - On-the-fly recognition for microphone and network input
  - GMM-based input rejection
  - Successive decoding, delimiting input by short pauses
  - N-best output
  - Word graph output
  - Forced alignment on word, phoneme, and state level
  - Confidence scoring
  - Server mode and control API
  - Many search parameters for tuning its performance
  - Character code conversion for result output.
  - (Rev. 4) Engine becomes Library and offers simple API
  - (Rev. 4) Long N-gram support
  - (Rev. 4) Run with forward / backward N-gram only
  - (Rev. 4) Confusion network output
  - (Rev. 4) Arbitrary multi-model decoding in a single thread.
  - (Rev. 4) Rapid isolated word recognition
  - (Rev. 4) User-defined LM function embedding
- DNN-based decoding, using front-end module for frame-wise state probability calculation for flexibility.

# Quick Run

How to test English dictation with Julius and English DNN model.  The procedure is for Linux but almost the same for other OS.

(For Japanese dictation, Use [dictation kit](https://github.com/julius-speech/julius#japanese-dictation-kit))

## 1. Build latest Julius

```shell
% sudo apt-get install build-essential zlib1g-dev libsdl2-dev libasound2-dev
% git clone https://github.com/julius-speech/julius.git
% cd julius
% ./configure --enable-words-int
% make -j4
% ls -l julius/julius
-rwxr-xr-x 1 ri lab 746056 May 26 13:01 julius/julius
```

## 2. Get English DNN model

Go to [JuliusModel](https://sourceforge.net/projects/juliusmodels/files/) page and download the English model(LM+DNN-HMM) named "`ENVR-v5.4.Dnn.Bin.zip`".  Unzip it and cd to there.

```shell
% cd ..
% unzip /some/where/ENVR-v5.4.Dnn.Bin.zip
% cd ENVR-v5.4.Dnn.Bin
```

## 3. Modify config file

Edit the `dnn.jconf` file in the unzipped folder to fit the latest version of Julius:

```text
(edit dnn.jconf)
@@ -1,5 +1,5 @@
 feature_type MFCC_E_D_A_Z
-feature_options -htkconf wav_config -cvn -cmnload ENVR-v5.3.norm -cmnstatic
+feature_options -htkconf wav_config -cvn -cmnload ENVR-v5.3.norm -cvnstatic
 num_threads 1
 feature_len 48
 context_len 11
@@ -21,3 +21,4 @@
 output_B ENVR-v5.3.layerout_bias.npy
 state_prior_factor 1.0
 state_prior ENVR-v5.3.prior
+state_prior_log10nize false
```

## 4. Recognize audio file

Recognize "`mozilla.wav`" included in the zip file.

```shell
% ../julius/julius/julius -C julius.jconf -dnnconf dnn.jconf
```

You'll get tons of messages, but the final result of the first speech part will be output like this:

```
sentence1: <s> without the data said the article was useless </s>
wseq1: <s> without the data said the article was useless </s>
phseq1: sil | w ih dh aw t | dh ax | d ae t ah | s eh d | dh iy | aa r t ah k ah l | w ax z | y uw s l ah s | sil
cmscore1: 0.785 0.892 0.318 0.284 0.669 0.701 0.818 0.103 0.528 1.000
score1: 261.947144
```

"`test.dbl`" contains list of audio files to be recognized.  Edit the file and run again to test with another files.

## 5. Run with live microphone input

To run Julius on live microphone input, save the following text as "`mic.jconf`".

```text
-input mic
-htkconf wav_config
-h ENVR-v5.3.am
-hlist ENVR-v5.3.phn
-d ENVR-v5.3.lm
-v ENVR-v5.3.dct
-b 4000
-lmp 12 -6
-lmp2 12 -6
-fallback1pass
-multipath
-iwsp
-iwcd1 max
-spmodel sp
-no_ccd
-sepnum 150
-b2 360
-n 40
-s 2000
-m 8000
-lookuprange 5
-sb 80
-forcedict
```

and run Julius with the mic.jconf instead of julius.jconf

```shell
% ../julius/julius/julius -C mic.jconf -dnnconf dnn.jconf
```

# Download

The latest release version is [4.6](https://github.com/julius-speech/julius/releases), released on September 2, 2020.
You can get the released package from the [Release page](https://github.com/julius-speech/julius/releases).
See the "Release.txt" file for full list of updates.  Run with "-help" to see full list of options.

# Install / Build Julius

Follow the instructions in [INSTALL.txt](https://github.com/julius-speech/julius/blob/master/INSTALL.txt).

# Tools and Assets

There are also toolkit and assets to run Julius.  They are maintained by the Julius development team.  You can get them from the following Github pages:

## [Japanese Dictation Kit](https://github.com/julius-speech/dictation-kit)

A set of Julius executables and Japanese LM/AM.  You can test 60k-word Japanese dictation with this kit.  For AM, triphone HMMs of both GMM and DNN are included.  For DNN, a front-end DNN module, separated from Julius, computes the state probabilities of HMM for each input frame and send them to Julius via socket to perform real-time DNN decoding.  For LM, 60k-word 3-gram trained by BCCWJ corpus is included.  You can get it from [its GitHub page](https://github.com/julius-speech/dictation-kit).

## [Recognition Grammar Toolkit](https://github.com/julius-speech/grammar-kit)

Documents, sample files and conversion tools to use and build a recognition grammar for Julius.  You can get it from [the GitHub page](https://github.com/julius-speech/grammar-kit).

## [Speech Segmentation Toolkit](https://github.com/julius-speech/segmentation-kit)

This is a handy toolkit to do phoneme segmentation (aka phoneme alignments) for speech audio file using Julius. Given pairs of speech audio file and its transcription, this toolkit perform Viterbi alignment to get the beginning and ending time of each phoneme.  This toolkit is available at [its GitHub page](https://github.com/julius-speech/segmentation-kit).

## [Prompter](https://github.com/julius-speech/prompter)

Prompter is a perl/Tkx based tiny program that displays recognition results of Julius in a scrolling caption style.

# About Models

Since Julius itself is a language-independent decoding program, you can make a recognizer of a language if given an appropriate language model and acoustic model for the target language. The recognition accuracy largely depends on the models. Julius adopts acoustic models in HTK ascii format, pronunciation dictionary in almost HTK format, and word 3-gram language models in ARPA standard format (forward 2-gram and reverse N-gram trained from same corpus).

We had already examined English dictations with Julius, and another researcher has reported that Julius has also worked well in English, Slovenian (see pp.681--684 of Proc. ICSLP2002), French, Thai language, and many other Languages.

Here you can get Japanese and English language/acoustic models.

## Japanese

Japanese language model (60k-word trained by balanced corpus) and acoustic models (triphone GMM/DNN) are included in the [Japanese dictation kit](https://github.com/julius-speech/dictation-kit).  More various types of Japanese N-gram LM and acoustic models are available at CSRC. For more detail, please contact csrc@astem.or.jp.

## English

There are some user-contributed English models for Julius available on the Web.

[JuliusModels](https://sourceforge.net/projects/juliusmodels/) hosts English and Polish models for Julius.  All of the models are based on HTK modelling software and data sets available freely on the Internet.  They can be downloaded from a project website which I created for this purpose.  Please note that DNN version of these models require minor changes which the author included in a modified version of Julius on Github at https://github.com/palles77/julius .

The [VoxForge-project](http://www.voxforge.org/) is working on the creation of an open-source acoustic model for the English language.
If you have any language or acoustic model that can be distributed as a freeware, would you please contact us? We want to run dictation kit on various languages other than Japanese, and share them freely to provide a free speech recognition system available for various languages.

# Documents

Recent documents:

- Up-to-date document is now provided in markdown at [doc/](https://github.com/julius-speech/julius/blob/master/doc/).
  - Updating all documents to recent version, work in progress.
  - Finished Section:
    [Options](https://github.com/julius-speech/julius/blob/master/doc/Options.md),
    [Audio](https://github.com/julius-speech/julius/blob/master/doc/Audio.md),
    [Feature](https://github.com/julius-speech/julius/blob/master/doc/Feature.md),
    [Vector Input](https://github.com/julius-speech/julius/blob/master/doc/VectorInput.md),
    [VAD](https://github.com/julius-speech/julius/blob/master/doc/VAD.md),
    [Normalization](https://github.com/julius-speech/julius/blob/master/doc/Normalize.md),
    [Input Rejection](https://github.com/julius-speech/julius/blob/master/doc/Rejection.md).
- All options are fully described at [Options](https://github.com/julius-speech/julius/blob/master/doc/Options.md), also listed in sample configuration file [Sample.jconf](https://github.com/julius-speech/julius/blob/master/Sample.jconf), also be output when invoked with "julius --help".
- Full history and short descriptions are in [Release Notes](https://github.com/julius-speech/julius/blob/master/Release.txt) ([JP version](https://github.com/julius-speech/julius/blob/master/Release-ja.txt))
- For DNN-HMM, take a look at [00readme-DNN.txt](https://github.com/julius-speech/julius/blob/master/00readme-HNN.txt) for how-to and [Sample.dnnconf](https://github.com/julius-speech/julius/blob/master/Sample.dnnconf) as example.

Other, old documents:

- [The Juliusbook 3 (English) - translated from Japanese for 3.x](http://julius.sourceforge.jp/book/Julius-3.2-book-e.pdf)
- [The Juliusbook 4 (Japanese) - full documentation in Japanese](http://julius.osdn.jp/juliusbook/ja/)
- [The grammar format of Julius](http://julius.sourceforge.jp/en_index.php?q=en_grammar.html)

# References

- [Official web site (Japanese)](http://julius.osdn.jp/)
- [Old development site, having old releases](http://sourceforge.jp/projects/julius/)
- Publications:
  - A. Lee and T. Kawahara. "Recent Development of Open-Source Speech Recognition Engine Julius" Asia-Pacific Signal and Information Processing Association Annual Summit and Conference (APSIPA ASC), 2009.
  - A. Lee, T. Kawahara and K. Shikano. "Julius --- an open source real-time large vocabulary recognition engine." In Proc. European Conference on Speech Communication and Technology (EUROSPEECH), pp. 1691--1694, 2001.
  - T. Kawahara, A. Lee, T. Kobayashi, K. Takeda, N. Minematsu, S. Sagayama, K. Itou, A. Ito, M. Yamamoto, A. Yamada, T. Utsuro and K. Shikano. "Free software toolkit for Japanese large vocabulary continuous speech recognition." In Proc. Int'l Conf. on Spoken Language Processing (ICSLP) , Vol. 4, pp. 476--479, 2000.

# Moved to UTF-8

We are going to move to UTF-8.

The master branch after the release of 4.5 (2019/1/2) has codes
converted to UTF-8.  All files were converted to UTF-8, and future
update will be commited also in UTF-8.

For backward compatibility and log visibility, we are keeping the old
encoding codes at branch "master-4.5-legacy".  The branch keeps legacy
encoding version of version 4.5.  If you want to inspect the code
progress before the release of 4.5 (2019/1/2), please checkout the
branch.

# License and Citation

This code is made available under the modified BSD License (BSD-3-Clause License).

Over and above the legal restrictions imposed by this license, when you publish or present results by using this software, we would highly appreciate if you mention the use of "Large Vocabulary Continuous Speech Recognition Engine Julius" and provide proper reference or citation so that readers can easily access the information of the software. This would help boost the visibility of Julius and then further enhance Julius and the related software.

Citation to this software can be a paper that describes it,

> A. Lee, T. Kawahara and K. Shikano. "Julius --- An Open Source Real-Time Large Vocabulary Recognition Engine".  In Proc. EUROSPEECH, pp.1691--1694, 2001.

> A. Lee and T. Kawahara. "Recent Development of Open-Source Speech Recognition Engine Julius" Asia-Pacific Signal and Information Processing Association Annual Summit and Conference (APSIPA ASC), 2009.

or a direct citation to this software,

> A. Lee and T. Kawahara: Julius v4.5 (2019) https://doi.org/10.5281/zenodo.2530395

or both.
