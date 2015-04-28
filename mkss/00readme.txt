    mkss

MKSS(1)                                                                MKSS(1)



NAME
           mkss
          - calculate average spectrum for spectral subtraction

SYNOPSIS
       mkss [options...] {filename}

DESCRIPTION
       mkss is a tool to estimate noise spectrum for spectral subtraction on
       Julius. It reads a few seconds of sound data from microphone input,
       calculate the average spectrum and save it to a file. The output file
       can be used as a noise spectrum data in Julius (option "-ssload").

       The recording will start immediately after startup. Sampling format is
       16bit, monoral. If outpue file already exist, it will be overridden.

OPTIONS
        -freq  Hz
           Sampling frequency in Hz (default: 16,000)

        -len  msec
           capture length in milliseconds (default: 3000)

        -fsize  sample_num
           frame size in number of samples (default: 400)

        -fshift  sample_num
           frame shift in number of samples (default: 160)

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



                                  12/19/2013                           MKSS(1)
