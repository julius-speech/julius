    mkgshmm

MKGSHMM(1)                                                          MKGSHMM(1)



NAME
           mkgshmm
          - convert monophone HMM to GS HMM for Julius

SYNOPSIS
       mkgshmm {monophone_hmmdefs}
                 >
                  {outputfile}

DESCRIPTION
       mkgshmm converts monophone HMM definition file in HTK format into a
       special format for Gaussian Mixture Selection (GMS) in Julius.

       GMS is an algorithm to reduce the amount of acoustic computation with
       triphone HMM, by pre-selection of promising gaussian mixtures using
       likelihoods of corresponding monophone mixtures.

EXAMPLES
       (1) Prepare a monophone model which was trained by the same corpus as
       target triphone model.

       (2) Convert the monophone model using mkgshmm.
       (3) Specify the output file in Julius with option "-gshmm"

SEE ALSO
        julius ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                        MKGSHMM(1)
