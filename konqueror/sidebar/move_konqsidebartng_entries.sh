#!/bin/sh

prefix=$(kde-config --localprefix)
source="$prefix/share/apps/konqsidebartng"

[ -d "$source/entries" ] || exit 0

profiles="filemanagement webbrowsing"
for profile in $profiles; do
	dest="$source/$profile/entries"
	if [ ! -d "$dest" ]; then
		mkdir -p "$dest" || exit 1
		cp $source/entries/.version $dest/
		cp $source/entries/* $dest/
	fi
done

rm -rf $source/entries
