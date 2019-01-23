# dnntools

DNN-HMM related tiny tools for Julius.

## Synopsis

```shell
% cc -o sendvec sendvec.c
% ./sendvec paramFile hostname [PortNum]
```

```shell
% ./embed_sid.pl < source_hmmdefs > embedded_hmmdefs
```

## Description

`sendvec.c` is a sample stand-alone program that sends either feature vectors or
output probability vectors to Julius via TCP/IP.  The `paramFile` should be a
file in HTK parameter file format.  When compiled without definition of
`OUTPROBVECTOR`, the `sendvec` will send the parameter as feature vector, which
can be received by Julius running with `-input vecnet`.  When compiled with
`OUTPROBVECTOR` defined, `sendvec` will send the parameter file as output
probability vectors, that can be received by Julius with `-input outprob`.  Note
that Julius will receive any vectors without checking the type and length of
received vectors, so you should send the same type and size of the vector as
assumed in Julius side.

`embed_sid.pl` is a perl script that can embed `<SID>` tags to HMM definition
file.  The `<SID>` tag can be used to make exact correspondence for HMM state
id, between frontend outprob calculator and Julius on server-client DNN-HMM
recognition.  See "00readme-DNN.txt" in the top directory of Julius archive for
details about HMM state id matching.

### Installing

The tools in this directory will NOT be installed automatically when installing
Julius.  You should compile/run it manually.

## Usage

Send feature vector file to Julius running with `-input vecnet` at localhost:

```shell
% ./sendvec htkVectorFile localhost
```

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
