set output "infantMortality.eps"


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

set xtics rotate by -45

set key off


set xdata time
set timefmt "%s"
#set xrange ["02/25/2018":"03/09/2018"]

#set xtics "01/01/2004",31536000,"01/01/2018" out

#set format x "%Y"
set format x "%b/%d"

#set timefmt "%m/%d/%y"

#set xtics rotate by -45
set xtics 345600

#set decimal locale
#set format y "%'g"


set style fill solid noborder

set size 1, .8


plot "infantMortality.dat" using ($2 - ( 8 * 3600 ) ):($4) with filledcurves y1=0 fillcolor rgb "#000000"