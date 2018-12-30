#!/bin/sh

DATADIR=$(dirname "$0")/data
EXPECTFILE=$DATADIR/wavtest.expect
FVADWAV=../examples/fvadwav
RESULTFILE=./wavtest.result

if [ ! -x "$FVADWAV" ]; then
    echo "$FVADWAV not executable, skipping test"
    exit 77
fi

> "$RESULTFILE"
for SMPRATE in 8 16 32 48; do
    SRCFILE=audio_tiny$SMPRATE.wav
    for MODE in 0 1 2 3; do
        for FRMLEN in 10 20 30; do
            TITLE="$SRCFILE; mode $MODE; $FRMLEN ms"
            echo "processing $TITLE..."
            echo "=== $TITLE ===" >> "$RESULTFILE"
            $FVADWAV -m $MODE -f $FRMLEN $DATADIR/$SRCFILE >> "$RESULTFILE" || exit 1
            echo >> "$RESULTFILE"
        done
    done
done

echo "comparing result..."
diff -b "$RESULTFILE" "$EXPECTFILE" || exit 2
