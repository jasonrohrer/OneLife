# process family data log with:
# cat familyDataLog.txt | sed -e "s/ [a-zA-Z]*:/ /g" > rawData.txt


set xlabel "Time"
set format x "%H:%M"
set timefmt "%s"
set xdata time

set multiplot layout 3,1
plot "rawData.txt" using 1:6 with lines title "Players", "rawData.txt" using 1:4 with lines title "Moms", "rawData.txt" using 1:5 with lines title "Babies"


plot "rawData.txt" using 1:3 with lines title "Families", "rawData.txt" using 1:7 with lines title "Eves",

plot "rawData.txt" using 1:9 with lines title "Ave Age"
unset multiplot