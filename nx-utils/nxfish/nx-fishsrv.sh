#!/bin/sh

# nx-fishssrv.sh - Copyright (c) 2007 by Fabian Franz.
#
#    A simple wrapper script for nx-fishsrv.pl with proper authentication.
#
# This is an example how to start nx-fishsrv using faucet and the supplied 
# nx-fishsrv.pl
#
# If - is supplied as cookie the password is read from stdin.
#
# Again: 6201 is just an arbitrary port I use for testing.
#
# It is assumed that you forward the port via ssh -R.

if [ $# -lt 1 ]
then
	echo "Usage: $(basename $0) <directory> <cookie> [port]"
fi

if [ $1 = '-' ]
then
	echo -n "Please provide password: "
	read -s rawcookie
	echo ""
else
	rawcookie="$2"
fi
echo "Starting nx-fishsrv.pl ..."

PORT=$3
[ -z "$PORT" ] && PORT=6201

# We use md5sum so that no one can get our original session cookie to easily
cookie=$(htpasswd -nbs nxfish ":NXFISH:$rawcookie" | cut -d: -f2 )

faucet $PORT -io sh -c 'echo "FISH:" ; ./nx-fishsrv.pl '"'$1'"' '"'$cookie'"''
