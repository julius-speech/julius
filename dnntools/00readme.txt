This directory contains a sample script and tools

1. sendvec.c
==============

This is a sample stand-alone program to send input vectors to Julius
via TCP/IP network.  You can compile like this:

    % cc -o sendvec sendvec.c 

Usage: "paramfile" is a HTK parameter file to be sent.

    % ./sendvec paramfile hostname [portnum]

Define OUTPROBVECTOR at the top of the code to send the parameter
vector as outprob vectors.  Remove the definition if you want to send
vectors as feature vector.  Please note that the parameter type and
vector length will not be checked at the server side so you should
send the same type and size of the vector as assumed in the server side.


2. embed_sid.pl
=================

This perl script embeds "<SID>" tags to HMM definition file, to make
correspondence between frontend outprob calculator and recognizer
(Julius) for DNN-HMM recognition.

  Usage:

    % ./embed_sid.pl < source_hmmdefs > embedded_hmmdefs


The script will embed "<SID> value" tags to all states, increasing the
"value" from 0, according to the order of appearance in the source
hmmdefs.

The input format should be in HTK ASCII format, and the output is the
same, with the extra "<SID>" tag embedded.
