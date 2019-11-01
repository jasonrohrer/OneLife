set yrange [0:100]
set grid 

plot "simData4.txt" using 1:2 with lines title "Ave Error", "" using 1:3 with lines title "Overlap", "" using 1:4 with lines title "Sum"

pause -1