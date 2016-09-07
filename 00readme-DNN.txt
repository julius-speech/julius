
	Julius for DNN-based speech recognition

						(revised 2016/08/30)
						(updated 2013/09/29)

A. Julius and DNN-HMM
======================

From 4.4, Julius can perform DNN-HMM based recognition in two ways:

  1. standalone: directly compute DNN for HMM inside Julius (>= 4.4)

  2. network: receive state probabilities calculated by other process
     via socket (<= 4.3.1)

Both are described below.

 A.1. Standalone mode
 =====================

From version 4.4, Julius is capable of performing DNN-HMM based
recognition by itself.  It can read a DNN definition along with a HMM,
and can compute the network against input (spliced) feature vectors
and output the node scores of output layer for each frame, which will
be used as output probabilities of corresponding HMM states in the
HMM.  All computation will be done in a single process.

Note that the current implementation is very simple and limited.  Only
basic functions are implemented for NN.  Any number of hidden layers
can be defined, but the number of the nodes in the hidden layers
should be the same.  No batch computation is performed: all
frame-wise.  SIMD instruction (Intel AVX) is used to speed up the
computation.  Only tested on Windows and Ubuntu on Intel PC.
See "libsent/src/phmm/calc_dnn.c" for the actual implementation.

To run, you need

 1) an HMM AM (GMM defs are ignored, only its structure is used)
 2) a DNN definition that corresponds to 1)
 3) ".dnnconf" configuration file (text)

The .dnnconf file specifies the parameters, options, DNN definition
files, and other parameters all relating to DNN computation. A sample
file is located in the top directory of Julius archive as
"Sample.dnnconf".

The matrix/vector definitions should be given in ".npy" format
(i. e. python's "NumPy.save" format).  Only 32bit-float little endian
datatype is acceptable.

To prepare a model for DNN-HMM, note that the orders are important.
The order of the output nodes in the DNN should be the order of HMM
state definition id.  If not, Julius won't work properly.

Julius uses SIMD instruction for internal DNN computation. For Intel
CPU, dispatch function for several Intel SIMD instruction sets (SSE,
AVX and FMA) are implemented. You need gcc-4.7 or later to compile all
the codes.  They are all compiled and built-in into Julius, and will
be determined which one to use at run time.  Run "julius -setting" and
see which code will be used on your cpu.  AVX can be run on Sandy
Bridge, and FMA on Haswell, later one will run faster.  And for ARM
architecture, you can enable NEON SIMD codes by adding "--enable-neon"
to configure.


 A.2. Modular mode
 =====================

Julius still has capability of receiving state output probability
vector from other process.  This is an older way before 4.4.

To run, you need 

1) a GMM-HMM AM for Julius, (GMM defs are ignored, only HMM structure is used)
2) a DNN state definition of DNN-HMM that corresponds to 1),
3) a program to compute outprob vector from audio input using 2),either
   to file or to Julius socket.

The related Julius options are:
- "-input outprob" for file input of outprob vector,
- "-input vecnet" for vector input (feature/outprob auto-detected by header)

You can also see the demo samples in DNN dictation toolkit which is available on the Web.


B. State ID to make correspondence between outprob vector and states
=====================================================================

Julius should know the correspondence between the states in the HMM
definition and the dimension number of the given input vector.  The
dimension index, beginning from zero, should be assigned for each
state in the HMM definition.  The index is called "state ID" in this
document.

You can explicitly specify the state ID of each state within HMM
definition by embedding extra tag "<SID> value" in the hmmdefs.  When
the "<SID>" tag exist in the given HMM file, Julius uses them as
dimension to access the input outprob vector.  Other tools that
generate the outprob vector using DNN should also refer to the values
to generate an outprob vector in the proper order that matches the hmm
definition file.

If "<SID>" tag does not exist in the hmmdefs, Julius assigns the state
ID of each state in the order of appearance in the ASCII hmmdefs.  In
that case the input outprob vector should also have the values in the
same order.

- Detailed format definition:

The "<SID> value" should be inserted at the head of "state_info"
statement, as described in the section "HTK definition language" in the
HTKBook.  Currently it is not an official extension, and an hmmdefs
with "<SID>" embedded can not be used in the current HTK.  You can see
the example script of manually embedding the "<SID>" tag into hmmdefs
at the script "embed_sil.pl" in the archive.


C. Will the state ID (or the order) be kept in the binary HMM?
===============================================================

No at old versions, yes at the newer version.

The state ID will be kept in the binary HMM with mkbinhmm of this
version and later.  "<SID>" will be kept in the binary HMM.  If not,
the appearance order of the source will be saved.

Please note that the older version of mkbinhmm does not concern about
the order of appearance in the source hmmdefs.  You CANNOT use the
binary HMM generated by the older version for DNN.  When you want to
perform DNN-based recognition, please re-convert from ASCII hmmdefs
with the newest version of mkbinhmm.


D. Making outprob vector for Modular mode
==========================================

D.1. Format of outprob vector file
===================================

To make an outprob vector file, just save the state output
probabilities of each input frame in HTK parameter format with "USER"
parameter type.  The length of parameter vector should match the
number of states in the HMM definition.  If the source hmmdefs have
"<SID>" tag, the output vector should have the same dimension order.
If don't, you should store the values in the order of appearance of
state definitions in the source hmmdefs file.

Advice: HTK by default cannot handle a vector input longer than 5000
bytes (= 1250 dim.).  To handle large vector, you may have to modify
the source code of HTK.


D.2. Testing generation of an outprob vector file with Julius
--------------------------------------------------------------

Julius has a test function to save the outprob vector computed while
recognition.  Run recognition with "-outprobout filename" and process
an input file.  Then the state probabilities of the whole given input
will be written to the given filename.

Note that currently this function does not support batch processing
using "-filelist".  Only the last one will be saved.


D.3. Use the outprob vector for recognition
---------------------------------------------

Run Julius with "-input outprob", and give the outprob vector file as
an input.  Julius will refer to the pre-computed state probabilities
and perform decoding.

Julius still needs the source GMM-HMM definition to represent search
space.  You should specify the source GMM-HMM using "-h" as normal
recognition even if using "-input outprob", and the state-dimension
correspondence as described in the "B" section above should be kept.

The "-input outprob" also accepts batch input by "-filelist".


D.3. Sending feature / outprob vector via network
--------------------------------------------------

This version of Julius can receive input feature vector or outprob
vector from tcp/ip network to perform on-line recognition.  To use
this, start Julius with an option "-input outprobnet", and connect
from other program with port number 5531.

The sample tiny program to send feature vector or outprob vector is in
"dnntools/sendvec.c".  It reads a HTK parameter file and send it as
either input vector or outprob vector toward Julius. To test:

Terminal 1:
    (compile Julius)
    % ./julius/julius -C ..... -input vecnet

Terminal 2:
    % cd dnntools
    (edit sendvec.c to choose that the paramfile is whether an output
     vector file or a feature vector file)
    % cc -o sendvec sendvec.c
    % sendvec paramfile localhost

