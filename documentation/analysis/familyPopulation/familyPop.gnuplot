set xlabel "Time"
set format x "%m/%d - %H:%M"
set timefmt "%s"
set xdata time

set xtics rotate

set key autotitle columnhead

set key outside

set terminal wxt size 800,600


firstcol=2
cumulated(i)=((i>firstcol)?column(i)+cumulated(i-1):(i==firstcol)?column(i):1/0)


# to show roughly first hour after Eve window closed
#set xrange ["1567511934.11":"1567516955.04"]

#plot "1567511928_familyPopLog.txt" using 1:(cumulated(7)) title columnhead(7) with filledcurves x1, "" using 1:(cumulated(6)) title columnhead(6) with filledcurves x1, "" using 1:(cumulated(5)) title columnhead(5) with filledcurves x1, "" using 1:(cumulated(4)) title columnhead(4) with filledcurves x1, "" using 1:(cumulated(3)) title columnhead(3) with filledcurves x1, "" using 1:(cumulated(2)) title columnhead(2) with filledcurves x1,



plot "1567511928_familyPopLog.txt" using 1:(cumulated(7)/cumulated(7)) title columnhead(7) with filledcurves x1, "" using 1:(cumulated(6)/cumulated(7)) title columnhead(6) with filledcurves x1, "" using 1:(cumulated(5)/cumulated(7)) title columnhead(5) with filledcurves x1, "" using 1:(cumulated(4)/cumulated(7)) title columnhead(4) with filledcurves x1, "" using 1:(cumulated(3)/cumulated(7)) title columnhead(3) with filledcurves x1, "" using 1:(cumulated(2)/cumulated(7)) title columnhead(2) with filledcurves x1,

pause -1