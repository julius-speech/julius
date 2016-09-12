(Moved from julius.osdn.jp since 2015/09, this is official)  
(Forum has been closed.  Please [make an issues](https://github.com/julius-speech/julius/issues) for questions and discussions about Julius)

# Julius: Open-Source Large Vocabulary Continuous Speech Recognition Engine

Copyright (c) 1991-2016 [Kawahara Lab., Kyoto University](http://www.ar.media.kyoto-u.ac.jp/)  
Copyright (c) 2005-2016 [Julius project team, Lee Lab., Nagoya Institute of Technology](http://www.slp.nitech.ac.jp/)  
Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan  
Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology  

# About Julius

"Julius" is a high-performance, small-footprint large vocabulary continuous speech recognition (LVCSR) decoder software for speech-related researchers and developers. Based on word N-gram and context-dependent HMM, it can perform real-time decoding on various computers and devices from micro-computer to cloud server. The algorithm is based on 2-pass tree-trellis search, which fully incorporates major decoding techniques such as tree-organized lexicon, 1-best / word-pair context approximation, rank/score pruning, N-gram factoring, cross-word context dependency handling, enveloped beam search, Gaussian pruning, Gaussian selection, etc. Besides search efficiency, it is also modularized to be independent from model structures, and wide variety of HMM structures are supported such as shared-state triphones and tied-mixture models, with any number of mixtures, states, or phone sets. It also can run multi-instance recognition, running dictation, grammar-based recognition or isolated word recognition simultaneously in a single thread.  Standard formats are adopted for the models to cope with other speech / language modeling toolkit such as HTK, SRILM, etc.  Recent version also supports Deep Neural Network (DNN) based real-time decoding.

The main platform is Linux and other Unix-based system, as well as Windows, Mac, Androids and other platforms.

Julius has been developed as a research software for Japanese LVCSR since 1997, and the work was continued under IPA Japanese dictation toolkit project (1997-2000), Continuous Speech Recognition Consortium, Japan (CSRC) (2000-2003) and Interactive Speech Technology Consortium (ISTC).

The main developer / maintainer is Akinobu Lee (ri@nitech.ac.jp).


# Features

- An open-source LVCSR software (see [terms and conditions of license](https://github.com/julius-speech/julius/blob/master/LICENSE.txt).)
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

# Download Julius

The latest release version is [4.4.2](https://github.com/julius-speech/julius/releases), released on September 12, 2016.
You can get the released package from the [Release page](https://github.com/julius-speech/julius/releases).

Version 4.4 supports stand-alone DNN-HMM support, and several new
tools and bug fixes are included.  See the "Release.txt" file for the
full list of updates.  Run with "-help" to see full list of options.

# Toolkit and Assets

There are also toolkit and assets to run Julius.  They are maintained by the Julius development team.  You can get them fron the following Github pages:

## [Japanese Dictation Kit](https://github.com/julius-speech/dictation-kit)

A set of Julius executables and Japanese LM/AM.  You can test 60k-word Japanese dictation with this kit.  For AM, triphone HMMs of both GMM and DNN are included.  For DNN, a front-end DNN module, separated from Julius, computes the state probabilities of HMM for each input frame and send them to Julius via socket to perform real-time DNN decoding.  For LM, 60k-word 3-gram trained by BCCWJ corpus is included.  You can get it from [its GitHub page](https://github.com/julius-speech/dictation-kit).

## [Recognition Grammar Toolkit](https://github.com/julius-speech/grammar-kit)

Documents, sample files and conversion tools to use and build a recognition grammar for Julius.  You can get it from [the GitHub page](https://github.com/julius-speech/grammar-kit).

## [Speech Segmentation Toolkit](https://github.com/julius-speech/segmentation-kit)

This is a handy toolkit to do phoneme segmentation (aka phoneme alignments) for speech audio file using Julius. Given pairs of speech audio file and its transcription, this toolkit perform Viterbi alignment to get the beginning and ending time of each phoneme.  This toolkit is available at [its GitHub page](https://github.com/julius-speech/segmentation-kit).

# About Models

Since Julius itself is a language-independent decoding program, you can make a recognizer of a language if given an appropriate language model and acoustic model for the target language. The recognition accuracy largely depends on the models. Julius adopts acoustic models in HTK ascii format, pronunciation dictionary in almost HTK format, and word 3-gram language models in ARPA standard format (forward 2-gram and reverse N-gram trained from same corpus).

We had already examined English dictations with Julius, and another researcher has reported that Julius has also worked well in English, Slovenian (see pp.681--684 of Proc. ICSLP2002), French, Thai language, and many other Languages.

Here you can get Japanese and English language/acoustic models.

## Japanese

Japanese language model (60k-word trained by balanced corpus) and acoustic models (triphone GMM/DNN) are included in the [Japanese dictation kit](https://github.com/julius-speech/dictation-kit).  More various types of Japanese N-gram LM and acoustic models are available at CSRC. For more detail, please contact csrc@astem.or.jp.

## English

We currently only have a sample English acoustic model trained from the WSJ database. According to the license of the database, this model *cannot* be used to develop or test products for commercialization, nor can they use it in any commercial product or for any commercial purpose. Also, the performance is not so good. Please contact to us for further information.

The VoxForge-project is working on the creation of an open-source acoustic model for the English language.
If you have any language or acoustic model that can be distributed as a freeware, would you please contact us? We want to run dictation kit on various languages other than Japanese, and share them freely to provide a free speech recognition system available for various languages.

# Documents

- [Release Notes](https://github.com/julius-speech/julius/blob/master/Release.txt)
- [The Juliusbook 3 (English) - fully translated from Japanese for 3.x](http://julius.sourceforge.jp/book/Julius-3.2-book-e.pdf)
- [The Juliusbook 4 (English) - commands and options for 4.x](http://sourceforge.jp/projects/julius/downloads/47534/Juliusbook-4.1.5.pdf)
- [The Juliusbook 4 (Japanese) - full documentation in Japanese](http://julius.osdn.jp/juliusbook/ja/)
- [The grammar format of Julius](http://julius.sourceforge.jp/en_index.php?q=en_grammar.html)
- How to run with DNN-HMM (preparing)

# References

- [Official web site (Japanese)](http://julius.osdn.jp/)
- [Old development site, having old releases](http://sourceforge.jp/projects/julius/)
- Publications:
  - A. Lee and T. Kawahara. "Recent Development of Open-Source Speech Recognition Engine Julius" Asia-Pacific Signal and Information Processing Association Annual Summit and Conference (APSIPA ASC), 2009.
  - A. Lee, T. Kawahara and K. Shikano. "Julius --- an open source real-time large vocabulary recognition engine." In Proc. European Conference on Speech Communication and Technology (EUROSPEECH), pp. 1691--1694, 2001.
  - T. Kawahara, A. Lee, T. Kobayashi, K. Takeda, N. Minematsu, S. Sagayama, K. Itou, A. Ito, M. Yamamoto, A. Yamada, T. Utsuro and K. Shikano. "Free software toolkit for Japanese large vocabulary continuous speech recognition." In Proc. Int'l Conf. on Spoken Language Processing (ICSLP) , Vol. 4, pp. 476--479, 2000.
