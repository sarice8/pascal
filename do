#!/bin/csh -f

foreach i ($*)
	set src=`echo $i.c | sed -e "s^\.c\.c^\.c^"`
	set obj=`echo $i | sed -e "s^\.c^^"`

	echo "Compiling $src to $obj"
	cc $src -g -o $obj
end
