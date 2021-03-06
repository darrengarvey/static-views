#!/usr/bin/env gnuplot

#          Copyright Tom Westerhout 2017.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

if (! exists("testcase")) {
	print "Error: missing the 'testcase' argument"
	exit 1
}

if (! exists("input_file")) {
	print "Error: missing the 'input_file' argument"
	exit 1
}

if (! exists("output_file")) {
	print "Error: missing the 'output_file' argument"
	exit 1
}

set terminal pngcairo noenhanced color notransparent \
	font "DejaVu Sans Mono, 12"
set output output_file

set title 'Peak memory usage for "' . testcase . '"' offset 0,-0.5
set xlabel "Input size" offset 0,0.25
set ylabel  "RAM usage, Mb" offset 0.75,0
set y2label "Compilation time, s"
set xtics nomirror out
set ytics nomirror out
set y2tics nomirror out
set key left top

stats input_file u (column("ram")) name "M" nooutput
stats input_file u (column("time")) name "T" nooutput
stats input_file u (column("size")) name "X" nooutput

set xrange  [X_min : X_max]
set yrange  [0:]
set y2range [0:]

set tmargin 3
set label 1 time("%A, %d %b %Y") \
	at screen 1.0, screen 1.0 right offset -0.5,-0.5
plot \
	input_file u (column("size")):((column("ram") - M_min) / 1024.) w l \
		lt 1 lw 1 lc rgb "#777777" notitle, \
	""         u (column("size")):((column("ram") - M_min) / 1024.) w p \
		pt 7 ps 0.5 lc rgb "#2E911A" title "Memory usage", \
	""         u (column("size")):(column("time") / 1000.) axes x1y2 w l \
		lt 1 lw 1 lc rgb "#777777" notitle, \
	""         u (column("size")):(column("time") / 1000.) axes x1y2 w p \
		pt 7 ps 0.5 lc rgb "#1A6A91" title "Compilation time"

set output
