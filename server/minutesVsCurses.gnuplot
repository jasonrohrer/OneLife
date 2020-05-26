f(x) = a + b*x + c*x**2  + d* x**3 + e * x **4 + g* x**5

fit f(x) 'minutesVsCurses.dat' using 1:2 via a, b, c, d, e, g


set xlabel "Minutes Played in last 30 days" 

set ylabel "Curses"

set yrange [0:40]

plot "minutesVsCurses.dat" using 1:2 with points, a + b * x + c * x**2 + d * x**3 + e * x**4  + g * x**5 title "Quintic best fit"


pause -1