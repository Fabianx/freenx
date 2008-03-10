#!/bin/sh
#
# Copyright 2007 Google Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# Author: diamond@google.com (Stephen Shirley)

EXTERN_STDIN=7
EXTERN_STDOUT=8
EXTERN_STDERR=9

aTrue() { assertTrue "$@"; }
aEquals() { assertEquals "$@"; }

readLineX()
{
  lineno="$1"
  head -n "$lineno" | tail -1
}

toupper()
{
  trim "$@" | tr '[:lower:]' '[:upper:']
}

trim()
{
  echo "$@" | tr -d '\r\n'
}

readTrimToupper()
{
  local resp
  IFS= read resp <&${EXTERN_READ:-$EXTERN_STDOUT}
  resp=$(trim "$resp")
  toupper "$resp"
}

recvCheck()
{
  local comment="$1"
  local expected="$(toupper "$2")"

  local resp="$(readTrimToupper)"
  #echo "Resp: \"$resp\" Exp: \"$expected\""

  aEquals "$comment" "$expected" "$resp"
}

recvCheckRegx()
{
  local comment="$1"
  local regex="$2"

  local resp="$(readTrimToupper)"
  #echo "Resp: \"$resp\" Regex: \"$regex\""

  echo "$resp" | egrep -ixq "$regex"
  aTrue "$comment didn't match \"$regex\" (Got \"$resp\")" $?
}

sendRecvCheck()
{
  local send="$1"
  local comment="$2"
  local expected="$3"

  eval "echo \"$send\" >&${EXTERN_WRITE:-$EXTERN_STDIN}"
  recvCheck "$comment" "$expected"
}

sendRecvCheckRegx()
{
  local send="$1"
  local comment="$2"
  local regex="$3"

  eval "echo \"$send\" >&${EXTERN_WRITE:-$EXTERN_STDIN}"
  recvCheckRegx "$comment" "$regex"
}

setupLoggingStderr()
{
  logger_addAppender stderr
  appender_setType stderr FileAppender
  appender_file_setFile stderr STDERR
  appender_activateOptions stderr
}

setupPaths()
{
  TEST_DIR="$__shunit_tmpDir"
  SRC_DIR="${SRC_DIR:-$(cd .. && pwd -L)}"
  PATH_ETC="${PATH_ETC:-$TEST_DIR/etc}"
  PATH_BIN="${PATH_BIN:-$TEST_DIR/bin}"
  PATH_LIB="${PATH_LIB:-$TEST_DIR/lib}"
  PATH="$PATH:$PATH_BIN"
}

setupLoadConfig()
{
  setupPaths
  setupTestDirs
  cat << EOF > "$PATH_ETC/general.conf"
LOG_LEVEL=DEBUG
PATH_BASE="$TEST_DIR"
PATH="$PATH"
EOF
}

setupTestDirs()
{
  set -e
  chmod 755 "$TEST_DIR"
  rm -rf "$PATH_BIN"
  mkdir -pm 755 "$PATH_BIN"
  (
    cd "$PATH_BIN"
    for file in "$SRC_DIR/"{compatibility,utility}/*; do
      cp "$file" .
    done
    chmod 755 *
  )
  cp -R "$SRC_DIR/lib" "$TEST_DIR"
  chmod -R u=rwX,g=rX,o=X "$PATH_LIB"
  mkdir -pm 755 "$PATH_ETC"
  set +e
}
