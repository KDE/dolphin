#!/bin/sh

prefix=$(kde-config --localprefix)
source1="$prefix/share/icons/favicons"
source2="$prefix/share/cache/favicons"
dest="$(kde-config --path cache)/favicons"

if [ -n "$prefix" -a -d "$source1" ]; then
	while [ ! -d "$dest" ]; do
		dir="$dest"
		while [ ! -d `dirname "$dir"` ]; do
			dir=`dirname "$dir"`
		done
		mkdir "$dir" || exit 1
	done

	icons=`ls "$source1" 2>/dev/null`
	if [ -n "$icons" ]; then
		for i in $icons; do
			mv -f "$source1/$i" "$dest/$i"
		done
	fi
	rmdir "$source1"
fi
if [ -n "$prefix" -a -d "$source2" ]; then
	while [ ! -d "$dest" ]; do
		dir="$dest"
		while [ ! -d `dirname "$dir"` ]; do
			dir=`dirname "$dir"`
		done
		mkdir "$dir" || exit 1
	done

	icons=`ls "$source2" 2>/dev/null`
	if [ -n "$icons" ]; then
		for i in $icons; do
			mv -f "$source2/$i" "$dest/$i"
		done
	fi
	rmdir "$source2"
fi
