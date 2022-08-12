#!/bin/bash
set -u -o pipefail

obminimize -o mol -ff mmff94 $1
if [ $? -eq 0 ] ; then
  echo "> <Method>"
  echo "OpenBabel-MMFF94"
  echo
else
  obminimize -o mol -ff uff $1
  if [ $? -eq 0 ] ; then
    echo "> <Method>"
    echo "OpenBabel-UFF"
    echo
  fi
fi
exit $?
