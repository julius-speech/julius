    dfa_minimize

DFA_MINIMIZE(1)                                                DFA_MINIMIZE(1)



NAME
           dfa_minimize
          - Minimize a DFA grammar network

SYNOPSIS
       dfa_minimize [-o outfile] {dfafile}

DESCRIPTION
       dfa_minimize will convert an .dfa file to an equivalent minimal form.
       Output to standard output, or to a file specified by "-o" option.

       On version 3.5.3 and later, mkdfa.pl invokes this tool inside, and the
       output .dfa file will be always minimized, so you do not need to use
       this manually.

OPTIONS
        -o  outfile
           Output file. If not specified output to standard output.

EXAMPLES
       Minimize foo.dfa to bar.dfa:
       Another way:

SEE ALSO
        mkdfa.pl ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                   DFA_MINIMIZE(1)
