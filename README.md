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
