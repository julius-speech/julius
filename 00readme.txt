======================================================================
                  Large Vocabulary Continuous Speech
                          Recognition Engine

                                Julius

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

 Copyright (c) 1991-2019 Kawahara Lab., Kyoto University
 Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan
 Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 Copyright (c) 2005-2019 Julius project team, Nagoya Institute of Technology
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


What's new in Julius-4.5
==========================

A WebRTC-based voice activity detection (VAD) was newly added into
Julius for robust voice detection.

  https://github.com/dpirch/libfvad

Now Julius has two VAD modules: old module (input level and zero-cross
based) and new module (libfvad = model based), and both of them runs
concurrently in parallel for an audio input.  When enabled, Julius
will detect speech trigger only if *both of them are signaled as
speech*.

DNN-HMM computation now supports CPU multi-threading.  Multi-threading
may speed up DNN-HMM computation to some extent.  The number of
threads can be specified by "num_threads" in dnnconf.  This feature is
enabled by default, and the default number of threads is 2.

Notice for module-mode users: in order to make module output
XML-compiliant, the XML special characters are now escaped their
escaped form, ex. '>' to '&gt;'.  A backward option "-noxmlescape" is
still available to inhibit escaping and output as the same as previous
versions.

See Release.txt for full changes and usage example.


Moving to UTF-8
================

We are going to move to UTF-8.

The master branch after the release of 4.5 (2019/1/2) has codes
converted to UTF-8.  All files were converted to UTF-8, and future
update will be commited also in UTF-8.

For backward compatibility and log visibility, we are keeping the old
encoding codes at branch "master-4.5-legacy".  The branch keeps legacy
encoding version of version 4.5.  If you want to inspect the code
progress before the release of 4.5 (2019/1/2), please checkout the
branch.


Contents of Julius-4.5
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


License
========

Julius is an open-source software provided as is.  For more
information about the license, please refer to the "LICENSE.txt" file
included in this archive.

Also see the copyrights in the files:

  libjulius/libfvad/
  gramtools/gram2sapixml/gram2sapixml.pl.in
  libsent/src/wav2mfcc/wav2mfcc-*.c
  libsent/src/adin/pa/
  msvc/portaudio/
  msvc/zlib/


Contact Us
===========

Julius is now hosted on GitHub:

        https://github.com/julius-speech/julius

You can still find older documents and files in previous web page:

        http://julius.osdn.jp/
	https://osdn.jp/projects/julius/

