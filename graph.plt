# Gnuplot script file for plotting data in file "data.dat"
set autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set title "Time to sort vs Number of Threads"
set xlabel "Number of Threads"
set ylabel "Time to sort (milliseconds)"
set style data linespoints
set grid
set key left top
set terminal png size 800,600
set output filename
plot "data.dat" using 2:1 with linespoints title "Time to sort"
