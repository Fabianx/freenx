#! /bin/bash
# see http://localhost:631/spm.html#WRITING_FILTERS
# debug info in /var/log/cups/error_log
set -x
# have the input at fd0 (stdin) in any case
[ -n "$6" ] && exec <"$6"
# prefiltering
ps2pdf - - | /usr/lib/cups/filter/ff-uniprn.sh
