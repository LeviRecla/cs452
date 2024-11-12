# Project Analysis

## Were you able to generate something close to what the example showed?
- Yes, I was able to produce a result that was relatively close to the example provided in terms of general structure and trend, though there were some differences in the exact timings shown on the graph. My graph displayed a similar overall shape, where sorting initially proceeded more slowly, then accelerated incrementally until reaching the optimal number of threads. However, I encountered an issue when running my shell script, which prevented the plot from fully generating in Gnuplot. The issue was related to an AddressSanitizer error that intermittently arises on my machine but does not occur consistently on all systems. Due to this, I had to run each incremental thread level manually from the command line, recording the output data in a spreadsheet. Once I had all the necessary data points, I imported the data into Excel and plotted the graph there. This approach allowed me to approximate the graph that would have been generated if my shell script had fully executed as expected. Typically, I could test up to around 15 threads before the AddressSanitizer error caused execution to halt.

## Did you see a slowdown at some point? Why or why not?
- Yes, I observed a noticeable slowdown in performance after the program exceeded 8 threads on my machine. This is because my CPU has 8 physical cores, which represents the maximum number of threads it can execute in true parallel without requiring additional overhead. Once the thread count exceeds the number of physical cores, the threads can no longer execute in parallel on separate cores, and the system has to engage in context switching to manage the extra threads. This context switching adds overhead, which can counteract the benefits of parallel execution and does not lead to further performance improvements.

## Did your program run faster and faster when you added more threads?
- For the most part, yes. My CPU is equipped with 8 cores, each overclocked to a maximum speed of 5GHz, which resulted in a substantial performance boost with multithreading up to 8 threads. The performance improvement was most dramatic when moving from 1 to 2 threads, with significant gains up to 8 threads, but the incremental performance benefits became less noticeable after the initial few threads.

## Why or why not?
- Adding more threads allows the workload to be distributed across multiple cores, enabling the sorting tasks to be completed in parallel, which improves overall execution speed. Because each CPU core on my machine is overclocked, the individual cores are capable of executing instructions at a very high speed, maximizing the benefits of parallel execution. However, as the thread count exceeds the physical core count, context switching and resource sharing start to affect performance, which explains why the speedup became less pronounced.

## What was the optimum number of threads for your machine?
- The optimal number of threads for my machine was 8, matching the number of physical cores available. This allowed each thread to execute on its own core without the need for context switching or resource contention, providing the best performance.

## What was the slowest number of threads for your machine?
- The slowest number of threads for my machine was 1. With only a single thread, the program could not take advantage of parallel execution and was limited to running the sorting algorithm sequentially on a single core, resulting in the longest execution time.

