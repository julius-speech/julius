    mkbinhmm

MKBINHMM(1)                                                        MKBINHMM(1)



NAME
           mkbinhmm
          - convert HMM definition file in HTK ascii format to Julius binary
       format

SYNOPSIS
       mkbinhmm [-htkconf HTKConfigFile] {hmmdefs_file} {binhmm_file}

DESCRIPTION
       mkbinhmm convert an HMM definition file in HTK ascii format into a
       binary HMM file for Julius. It will greatly speed up the launch
       process.

       You can also embed acoustic analysis condition parameters needed for
       recognition into the output file. To embed the parameters, specify the
       HTK Config file you have used to extract acoustic features for training
       the HMM by the optione "-htkconf".

       The embedded parameters in a binary HMM format will be loaded into
       Julius automatically, so you do not need to specify the acoustic
       feature options at run time. It will be convenient when you deliver an
       acoustic model.

       You can also specify binary file as the input. This can be used to
       update the old binary format into new one, or to embed the config
       parameters into the already existing binary files. If the input binhmm
       already has acoustic analysis parameters embedded, they will be
       overridden by the specified values.

       mkbinhmm can read gzipped file as input.

OPTIONS
        -htkconf  HTKConfigFile
           HTK Config file you used at training time. If specified, the values
           are embedded to the output file.

       hmmdefs_file
           The source HMm definitino file in HTK ascii format or Julius binary
           format.

       hmmdefs_file
           Output file.

EXAMPLES
       Convert HTK ascii format HMM definition file into Julius binary file:
       Furthermore, embed acoustic feature parameters as specified by Config
       file
       Embed the acoustic parameters into an existing binary file

SEE ALSO
        julius ( 1 ) ,
        mkbingram ( 1 ) ,
        mkbinhmmlist ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                       MKBINHMM(1)
