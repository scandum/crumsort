Intro
-----
This document describes a hybrid quicksort / mergesort named crumsort. The sort is in-place, unstable, adaptive, branchless, and has exceptional performance.

Analyzer
--------
Crumsort starts out with an analyzer that sorts fully in-order or reverse-order arrays using n comparisons. It also obtains a measure of presortedness for 4 segments of the array and switches to [quadsort](https://github.com/scandum/quadsort) if a segment is more than 50% ordered.

Partitioning
------------
Partitioning is performed in a top-down manner similar to quicksort. Crumsort obtains the pseudomedian of 9 for partitions smaller than 2048 elements, and the median of 16 for paritions smaller than 65536. For larger partitions crumsort obtains the median of 128, 256, or 512 as an approximation of the cubic root of the partition size. While the square root is optimal in theory, the law of diminishing returns appears to apply to increasing the number of pivot candidates. Hardware limitations need to be factored in as well.

For large partitions crumsort will swap 128-512 random elements to the start of the array, sort them with quadsort, and take the center right element. Using pseudomedian instead of median selection on large arrays is slower, likely due to cache pollution.

The median element obtained will be referred to as the pivot. Partitions that grow smaller than 24 elements are sorted with quadsort.

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
                if (array[tail] > pivot)
                {
                        tail--;
                        continue;
                }

                if (head >= tail)
                {
                        array[head] = pivot;
                        return head;
                }
                array[head++] = array[tail];

                while (1)
                {
                        if (head >= tail)
                        {
                                array[head] = pivot;
                                return head;
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
Instead of using multiple swaps the fulcrum partition creates a 1 element swap space, with the pivot holding the original data. Doing so turns the 3 assignments from the swap into 2 assignments. Overall the fulcrum partition has a 10-20% performance improvement.

The swap space of the fulcrum partition can easily be increased from 1 to 32 elements to allow it to perform 16 boundless comparisons at a time, which in turn also allows it to perform these comparisons in a branchless manner with little additional overhead.

Worst case handling
-------------------
To avoid run-away recursion crumsort switches to quadsort for both partitions if one partition is less than 1/16th the size of the other partition. On a distribution of random unique values the observed chance of a false positive is 1 in 1,336 for the pseudomedian of 9 and approximately 1 in 500,000 for the median of 16.

Combined with the analyzer crumsort starts out with this makes the existence of killer patterns unlikely, other than a 33-50% performance slowdown by prematurely triggering the use of quadsort.

Branchless optimizations
------------------------
Crumsort uses a branchless comparison optimization. The ability of quicksort to partition branchless was first described in "BlockQuicksort: How Branch Mispredictions don't affect Quicksort" by Stefan Edelkamp and Armin Weiss. Crumsort uses the fulcrum partitioning scheme where as BlockQuicksort uses a scheme resembling Hoare partitioning.

Median selection uses a branchless comparison technique that selects the pseudomedian of 9 using 12 comparisons, and the pseudomedian of 25 using 42 comparisons.

These optimizations do not work as well when the comparisons themselves are branched and the largest performance increase is on 32 and 64 bit integers.

Generic data optimizations
--------------------------
Crumsort uses a method that mimicks dual-pivot quicksort to improve generic data handling. If after a partition all elements were smaller or equal to the pivot, a reverse partition is performed, filtering out all elements equal to the pivot, next it carries on as usual. This typically only occurs when sorting tables with many identical values, like gender, age, etc. Crumsort has a small bias in its pivot selection to increase the odds of this happening. In addition, generic data performance is improved slightly by checking if the same pivot is chosen twice in a row, in which case it performs a reverse partition as well. Pivot retention was first introduced by [pdqsort](https://github.com/orlp/pdqsort).

Small array optimizations
-------------------------
Most modern quicksorts use insertion sort for partitions smaller than 24 elements. Crumsort uses quadsort which has a dedicated small array sorting routine that outperforms insertion sort.

Data Types
----------
The C implementation of crumsort supports long doubles and 8, 16, 32, and 64 bit data types. By using pointers it's possible to sort any other data type, like strings.

Interface
---------
Crumsort uses the same interface as qsort, which is described in [man qsort](https://man7.org/linux/man-pages/man3/qsort.3p.html).

Crumsort comes with the `crumsort_prim(void *array, size_t nmemb, size_t size)` function to perform primitive comparisons on arrays of 32 and 64 bit integers. Nmemb is the number of elements. Size should be either sizeof(int) or sizeof(long long) for signed integers, and sizeof(int) + 1 or sizeof(long long) + 1 for unsigned integers. Support for additional primitive as well as custom types can be added to fluxsort.h and quadsort.h.

Porting
-------
People wanting to port crumsort might want to have a look at [fluxsort](https://github.com/scandum/fluxsort), which is a little bit simpler because it's stable and out of place. There's also [piposort](https://github.com/scandum/piposort), a simplified implementation of quadsort.

Memory
------
Crumsort uses 512 elements of stack memory, which is shared with quadsort. Recursion requires log n stack memory.

Crumsort can be configured to use sqrt(n) memory, with a minimum memory requirement of 32 elements.

Performance
-----------
Crumsort will begin to outperform fluxsort on random data right around 1,000,000 elements. Since it runs on 512 elements of auxiliary memory the sorting of ordered data will be slower than fluxsort for larger arrays.

Crumsort being unstable will scramble pre-existing patterns, making it less adaptive than fluxsort, which will switch to quadsort when it detects the emergence of ordered data during the partitioning phase.

Because of the partitioning scheme crumsort is slower than pdqsort when sorting arrays of long doubles. Fixing this is on my todo list and I've devised a scheme to do so. The main focus of crumsort is the sorting of tables however, and crumsort will outperform pdqsort when long doubles are embedded within a table. In this case crumsort only has to move a 64 bit pointer, instead of a 128 bit float.

To take full advantage of branchless operations the cmp macro needs to be uncommented in bench.c, which will increase the performance by 100% on primitive types. The crumsort_prim() function can be used to access primitive comparisons directly. In the case of 64 bit integers crumsort will outperform all radix sorts I've tested so far. Radix sorts still hold an advantage for 32 bit integers on arrays under 1 million elements.

Big O
-----
```
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

Variants
--------
- [crumsort-rs](https://github.com/google/crumsort-rs) is a parallelized Rust port of crumsort with a focus on random data.

- [distcrum](https://github.com/mlochbaum/distcrum) is a crumsort / [rhsort](https://github.com/mlochbaum/rhsort) hybrid.

Visualization
-------------
In the visualization below two tests are performed on 512 elements.

1. Random order
2. Random % 10

The upper half shows the swap memory (32 elements) and the bottom half shows the main memory.
Colors are used to differentiate various operations.

[![crumsort benchmark](/images/crumsort.gif)](https://www.youtube.com/watch?v=NRREkZeNaC4)

Benchmarks
----------

The following benchmark was on WSL gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04) using the [wolfsort](https://github.com/scandum/wolfsort) benchmark.
The source code was compiled using g++ -O3 -w -fpermissive bench.c. Each test was ran 100 times on 100,000 elements. A table with the best and average time in seconds can be uncollapsed below the bar graph. Comparisons for fluxsort, crumsort and pdqsort are inlined.

![fluxsort vs crumsort vs pdqsort](https://github.com/scandum/crumsort/blob/main/images/graph1.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |   100000 |  128 | 0.005908 | 0.005975 |         0 |     100 |     random order |
|  crumsort |   100000 |  128 | 0.008262 | 0.008316 |         0 |     100 |     random order |
|  fluxsort |   100000 |  128 | 0.008567 | 0.008676 |         0 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |   100000 |   64 | 0.002645 | 0.002668 |         0 |     100 |     random order |
|  crumsort |   100000 |   64 | 0.001896 | 0.001927 |         0 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.001942 | 0.001965 |         0 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |   100000 |   32 | 0.002682 | 0.002705 |         0 |     100 |     random order |
|  crumsort |   100000 |   32 | 0.001797 | 0.001812 |         0 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.001834 | 0.001858 |         0 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000801 | 0.000807 |         0 |     100 |     random % 100 |
|  crumsort |   100000 |   32 | 0.000560 | 0.000575 |         0 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.000657 | 0.000670 |         0 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000098 | 0.000099 |         0 |     100 |  ascending order |
|  crumsort |   100000 |   32 | 0.000043 | 0.000046 |         0 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000044 | 0.000044 |         0 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.003460 | 0.003483 |         0 |     100 |    ascending saw |
|  crumsort |   100000 |   32 | 0.000628 | 0.000638 |         0 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000328 | 0.000336 |         0 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002828 | 0.002850 |         0 |     100 |       pipe organ |
|  crumsort |   100000 |   32 | 0.000359 | 0.000363 |         0 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000215 | 0.000219 |         0 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000201 | 0.000203 |         0 |     100 | descending order |
|  crumsort |   100000 |   32 | 0.000055 | 0.000055 |         0 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000055 | 0.000056 |         0 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.003229 | 0.003260 |         0 |     100 |   descending saw |
|  crumsort |   100000 |   32 | 0.000637 | 0.000645 |         0 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000328 | 0.000332 |         0 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002558 | 0.002579 |         0 |     100 |      random tail |
|  crumsort |   100000 |   32 | 0.000879 | 0.000895 |         0 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.000626 | 0.000631 |         0 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002660 | 0.002677 |         0 |     100 |      random half |
|  crumsort |   100000 |   32 | 0.001200 | 0.001207 |         0 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.001069 | 0.001084 |         0 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002310 | 0.002328 |         0 |     100 |  ascending tiles |
|  crumsort |   100000 |   32 | 0.001520 | 0.001534 |         0 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.000294 | 0.000298 |         0 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002659 | 0.002681 |         0 |     100 |     bit reversal |
|  crumsort |   100000 |   32 | 0.001787 | 0.001800 |         0 |     100 |     bit reversal |
|  fluxsort |   100000 |   32 | 0.001696 | 0.001721 |         0 |     100 |     bit reversal |

</details>

![fluxsort vs crumsort vs pdqsort](https://github.com/scandum/crumsort/blob/main/images/graph2.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |       10 |   32 | 0.087140 | 0.087436 |       0.0 |      10 |        random 10 |
|  crumsort |       10 |   32 | 0.049921 | 0.050132 |       0.0 |      10 |        random 10 |
|  fluxsort |       10 |   32 | 0.048499 | 0.048724 |       0.0 |      10 |        random 10 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |      100 |   32 | 0.169583 | 0.169940 |       0.0 |      10 |       random 100 |
|  crumsort |      100 |   32 | 0.113443 | 0.113882 |       0.0 |      10 |       random 100 |
|  fluxsort |      100 |   32 | 0.113426 | 0.113955 |       0.0 |      10 |       random 100 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |     1000 |   32 | 0.207200 | 0.207858 |       0.0 |      10 |      random 1000 |
|  crumsort |     1000 |   32 | 0.135912 | 0.136251 |       0.0 |      10 |      random 1000 |
|  fluxsort |     1000 |   32 | 0.137019 | 0.138586 |       0.0 |      10 |      random 1000 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |    10000 |   32 | 0.238297 | 0.239006 |       0.0 |      10 |     random 10000 |
|  crumsort |    10000 |   32 | 0.158249 | 0.158476 |       0.0 |      10 |     random 10000 |
|  fluxsort |    10000 |   32 | 0.158445 | 0.158694 |       0.0 |      10 |     random 10000 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.270447 | 0.270855 |       0.0 |      10 |    random 100000 |
|  crumsort |   100000 |   32 | 0.181770 | 0.183123 |       0.0 |      10 |    random 100000 |
|  fluxsort |   100000 |   32 | 0.185907 | 0.186829 |       0.0 |      10 |    random 100000 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |  1000000 |   32 | 0.303525 | 0.305467 |       0.0 |      10 |   random 1000000 |
|  crumsort |  1000000 |   32 | 0.206979 | 0.208153 |       0.0 |      10 |   random 1000000 |
|  fluxsort |  1000000 |   32 | 0.215098 | 0.216294 |       0.0 |      10 |   random 1000000 |
|           |          |      |          |          |           |         |                  |
|   pdqsort | 10000000 |   32 | 0.338767 | 0.342580 |         0 |      10 |  random 10000000 |
|  crumsort | 10000000 |   32 | 0.234268 | 0.234664 |         0 |      10 |  random 10000000 |
|  fluxsort | 10000000 |   32 | 0.264988 | 0.267283 |         0 |      10 |  random 10000000 |

</details>
