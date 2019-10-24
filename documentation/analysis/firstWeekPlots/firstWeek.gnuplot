set xdata time
set timefmt "%Y-%m"

set yrange [0:19]

set xtics rotate

plot "firstTwoMonths.txt"  using 1:2 with linespoints title "Hours played in first two months of ownership",  "firstWeek.txt"  using 1:2 with linespoints title "Hours played in first week of ownership", "firstMonth.txt"  using 1:2 with linespoints title "Hours played in first month of ownership",  "firstDay.txt"  using 1:2 with linespoints title "Hours played in first day of ownership"

pause -1