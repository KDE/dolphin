#!/bin/sh

# Converts all user's .kdelnk files to .desktop
# Necessary to avoid duplication when saving a mimetype or an applnk

find ~/.kde/share/mimelnk ~/.kde/share/applnk -name "*.kdelnk" -print |
 while read k; do
  d=`echo $k|sed -e 's/\.kdelnk/\.desktop/'`
  echo "Renaming $k to $d"
  mv "$k" "$d"
 done

# Convert user's bookmarks to .desktop and to remove .xpm from icons
find ~/.kde/share/apps/kfm/bookmarks -type f -print |
 while read k; do
  if echo $k | grep -q kdelnk; then  # kdelnk file
    d=`echo $k|sed -e 's/\.kdelnk/\.desktop/'`
    echo "Renaming $k to $d"
  else
    d=$k
    k=$d".tmp"
    mv "$d" "$k"
  fi
  sed -e 's/\.xpm//' "$k" > "$d"
  rm -f "$k"
 done

