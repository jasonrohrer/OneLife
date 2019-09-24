set xlabel "Time"
set format x "%m/%d - %H:%M"
set timefmt "%s"
set xdata time

set xtics font "Verdana,5"

set xtics rotate

set xlabel offset 0,3

set key autotitle columnhead

set key outside

set bmargin 5

#set terminal wxt size 800,600



# make thin for stacked curves
# make thick for individual lines
myLineWidth = 1

# 99% accessible colors taken from here:
# https://sashat.me/2017/01/11/list-of-20-simple-distinct-colors/
set linetype  1 lc rgb "#e6194B" lw myLineWidth
set linetype  2 lc rgb "#3cb44b" lw myLineWidth
set linetype  3 lc rgb "#ffe119" lw myLineWidth
set linetype  4 lc rgb "#4363d8" lw myLineWidth
set linetype  5 lc rgb "#f58231" lw myLineWidth
set linetype  6 lc rgb "#42d4f4" lw myLineWidth
set linetype  7 lc rgb "#f032e6" lw myLineWidth
set linetype  8 lc rgb "#fabebe" lw myLineWidth
set linetype  9 lc rgb "#469990" lw myLineWidth
set linetype  10 lc rgb "#e6beff" lw myLineWidth
set linetype  11 lc rgb "#9A6324" lw myLineWidth
set linetype  12 lc rgb "#fffac8" lw myLineWidth
set linetype  13 lc rgb "#800000" lw myLineWidth
set linetype  14 lc rgb "#aaffc3" lw myLineWidth
set linetype  15 lc rgb "#000075" lw myLineWidth
set linetype  16 lc rgb "#a9a9a9" lw myLineWidth
set linetype  17 lc rgb "#000000" lw myLineWidth


firstcol=2
cumulated(i)=((i>firstcol)?column(i)+cumulated(i-1):(i==firstcol)?column(i):1/0)


# to show roughly first hour after Eve window closed
#set xrange ["1567511934.11":"1567516955.04"]

#plot "1567511928_familyPopLog.txt" using 1:(cumulated(7)) title columnhead(7) with filledcurves x1, "" using 1:(cumulated(6)) title columnhead(6) with filledcurves x1, "" using 1:(cumulated(5)) title columnhead(5) with filledcurves x1, "" using 1:(cumulated(4)) title columnhead(4) with filledcurves x1, "" using 1:(cumulated(3)) title columnhead(3) with filledcurves x1, "" using 1:(cumulated(2)) title columnhead(2) with filledcurves x1,



#plot "1568799648_familyPopLog.txt" using 1:(cumulated(7)/cumulated(7)) title columnhead(7) with filledcurves x1, "" using 1:(cumulated(6)/cumulated(7)) title columnhead(6) with filledcurves x1, "" using 1:(cumulated(5)/cumulated(7)) title columnhead(5) with filledcurves x1, "" using 1:(cumulated(4)/cumulated(7)) title columnhead(4) with filledcurves x1, "" using 1:(cumulated(3)/cumulated(7)) title columnhead(3) with filledcurves x1, "" using 1:(cumulated(2)/cumulated(7)) title columnhead(2) with filledcurves x1,


# this version shows total population instead of fraction of total
# best so far

plot "1569203320_familyPopLog.txt" using 1:(cumulated(11)) title columnhead(11) with filledcurves x1, "" using 1:(cumulated(10)) title columnhead(10) with filledcurves x1, "" using 1:(cumulated(9)) title columnhead(9) with filledcurves x1, "" using 1:(cumulated(8)) title columnhead(8) with filledcurves x1, "" using 1:(cumulated(7)) title columnhead(7) with filledcurves x1, "" using 1:(cumulated(6)) title columnhead(6) with filledcurves x1, "" using 1:(cumulated(5)) title columnhead(5) with filledcurves x1, "" using 1:(cumulated(4)) title columnhead(4) with filledcurves x1, "" using 1:(cumulated(3)) title columnhead(3) with filledcurves x1, "" using 1:(cumulated(2)) title columnhead(2) with filledcurves x1,


# and separate lines, if we need to show them

#plot "1568917336_familyPopLog.txt" using 1:11 title columnhead(11) with lines , "" using 1:10 title columnhead(10) with lines , "" using 1:9 title columnhead(9) with lines , "" using 1:8 title columnhead(8) with lines , "" using 1:7 title columnhead(7) with lines , "" using 1:6 title columnhead(6) with lines , "" using 1:5 title columnhead(5) with lines , "" using 1:4 title columnhead(4) with lines , "" using 1:3 title columnhead(3) with lines , "" using 1:2 title columnhead(2) with lines ,


pause -1