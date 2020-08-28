======================================================================
                  Large Vocabulary Continuous Speech
                          Recognition Engine

                                Julius

                                                (Rev 4.6   2020/09/02)
                                                (Rev 4.5   2019/01/02)
                                                (Rev 4.4.2 2016/09/12)
                                                (Rev 4.4   2016/08/30)
                                                (Rev 4.3.1 2014/01/15)
                                                (Rev 4.3   2013/12/25)
                                                (Rev 4.2.3 2013/06/30)
                                                (Rev 4.2.2 2012/08/01)
                                                (Rev 4.2.1 2011/12/25)
                                                (Rev 4.2   2011/05/01)
                                                (Rev 4.1.5 2010/06/04)
                                                (Rev 4.1   2008/10/03)
                                                (Rev 4.0.2 2008/05/27)
                                                (Rev 4.0   2007/12/19)
                                                (Rev 3.5.3 2006/12/29)
                                                (Rev 3.4.2 2004/04/30)
                                                (Rev 2.0   1999/02/20)
                                                (Rev 1.0   1998/02/20)

 Copyright (c) 1991-2020 Kawahara Lab., Kyoto University
 Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan
 Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 Copyright (c) 2005-2020 Julius project team, Nagoya Institute of Technology
 All rights reserved
======================================================================

About Julius
=============

"Julius" is an open-source high-performance large vocabulary
continuous speech recognition (LVCSR) decoder software for
speech-related researchers and developers.  Based on word N-gram and
triphone context-dependent HMM, it can perform almost real-time
decoding on most current PCs with small amount of memory.

It also has high versatility. The acoustic models and language models
are plug-gable, and you can build various types of speech recognition
system by building your own models and modules to be suitable for your
task.  It also adopts standard formats to cope with other toolkit such
as HTK, CMU-Cam SLM toolkit, etc.

The core engine is implemented as embeddable library, to aim to offer
speech recognition capability to various applications.  The recent
version supports plug-in capability so that the engine can be extended
by user.

The main platform is Linux and other Unix workstations, and also works
on Windows, MacOS, iOS, Android and other OS. Julius is distributed
with open license together with source codes.


What's new in Julius-4.6
==========================

Julius-4.6 is a minor release with a few new features and many fixes.

- New CUDA support at DNN computation (tested on Linux / CUDA-8,9,10)
- New 1-pass grammar recognition
- Support non-log10nized state priors in DNN model
- Feature normalization pattern added: mean = input self, variance = static
- Now can build almost all tools with Visual Studio 2017 (msvc/Julius.sln)
- Now delivered under simplified BSD License

See Release.txt for full changes and usage example.


Contents of Julius-4.6
=======================

        (Documents with suffix "ja" are written in Japanese)

        00readme.txt		ReadMe (This file)
        LICENSE.txt		Terms and conditions of use
        Release.txt		Release note / ChangeLog
        00readme-DNN.txt	DNN-HMM related issues
        README.md		description about Julius for GitHub
        configure		configure script
        configure.in
        Sample.jconf		Sample configuration file
        Sample.dnnconf		Sample DNN configuration file
        julius/			Julius sources
        libjulius/		JuliusLib core engine library sources
        libsent/		JuliusLib low-level library sources
        adinrec/		Record one sentence utterance to a file
        adintool/		Record/split/send/receive speech data (GUI)
        generate-ngram/		Tool to generate random sentences from N-gram
        gramtools/		Tools to build and test recognition grammar
        jcontrol/		A sample network client module
        mkbingram/		Convert N-gram to binary format
        mkbinhmm/		Convert ascii hmmdefs to binary format
        mkgshmm/		Model conversion for Gaussian Mixture Selection
        mkss/			Estimate noise spectrum from mic input
        support/		some tools to compile from source
        jclient-perl/		A simple perl version of module mode client
        plugin/			Several plugin source codes and documentation
        man/			Unix online manuals
        msvc/			Files to compile on Microsoft VC++ 2013
        dnntools/		Sample programs for dnn and vecnet client
        binlm2arpa/		Convert binary N-gram to ARPA format


License and Citation
=====================

This code is made available under the modified BSD License (BSD-3-Clause License).

Over and above the legal restrictions imposed by this license, when you publish
or present results by using this software, we would highly appreciate if you
mention the use of "Large Vocabulary Continuous Speech Recognition Engine Julius"
and provide proper reference or citation so that readers can easily access
the information of the software. This would help boost the visibility
of Julius and then further enhance Julius and the related software.

Citation to this software can be a paper that describes it,

  A. Lee, T. Kawahara and K. Shikano. "Julius --- An Open Source Real-Time Large
  Vocabulary Recognition Engine".  In Proc. EUROSPEECH, pp.1691--1694, 2001.

  A. Lee and T. Kawahara. "Recent Development of Open-Source Speech Recognition
  Engine Julius" Asia-Pacific Signal and Information Processing Association Annual
  Summit and Conference (APSIPA ASC), 2009.

or a direct citation to this software,

  A. Lee and T. Kawahara: Julius v4.5 (2019) https://doi.org/10.5281/zenodo.2530395

or both.


Contact Us
===========

Julius is now hosted on GitHub:

        https://github.com/julius-speech/julius

You can still find older documents and files in previous web page:

        http://julius.osdn.jp/
        https://osdn.jp/projects/julius/

