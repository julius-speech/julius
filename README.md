# Julius: Open-Source Large Vocabulary Continuous Speech Recognition Engine

Copyright (c) 1991-2015 Kawahara Lab., Kyoto University  
Copyright (c) 2005-2015 Julius project team, Nagoya Institute of Technology  
Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan  
Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology  

# About Julius

"Julius" is a high-performance, small-footprint large vocabulary continuous speech recognition (LVCSR) decoder software for speech-related researchers and developers. Based on word N-gram and context-dependent HMM, it can perform real-time decoding on various computers and devices from micro-computer to cloud server. The algorithm is based on 2-pass tree-trellis search, which fully incorporates major decoding techniques such as tree-organized lexicon, 1-best / word-pair context approximation, rank/score pruning, N-gram factoring, cross-word context dependency handling, enveloped beam search, Gaussian pruning, Gaussian selection, etc. Besides search efficiency, it is also modularized to be independent from model structures, and wide variety of HMM structures are supported such as shared-state triphones and tied-mixture models, with any number of mixtures, states, or phone sets. Standard formats are adopted for the models to cope with other speech / language modeling toolkit such as HTK, SRILM, etc.  Recent version also supports Deep Neural Network (DNN) based real-time decoding.

The main platform is Linux and other Unix-based system, as well as Windows, Mac, Androids and other platforms.

Julius is distributed with an open license together with source codes.

Julius has been developed as a research software for Japanese LVCSR since 1997, and the work was continued under IPA Japanese dictation toolkit project (1997-2000), Continuous Speech Recognition Consortium, Japan (CSRC) (2000-2003) and Interactive Speech Technology Consortium (ISTC). The main developer / maintainer is Akinobu Lee.


# Features

- An open-source LVCSR software (see terms and conditions of license).
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

# Latest version: 4.3.1

The latest version is 4.3.1, released on January 15, 2014.

Version 4.3.1 is a bug fix release. Several bugs has been fixed.

See the "Release.txt" file for the full list of updates.
Run with "-help" to see full list of options.

# Related Tools

## Julius for SAPI

Julius for SAPI is MS Windows version of Julius/Julian which implements Microsoft(R) Speech API (SAPI) 5.1. You can use this version of Julius as a SAPI Voice Recognizer in applications created for SAPI (e.g. Office XP).
The recent version is fully SAPI-5.1 compliant, and it also supports SALT extension.  Julius for SAPI assumes that the user language and the application's grammar is in Japanese. So it is a little troublesome in case of the other languages because Julius for SAPI does not know the pronunciation of the words in a grammar. If you define pronunciations to each of these, it may work, but we have not tried it.

Documents:
- Julius for SAPI README (Japanese)
- Julius for SAPI Documents for Developers (Japanese)

Download (last updated: 2004/02/05 for ver. 2.3)
- Julius for Windows SAPI ver. 2.3 (installer)
- Japanese standard language model and acoustic model installer
- Sample programs:
  - JavaScript
  - Win32 Application (C++, Microsoft Visual C++ 7.0)
  - Win32 Application (C++, OpenGL, Robot manipulation, executable binaries and part of sources).
  - SALT (for Microsoft .NET Speech SDK 1.0 beta2)
  - SALT (for Microsoft .NET Speech SDK 1.0 beta3)

## Word / phoneme segmentation kit

This toolkit helps performing "forced alignment" with speech recognition engine Julius with grammar-based recognition. This kit uses Julius to do forced alignment to a speech file by generating grammar for each samples from transcription.
julius4-segmentation-kit-v1.0.tar.gz

## HTK-to-Julius grammar converter

This toolkit converts an HTK recognition grammar into Julian format. A word network (SLF) will be converted to DFA format, and the words in the SLF are extracted from the dictionary to be used in Julian. Furthermore, word category will be automatically detected and defined to optimize performance in Julian.
slg2dfa-1.0.tar.gz

# About Models

Since Julius itself is a language-independent decoding program, you can make a recognizer of a language if given an appropriate language model and acoustic model for the target language. The recognition accuracy largely depends on the models.
Julius adopts acoustic models in HTK ascii format, pronunciation dictionary in almost HTK format, and word 3-gram language models in ARPA standard format (forward 2-gram and reverse 3-gram trained from same corpus).

We had already examined English dictations with Julius, and another researcher has reported that Julius has also worked well in English, Slovenian (see pp.681--684 of Proc. ICSLP2002), French, Thai language, and many other Languages.

Here you can get Japanese and English free language/acoustic models.

Japanese
Japanese language model (20k-word trained by newspaper article) and acoustic models (Phonetic tied-mixture triphone / monophone)
More various types of Japanese N-gram LM and acoustic models are available at CSRC. For more detail, please contact csrc@astem.or.jp.

English
We currently have a sample English acoustic model trained from the WSJ database. According to the license of the database, this model *cannot* be used to develop or test products for commercialization, nor can they use it in any commercial product or for any commercial purpose. Also, the performance is not so good. Please contact to us for further information.
The VoxForge-project is working on the creation of an open-source acoustic model for the English language.
If you have any language or acoustic model that can be distributed as a freeware, would you please contact us? We want to run dictation kit on various languages other than Japanese, and share them freely to provide a free speech recognition system available for various languages.

# Documentation

We are also making a complete documentation of Julius, fully updated for the current version. The document is called "Juliusbook", and its initial release has been done in Japanese. We are now making English version.
The Juliusbook (command manuals and option descriptions only)
The Juliusbook (Online Documentation)
New features in Julius rev.4.0
JuliusLib API Reference
JuliusLib application callbacks
Julius book for rev.3.2: an old document but has many informations.
full source code browser generated by Doxygen.
The recognition grammar format of Julius

The format of recognition grammar for Julius is briefly described here.

# References

Development site (older versions here)
All documents (most up-to-date but in Japanese)
Papers: (each link refers to its PDF reprints)
A. Lee and T. Kawahara. "Recent Development of Open-Source Speech Recognition Engine Julius" Asia-Pacific Signal and Information Processing Association Annual Summit and Conference (APSIPA ASC), 2009.
A. Lee, T. Kawahara and K. Shikano. "Julius --- an open source real-time large vocabulary recognition engine." In Proc. European Conference on Speech Communication and Technology (EUROSPEECH), pp. 1691--1694, 2001.
T. Kawahara, A. Lee, T. Kobayashi, K. Takeda, N. Minematsu, S. Sagayama, K. Itou, A. Ito, M. Yamamoto, A. Yamada, T. Utsuro and K. Shikano. "Free software toolkit for Japanese large vocabulary continuous speech recognition." In Proc. Int'l Conf. on Spoken Language Processing (ICSLP) , Vol. 4, pp. 476--479, 2000.
