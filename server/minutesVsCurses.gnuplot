f(x) = a + b*x + c*x**2  + d* x**3

fit f(x) 'minutesVsCurses.dat' using 1:2 via a, b, c, d


set xlabel "Minutes Played in last 30 days" 

set ylabel "Curses"


plot "minutesVsCurses.dat" using 1:2 with points, a + b * x + c * x**2 + d * x**3 title "Cubic best fit"