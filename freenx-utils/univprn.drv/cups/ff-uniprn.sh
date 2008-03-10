#! /bin/bash
# see http://localhost:631/spm.html#WRITING_FILTERS
# debug info in /var/log/cups/error_log
set -x
# Have some dummy Xserver like Xvnc running there
export DISPLAY=:1.0
# have the input at fd0 (stdin) in any case
[ -n "$6" ] && exec <"$6"
# prefiltering
TMPDIR=$(mktemp -d /tmp/ff-uniprn.XXXXXXXXXX) || exit 1
(
cd $TMPDIR
dd bs=1k of=testdoc.pdf
LD_LIBRARY_PATH=/usr/local/lib/ wine /home/info/ff/public/SPLFilter/SPLFilter.exe
cat test.spl
)
rm -rf "$TMPDIR"

