set output "playerStats.eps"


set terminal post enhanced color


#set style fill solid border -1

#set boxwidth 0.5

set lmargin 5
set rmargin 6
#set bmargin 2

set size 1.0, .8

set grid ytics linewidth 4
set grid xtics linewidth 4
show grid

set xtics 3600 * 24 * 30
set xtics rotate by -90

set key off


set xdata time
set timefmt "%s"
#set xrange ["01/25/2018":"03/09/2019"]

#set xtics "01/01/2004",31536000,"01/01/2018" out

#set format x "%Y"
set format x "%b/%d"

#set timefmt "%m/%d/%y"

#set xtics rotate by -45
#set xtic 52

#set decimal locale
#set format y "%'g"

set xtics out

set style fill solid noborder

set size 1, .8


plot "playerStatsTrimmed3_ave.dat" using ($1 - ( 8 * 3600 ) ):($2) with filledcurves y1=0 fillcolor rgb "#000066", "" using ($1 - ( 8 * 3600 ) ):($3) with lines lw 3 lt 1 linecolor rgb "#FF0000"