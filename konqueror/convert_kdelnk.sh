#!/bin/sh

# Converts all user's .kdelnk files to .desktop
# Necessary to avoid duplication when saving a mimetype or an applnk

find ~/.kde/share/mimelnk ~/.kde/share/applnk -name "*.kdelnk" -print |
 while read k; do
  d=`echo $k|sed -e 's/\.kdelnk/\.desktop/'`
  echo "Renaming $k to $d"
  mv "$k" "$d"
 done

