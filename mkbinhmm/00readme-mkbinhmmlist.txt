    mkbinhmmlist

MKBINHMMLIST(1)                                                MKBINHMMLIST(1)



NAME
           mkbinhmmlist
          - convert HMMList file into binary format

SYNOPSIS
       mkbinhmmlist {hmmdefs_file} {HMMList_file} {output_binhmmlist_file}

DESCRIPTION
       mkbinhmmlist converts a HMMList file to binary format. Since the index
       trees for lookup are also stored in the binary format, it will speed up
       the startup of Julius, namely when using big HMMList file.

       For conversion, HMM definition file hmmdefs_file that will be used
       together at Julius needs to be specified. The format of the HMM
       definition file can be either ascii or Julius binary format.

       The output binary file can be used in Julius as the same by "-hlist".
       The format wil be auto-detected by Julius.

       The pseudo phone extracted form acoustic model is outputed by version
       4.2 or later. This cause faster starting Julius. However, It is
       required that using it with created acoustic model. Also, binhmmlist
       file created by mkbinhmmlist of this version or later cannot be used in
       earlier version.

       mkbinhmmlist can read gzipped file.

OPTIONS
       hmmdefs_file
           Acoustic HMM definition file, in HMM ascii format or Julius binary
           format.

       HMMList_file
           Source HMMList file

       output_binhmmlist_file
           Output file, will be overwritten if already exist.

EXAMPLES
       Convert a HMMList file logicalTri into binary format and store to
       logicalTri.bin:

SEE ALSO
        julius ( 1 ) ,
        mkbinhmm ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                   MKBINHMMLIST(1)
