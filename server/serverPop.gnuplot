set xdata time
set timefmt "%s"

set grid ytics
set grid xtics

show grid

set xtics 3600 * 24
set xtics rotate by -90

set format x "%m/%d/%Y"

#set xrange ['1579348800':'1579564800']

set xrange ['1579435200':'1579478400']

plot "~/popData2020/2020_pop_bs2.txt" using 1:2 with lines title "bigserver2", "~/popData2020/2020_pop_s1.txt" using 1:2 with lines title "server1"

pause -1