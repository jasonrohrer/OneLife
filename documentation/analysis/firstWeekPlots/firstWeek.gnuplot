set xdata time
set timefmt "%Y-%m"

set yrange [0:25]

set xtics rotate

plot "firstTwoMonths.txt"  using 1:2 with linespoints title "Hours played in first two months of ownership", "firstMonth.txt"  using 1:2 with linespoints title "Hours played in first month of ownership",  "firstWeek.txt"  using 1:2 with linespoints title "Hours played in first week of ownership", "firstDay.txt"  using 1:2 with linespoints title "Hours played in first day of ownership", "firstHour.txt"  using 1:2 with linespoints title "Hours played in first hour of ownership"

pause -1

set yrange [0:1.5]

plot "firstTwoMonths.txt"  using 1:5 with linespoints title "Fraction who quit in first two months of ownership", "firstMonth.txt"  using 1:5 with linespoints title "Fraction who quit in first month of ownership", "firstWeed.txt"  using 1:5 with linespoints title "Fraction who quit in first week of ownership", "firstDay.txt"  using 1:5 with linespoints title "Fraction who quit in first day of ownership", "firstHour.txt"  using 1:5 with linespoints title "Fraction who quit in first hour of ownership"

pause -1