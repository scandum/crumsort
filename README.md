Intro
-----
This document describes a hybrid quicksort / mergesort named crumsort. The sort is unstable, adaptive, branchless, and has exceptional performance.

Analyzer
--------
Crumsort starts out with an analyzer that sorts fully in-order or reverse-order arrays using n comparisons. It also obtains a measure of presortedness and switches to [quadsort](https://github.com/scandum/quadsort) if the array is more than 66% ordered.

Partitioning
------------
Partitioning is performed in a top-down manner similar to quicksort. Crumsort obtains the pseudomedian of 9 for partitions smaller than 2048 elements, and the pseudomedian of 25 otherwise. The median element obtained will be referred to as the pivot. Partitions that grow smaller than 24 elements are sorted with quadsort.

Fulcrum Partition
-----------------
After obtaining a pivot the array is parsed from start to end using the fulcrum partitioning scheme. The scheme is similar to the original quicksort scheme known as the Hoare partition with some notable differences. The differences are perhaps best explained with two code examples.
```c
int hoare_partition(int array[], int head, int tail)
{
        int pivot = head++;
        int swap;

        while (1)
        {
                while (array[head] <= array[pivot] && head < tail)
                {
                        head++;
                }

                while (array[tail] > array[pivot])
                {
                        tail--;
                }

                if (head >= tail)
                {
                        swap = array[pivot]; array[pivot] = array[tail]; array[tail] = swap;

                        return tail;
                }
                swap = array[head]; array[head] = array[tail]; array[tail] = swap;
        }
}
```

```c
int fulcrum_partition(int array[], int head, int tail)
{
        int pivot = array[head];

        while (1)
        {
                if (head == tail)
                {
                        array[head] = pivot;
                        return head;
                }

                if (array[tail] > pivot)
                {
                        tail--;
                        continue;
                }
                array[head++] = array[tail];

                while (1)
                {
                        if (head == tail)
                        {
                                array[tail] = pivot;
                                return tail;
                        }

                        if (array[head] <= pivot)
                        {
                                head++;
                                continue;
                        }
                        array[tail--] = array[head];
                        break;
                }
        }
}
```
Instead of using multiple swaps the fulcrum partition creates a 1 element swap space, with the pivot holding the original data. Doing so turns the 3 assignments from the swap into 2 assignments. Overall the fulcrum partition has a 10-20% performance improvement on random data.

The biggest downside is that hoare partitioning takes advantage of leaving the pivot in the distribution, allowing it to perform an unguarded loop. This gives the hoare partition a 2x performance improvement in the worst case. This is significant enough for anyone who experimented with such a partitioning scheme to abandon it as useless.

However, the swap space of the fulcrum partition can easily be increased from 1 to 32 elements to allow it to perform 16 unguarded comparisons at a time, which in turn also allows it to perform these comparisons in a branchless manner with no additional overhead.

Worst case handling
-------------------
To avoid run-away recursion crumsort switches to quadsort for both partitions if one partition is less than 1/16th the size of the other partition. On a distribution of random unique values the observed chance of a false positive is 1 in 1,336 for the pseudomedian of 9 and approximately 1 in 4 million for the pseudomedian of 25.

Combined with the analyzer crumsort starts out with this makes the existence of killer patterns unlikely, other than a 33-50% performance slowdown by prematurely triggering the use of quadsort.

Branchless optimizations
------------------------
Crumsort uses a branchless comparison optimization. The ability of quicksort to partition branchless was first described in "BlockQuicksort: How Branch Mispredictions don't affect Quicksort" by Stefan Edelkamp and Armin Weiss. Crumsort uses the fulcrum partitioning scheme where as BlockQuicksort uses a scheme resembling Hoare partitioning.

Median selection uses a branchless comparison technique that selects the pseudomedian of 9 using 12 comparisons, and the pseudomedian of 25 using 42 comparisons.

These optimizations do not work as well when the comparisons themselves are branched and the largest performance increase is on 32 and 64 bit integers.

Generic data optimizations
--------------------------
Crumsort uses a method popularized by [pdqsort](https://github.com/orlp/pdqsort) to improve generic data handling. If the same pivot is chosen twice in a row it performs a reverse partition, filtering out all elements equal to the pivot, next it carries on as usual. This typically only occurs when sorting tables with many repeating values, like gender, education level, birthyear, zipcode, etc.

Large array optimizations
-------------------------
For partitions larger than 65536 elements crumsort obtains the median of 128 or 256. It does so by swapping 128 or 256 random elements to the start of the array, next sorting them with quadsort, and taking the center element. Using pseudomedian instead of median selection on large arrays is slower, likely due to cache pollution.

Small array optimizations
-------------------------
Most modern quicksorts use insertion sort for partitions smaller than 24 elements. Crumsort uses quadsort which has a dedicated small array sorting routine that outperforms insertion sort.

Data Types
----------
The C implementation of crumsort supports long doubles and 8, 16, 32, and 64 bit data types. By using pointers it's possible to sort any other data type, like strings.

Interface
---------
Crumsort uses the same interface as qsort, which is described in [man qsort](https://man7.org/linux/man-pages/man3/qsort.3p.html).

Porting
-------
People wanting to port crumsort might want to have a look at [fluxsort](https://github.com/scandum/fluxsort), which is a little bit simpler because it's stable and out of place.

Memory
------
Crumsort uses 512 elements of stack memory, which is shared with quadsort. Recursion requires log n stack memory.

Crumsort can be configured to use sqrt(n) memory, with a minimum memory requirement of 32 elements.

Performance
-----------
Crumsort will begin to outperform fluxsort on random data right around 1,000,000 elements. Since it runs on 512 elements of auxiliary memory the sorting of ordered data will be slower for larger arrays.

Because of the partitioning scheme crumsort is slower than pdqsort when sorting long doubles. Fixing this is on my todo list.

Big O
-----
```cobol
                 ┌───────────────────────┐┌───────────────────────┐
                 │comparisons            ││swap memory            │
┌───────────────┐├───────┬───────┬───────┤├───────┬───────┬───────┤┌──────┐┌─────────┐┌─────────┐
│name           ││min    │avg    │max    ││min    │avg    │max    ││stable││partition││adaptive │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│fluxsort       ││n      │n log n│n log n││1      │n      │n      ││yes   ││yes      ││yes      │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│quadsort       ││n      │n log n│n log n││1      │n      │n      ││yes   ││no       ││yes      │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│quicksort      ││n log n│n log n│n²     ││1      │1      │1      ││no    ││yes      ││no       │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│crumsort       ││n      │n log n│n log n││1      │1      │1      ││no    ││yes      ││yes      │
└───────────────┘└───────┴───────┴───────┘└───────┴───────┴───────┘└──────┘└─────────┘└─────────┘
```

Benchmarks
----------

The following benchmark was on WSL gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04) using the [wolfsort](https://github.com/scandum/wolfsort) benchmark.
The source code was compiled using g++ -O3 -w -fpermissive bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for fluxsort, crumsort and pdqsort are inlined.

![fluxsort vs crumsort vs pdqsort](https://github.com/scandum/crumsort/blob/main/images/graph1.png)

<details><summary>data table</summary>
        
|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|  fluxsort |   100000 |   64 | 0.001922 | 0.001952 |         1 |     100 |     random order |
|  crumsort |   100000 |   64 | 0.001873 | 0.001946 |         1 |     100 |     random order |
|   pdqsort |   100000 |   64 | 0.002653 | 0.002675 |         1 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|  fluxsort |   100000 |   32 | 0.001836 | 0.001855 |         1 |     100 |     random order |
|  crumsort |   100000 |   32 | 0.001819 | 0.001870 |         1 |     100 |     random order |
|   pdqsort |   100000 |   32 | 0.002674 | 0.002699 |         1 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000675 | 0.000689 |         1 |     100 |     random % 100 |
|  crumsort |   100000 |   32 | 0.000624 | 0.000689 |         1 |     100 |     random % 100 |
|   pdqsort |   100000 |   32 | 0.000782 | 0.000788 |         1 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000046 | 0.000047 |         1 |     100 |  ascending order |
|  crumsort |   100000 |   32 | 0.000046 | 0.000046 |         1 |     100 |  ascending order |
|   pdqsort |   100000 |   32 | 0.000091 | 0.000091 |         1 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000812 | 0.000820 |         1 |     100 |    ascending saw |
|  crumsort |   100000 |   32 | 0.000991 | 0.001000 |         1 |     100 |    ascending saw |
|   pdqsort |   100000 |   32 | 0.003391 | 0.003416 |         1 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000370 | 0.000377 |         1 |     100 |       pipe organ |
|  crumsort |   100000 |   32 | 0.000481 | 0.000486 |         1 |     100 |       pipe organ |
|   pdqsort |   100000 |   32 | 0.002824 | 0.002849 |         1 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000057 | 0.000057 |         1 |     100 | descending order |
|  crumsort |   100000 |   32 | 0.000057 | 0.000061 |         1 |     100 | descending order |
|   pdqsort |   100000 |   32 | 0.000194 | 0.000198 |         1 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000814 | 0.000830 |         1 |     100 |   descending saw |
|  crumsort |   100000 |   32 | 0.000993 | 0.000998 |         1 |     100 |   descending saw |
|   pdqsort |   100000 |   32 | 0.003367 | 0.003385 |         1 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000930 | 0.000940 |         1 |     100 |      random tail |
|  crumsort |   100000 |   32 | 0.001251 | 0.001258 |         1 |     100 |      random tail |
|   pdqsort |   100000 |   32 | 0.002545 | 0.002558 |         1 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.001627 | 0.001636 |         1 |     100 |      random half |
|  crumsort |   100000 |   32 | 0.001869 | 0.001975 |         1 |     100 |      random half |
|   pdqsort |   100000 |   32 | 0.002676 | 0.002720 |         1 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000334 | 0.000339 |         1 |     100 |  ascending tiles |
|  crumsort |   100000 |   32 | 0.001292 | 0.001388 |         1 |     100 |  ascending tiles |
|   pdqsort |   100000 |   32 | 0.002303 | 0.002344 |         1 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.001664 | 0.001692 |         1 |     100 |     bit reversal |
|  crumsort |   100000 |   32 | 0.001793 | 0.001859 |         1 |     100 |     bit reversal |
|   pdqsort |   100000 |   32 | 0.002656 | 0.002683 |         1 |     100 |     bit reversal |
</details>

![fluxsort vs crumsort vs pdqsort](https://github.com/scandum/crumsort/blob/main/images/graph2.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|  fluxsort | 10000000 |   32 | 0.260900 | 0.267100 |         1 |     100 |  random 10000000 |
|  crumsort | 10000000 |   32 | 0.237100 | 0.239200 |         1 |     100 |  random 10000000 |
|   pdqsort | 10000000 |   32 | 0.337900 | 0.338700 |         1 |     100 |  random 10000000 |
|           |          |      |          |          |           |         |                  |
|  fluxsort |  1000000 |   32 | 0.212900 | 0.214900 |        10 |     100 |   random 1000000 |
|  crumsort |  1000000 |   32 | 0.208700 | 0.213500 |        10 |     100 |   random 1000000 |
|   pdqsort |  1000000 |   32 | 0.302600 | 0.304600 |        10 |     100 |   random 1000000 |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.183000 | 0.184400 |       100 |    1000 |    random 100000 |
|  crumsort |   100000 |   32 | 0.180400 | 0.190400 |       100 |    1000 |    random 100000 |
|   pdqsort |   100000 |   32 | 0.267200 | 0.269000 |       100 |    1000 |    random 100000 |
|           |          |      |          |          |           |         |                  |
|  fluxsort |    10000 |   32 | 0.156300 | 0.157000 |      1000 |    1000 |     random 10000 |
|  crumsort |    10000 |   32 | 0.158700 | 0.159300 |      1000 |    1000 |     random 10000 |
|   pdqsort |    10000 |   32 | 0.236500 | 0.237800 |      1000 |    1000 |     random 10000 |
|           |          |      |          |          |           |         |                  |
|  fluxsort |     1000 |   32 | 0.131400 | 0.132100 |     10000 |   10000 |      random 1000 |
|  crumsort |     1000 |   32 | 0.128500 | 0.129200 |     10000 |   10000 |      random 1000 |
|   pdqsort |     1000 |   32 | 0.204200 | 0.205900 |     10000 |   10000 |      random 1000 |
|           |          |      |          |          |           |         |                  |
|  fluxsort |      100 |   32 | 0.105000 | 0.105400 |    100000 |    1000 |       random 100 |
|  crumsort |      100 |   32 | 0.100100 | 0.100600 |    100000 |    1000 |       random 100 |
|   pdqsort |      100 |   32 | 0.167300 | 0.168600 |    100000 |    1000 |       random 100 |
|           |          |      |          |          |           |         |                  |
|  fluxsort |       10 |   32 | 0.044500 | 0.045200 |   1000000 |    1000 |        random 10 |
|  crumsort |       10 |   32 | 0.046900 | 0.047600 |   1000000 |    1000 |        random 10 |
|   pdqsort |       10 |   32 | 0.089700 | 0.091500 |   1000000 |    1000 |        random 10 |

</details>
