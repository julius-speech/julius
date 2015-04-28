Plugin samples
================

From rev.4.1, plugin is supported.  This directory contains exmaple
sample source codes of plugins.

Function specifications of plugin are fully documented within the
source.  See the instruction below.


Files
======

    00readme.txt		This file
    plugin_defs.h		Plugin related typedefs for C.
    adin_oss.c			A/D-in plugin example: OSS mic input
    audio_postprocess.c		A/D-in postprocess plugin
    fvin.c			Feature vector input plugin
    feature_postprocess.c	Feature vector postprocess plugin
    calcmix.c			AM Mixture calculation plugin
    Makefile			Makefile for Linux / mingw


How to compile
===============

The source should be compiled into a dynamic shared object.
The object file should have a suffix of ".jpi".

On Linux and cygwin, you can compile with gcc like this:

    % gcc -shared -o result.jpi result.c

If you compile on cygwin and want it to run without cygwin, you can do

    % gcc -shared -mno-cygwin -o result.jpi result.c

On Mac OS X:

    % gcc -bundle -flat_namespace -undefined suppress -o result.jpi result.c


How to use
===========

Add option "-plugindir dirname" to Julius.  The "dirname" should be
a directory (or colon-separated list of directories).  All the .jpi
files in the specified directory will be loaded into Julius at startup.


How to test
============

You can test the OSS API audio input plugin written at "adin_oss.c".
The loaded plugin component "adin_oss.jpi" will be selected as input
by specifying "-input myadin", where the string "myadin" is the
string which the function "adin_get_optname()" returns in adin_oss.c.

	% cd plugin
	% make adin_oss.jpi
	% cd ..
	% ./julius/julius -plugindir plugin -input myadin

This adin plugin can be used from adintool and adinrec like this:

	% ./adinrec/adinrec -plugindir plugin -input myadin


