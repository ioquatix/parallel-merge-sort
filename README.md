# Dictionary Sort

This program implements a dictionary sort where the alphabet can be manually specified. The code depends on C++11 standard, primarily `<thread>`, which could be substituted for `<boost/thread>` if available. Therefore, you need to use latest Clang 3.0+ that supports C++11, along with libc++ (rather than libstdc++).

The main testing program is in main.cpp which tests the performance of the dictionary sort several times and prints out the average time.

The dictionary sort algorithm requires a list of letters in ascending order. The order of these letters defines the lexicographical order of sorted words. e.g.

        "abc" will sort the word "apple" before "bananna"
        "cba" will sort the word "cucumber" before "bandanna"

The dictionary sort algorithm can use either `std::sort` or `ParallelMergeSort::sort`. To change the behaviour, at the top of `DictionarySort.h`, change the constant `SORT_MODE`, details are in the comments at that point.

The parallel merge sort algorithm can be distributed over a number of processors in a shared memory architecture machine. The default merge sort splits the data to be sorted into two pieces, and sorts each side independently. The data is then merged back together. In this case, the parallel merge sort splits at n top levels of the tree, such that

    n = 0, sequential implementation
    n = 1, split at top level, threads = 2 for partition, 2 for merge, armortised total = 2.
    n = 2, split at top level and one level down, threads = 6
    n = 3, split at top level, ..., threads = 14
    n = 4, ..., threads = 22
	
	or generally, 2^n+1 - 2

So without automatically detecting the number of processors, the greatest gains can be made by setting n = 1...3, typically in the range to 2x to 3x the performance over n = 0 and `std::sort`.

## Author's Benchmarks

These benchmarks were performed on a Intel Core i7 2.3Ghz with 16GB main memory and a solid state disk.

Sorting using builtin `std::sort`:

	Sorting 2500000 words...
	Sort mode = -1
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 1.86927
		* Processor sort time: 1.8692
		* Approximate processor usage: 0.999965
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 1.8631
		* Processor sort time: 1.86307
		* Approximate processor usage: 0.999982
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 1.93701
		* Processor sort time: 1.93679
		* Approximate processor usage: 0.999885
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 1.91942
		* Processor sort time: 1.9193
		* Approximate processor usage: 0.999941
	Checksum: 479465310674138860 ? 1
	Total Time: 3.43858
	Finished.

Next is using my parallel merge sort with a single processor (i.e not parallel at all) - I believe it should be possible for my implementation to improve on the single-processor performance similar to `std::sort` but I need to implement some pretty gnarly optimisations. However it potentially should be a big pay of, for example to find sorted sub-sequences, using hand optimised sorting networks for < 8 items, etc.

	Sorting 2500000 words...
	Sort mode = 0
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 2.12582
		* Processor sort time: 2.12572
		* Approximate processor usage: 0.999951
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 2.27442
		* Processor sort time: 2.27399
		* Approximate processor usage: 0.999811
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 2.14226
		* Processor sort time: 2.14216
		* Approximate processor usage: 0.999954
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 2.08873
		* Processor sort time: 2.08866
		* Approximate processor usage: 0.999966
	Checksum: 479465310674138860 ? 1
	Total Time: 3.67613
	Finished.

Now the magic happens when we enable  Sort mode = 1 - we have almost 2x performance, using 2x processors.

	Sorting 2500000 words...
	Sort mode = 1
	Parallel merge thread count: 2
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 1.19818
		* Processor sort time: 2.35019
		* Approximate processor usage: 1.96148
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 1.19826
		* Processor sort time: 2.35665
		* Approximate processor usage: 1.96673
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 1.2095
		* Processor sort time: 2.38227
		* Approximate processor usage: 1.96963
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 1.20012
		* Processor sort time: 2.36316
		* Approximate processor usage: 1.96911
	Checksum: 479465310674138860 ? 1
	Total Time: 2.79799
	Finished.

Now, even more magic, sort mode = 2, theoretically, 6 threads total, but an approximate processor usage of 4 threads total, in practice, 3.5x single processor usage, gives us a speed gain over single processor of 2.1/0.7 ≈ 2.65 times.

	Sorting 2500000 words...
	Sort mode = 2
	Parallel merge thread count: 6
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 0.81971
		* Processor sort time: 2.82076
		* Approximate processor usage: 3.44117
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 0.854406
		* Processor sort time: 2.97835
		* Approximate processor usage: 3.48587
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 0.816546
		* Processor sort time: 2.89088
		* Approximate processor usage: 3.54038
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 0.834062
		* Processor sort time: 2.9364
		* Approximate processor usage: 3.5206
	Checksum: 479465310674138860 ? 1
	Total Time: 2.47451
	Finished.

Unfortunately, we do hit scalability issues:

	Sorting 2500000 words...
	Sort mode = 3
	Parallel merge thread count: 14
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 0.68632
		* Processor sort time: 4.18659
		* Approximate processor usage: 6.10005
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 0.703832
		* Processor sort time: 4.15296
		* Approximate processor usage: 5.9005
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 0.683881
		* Processor sort time: 4.17384
		* Approximate processor usage: 6.10317
	--- Completed Dictionary Sort ---
		* Dictionary sort time: 0.722513
		* Processor sort time: 4.09178
		* Approximate processor usage: 5.66327
	Checksum: 479465310674138860 ? 1
	Total Time: 2.40132
	Finished.

We are using in practice 6x the single processor usage, for a speedup of 2.1/0.7 ≈ 3 times. Another problem is that my processor only has 8 cores, and we are already using 14 threads (but only 8 should be active performing work). The main bottleneck is the top level merge, which I could only make lock free with 2 processors. It is innovative, but using locks might be preferable - it might actually be faster for the top level merge to distribute it over all available processors. Partition is already maximally distributed. But, I can't figure out how to make that merge lock free and the novelty of this approach is that it is lock free.

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

## License

Released under the MIT license.

Copyright, 2012, by [Samuel G. D. Williams](http://www.codeotaku.com/samuel-williams).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
