#!/bin/sh

prefix=`kde-config --localprefix`
source="$prefix/share/icons/favicons"
dest="$prefix/share/cache/favicons"

if [ -n "$prefix" -a -d "$source" ]; then
	while [ ! -d "$dest" ]; do
		dir="$dest"
		while [ ! -d `dirname "$dir"` ]; do
			dir=`dirname "$dir"`
		done
		mkdir "$dir" || exit 1
	done

	icons=`ls "$source" 2>/dev/null`
	if [ -n "$icons" ]; then
		for i in $icons; do
			mv "$source/$i" "$dest/$i"
		done
	fi
	rmdir "$source"
fi
