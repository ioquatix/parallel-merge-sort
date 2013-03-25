//
//  ParallelMergeSort.h
//  DictionarySort
//
//  Created by Samuel Williams on 2/11/11.
//  Copyright (c) 2011 Orion Transfer Ltd. All rights reserved.
//

#ifndef DictionarySort_ParallelMergeSort_h
#define DictionarySort_ParallelMergeSort_h

#include <thread>
#include "Benchmark.h"

// A parallel merge sort algorithm template implemented using C++0x11 threads.
namespace ParallelMergeSort {        
    /** Parallel Merge Algorithm.
     
         This parallel merge algorithm uses two threads and requires no synchrnoisation (e.g. lock free).
         
         Given two sorted sequences i and j, such that |i| == |j| or |i| == |j|-1, we can merge these together by taking the |i| smallest items and |j| biggest items independently. The final sorted list q which consists of all items from i and j in order, has a basic property such that the lower |i| items in q and the upper |j| items in q are mutually exclusive. Therefore, we can select each half of the list independently:
         
            ij = [1, 3, 5, 2, 4, 6]
                i = [1, 3, 5]
                j = [2, 4, 6]

            q = [1, 2, 3, 4, 5, 6]
         
         In this case, we can see that q[0,3] can be formed by merging the first 3 smallest items, and q[3,6] can be formed by merging the largest 3 items. Because these are mutually exclusive, this process can be done on two threads.
         
         Other merging algorithms exist, but may require locking. Another approach worth exploring would be to form in parallel n heaps, where all items heap[k] < heap[k+1]. If the heaps can be constructed in sorted order, the destination array will naturally contain the final sorted list.
     
     */
    
    // This implementation assumes that if there are |i| items on the left side, there must be at least |i| items on the right side.
    template <typename ArrayT, typename ComparatorT>
    struct ParallelLeftMerge {
        ArrayT & source, & destination;
        const ComparatorT & comparator;
        std::size_t lower_bound, middle_bound;
        
        void operator()() {
            std::size_t left = lower_bound;
            std::size_t right = middle_bound;
            std::size_t offset = lower_bound;
            
            while (offset < middle_bound) {
                if (comparator(source[left], source[right])) {
                    destination[offset++] = source[left++];
                } else {
                    destination[offset++] = source[right++];
                }
            }
        }
    };
    
    // This implementation assumes that if there are |j| items on the right side, there are at least |j| - 1 items on the left side.
    template <typename ArrayT, typename ComparatorT>
    struct ParallelRightMerge {
        ArrayT & source, & destination;
        const ComparatorT & comparator;
        std::size_t lower_bound, middle_bound, upper_bound;
        
        void operator()() {
            std::size_t left = middle_bound-1;
            std::size_t right = upper_bound-1;
            std::size_t offset = upper_bound-1;
            
            while (offset >= middle_bound) {
                if (comparator(source[left], source[right])) {
                    destination[offset--] = source[right--];
                } else {
                    destination[offset--] = source[left--];
                    if (left == lower_bound) {
                        // There are no more items on left hand side - in this case, there is at most one more item on right side to copy.
                        if (offset >= middle_bound) {
                            destination[offset] = source[right];
                        }
                        
                        break;
                    }
                }
            }
        }
    };
    
    // Merge two sorted sub-sequences sequentially (from left to right).
    template <typename ArrayT, typename ComparatorT>
    void merge (ArrayT & source, ArrayT & destination, const ComparatorT & comparator, std::size_t lower_bound, std::size_t middle_bound, std::size_t upper_bound) {
        std::size_t left = lower_bound;
        std::size_t right = middle_bound;
        std::size_t offset = lower_bound;
        
        // We merge both sub-sequences, defined as [lower_bound, middle_bound] and [middle_bound, upper_bound].
        while (true) {
            if (comparator(source[left], source[right])) {
                destination[offset++] = source[left++];
                
                // If we have adjusted left, we may have exhausted left side:
                if (left == middle_bound) {
                    // We have no more elements in lower half.
                    std::copy(source.begin() + right, source.begin() + upper_bound, destination.begin() + offset);
                    break;
                }
            } else {
                destination[offset++] = source[right++];
                
                // As above, we may have exhausted right side:
                if (right == upper_bound) {
                    // We have no more elements in upper half.
                    std::copy(source.begin() + left, source.begin() + middle_bound, destination.begin() + offset);
                    break;
                }
            }
        }        
    }
    
    template <typename ArrayT, typename ComparatorT>
    void partition(ArrayT & array, ArrayT & temporary, const ComparatorT & comparator, std::size_t lower_bound, std::size_t upper_bound, std::size_t threaded);
    
    // This functor is used for parallelizing the top level partition function.
    template <typename ArrayT, typename ComparatorT>
    struct ParallelPartition {
        ArrayT & array, & temporary;
        const ComparatorT & comparator;
        std::size_t lower_bound, upper_bound, threaded;
        
        void operator()() {
            partition(array, temporary, comparator, lower_bound, upper_bound, threaded);
        }
    };
    
    /** Recursive Partition Algorithm.
     
        This algorithm uses O(2n) memory to reduce the amount of copies that occurs. It does this by using a parity such that at each point in the partition tree we provide a source and destination. Given the functions P (partition) and M (merge), we have the following theorem:
        
        P(A=source, B=destination) sorts source into destination. A=[...] means that we are considering only a subset of A. Subscript is not given, but should be intuitive given the definition of merge sort. (x) on the left gives the order of each step as performed sequentially.
     
                == [ PARTITION ] ==                     == [ MERGE ] ==
         
            (1) P(A=[1,3,4,2], B=[1,3,2,4])        (14) M(A=[1,3,2,4], B): B = [1,2,3,4]
                |
            (2) |---P(B=[1,3], A=[1,3])             (7) M(B=[1,3], A): A=[1,3]
                |   |
            (3) |   |---P(A=[1], B=[1])             (4) M(A=[1], B): B=[1]
            (5) |   \---P(A=[3], B=[3])             (6) M(A=[3], B): B=[3]
                |
            (8) \---P(B=[4,2], A=[4,2])            (13) M(B=[4,2], A): A = [2,4]
                    |
            (9)     |---P(A=[4],B=[4])             (10) M(A=[4], B): B=[4]
           (11)     \---P(A=[2],B=[2])             (12) M(A=[2], B): B=[2]
         
         During merge, we fold back up, and alternate between A and B for the current storage. This avoids the need to dynamically allocate memory during sort and avoids unnecessary copies.
     
     */
    
    // Sequential partition algorithm. Provide an array, and an upper and lower bound to sort.
    template <typename ArrayT, typename ComparatorT>
    void partition(ArrayT & source, ArrayT & destination, const ComparatorT & comparator, const std::size_t & lower_bound, const std::size_t & upper_bound) {
        std::size_t count = upper_bound - lower_bound;
        
        // In the case where count == 1, we are at the very bottom of the tree and both source and destination will be the same.
        // The same applies when count == 2, but we might need to swap the items around if they are not in the right order.
        if (count == 2) {
            if (!comparator(destination[lower_bound], destination[lower_bound+1])) {
                std::swap(destination[lower_bound], destination[lower_bound+1]);
            }
            // After this point, where count > 2, source and destination are different.
        } else if (count > 2) {
            std::size_t middle_bound = (lower_bound + upper_bound) / 2;
                        
            // While it is possible to simply call partition, we try to avoid recursion by folding up the bottom two cases:
            //  (count == 1), do nothing
            //  (count == 2), swap if order is not correct
            //  (count > 2), partition            
            // After profilling, I found that the benefit of unrolling (count == 2) was minimal - there was about a 2-3% improvement.

            std::size_t lower_count = middle_bound - lower_bound;
            if (lower_count > 1)
                partition(destination, source, comparator, lower_bound, middle_bound);
            
            std::size_t upper_count = upper_bound - middle_bound;            
            if (upper_count > 1)
                partition(destination, source, comparator, middle_bound, upper_bound);
            
            merge(source, destination, comparator, lower_bound, middle_bound, upper_bound);
        }
    }
    
    /** Parallel Partition Algorithm
     
        This parallel partition algorithm which controls the downward descent of the merge sort algorithm is designed for large datasets. Because merge sort follows a binary tree structure, the work is essentially split between two threads at each node in the tree. Firstly, we must recursively call partition on two separate threads. Once this is done, we have two ascending sequences, and we merge these together, again using two threads, one for left sequence and one for right sequence.
     
        Because higher level threads will be waiting on lower level threads, the value of threaded should be equal to 2^threaded == processors for best performance.
     
     */
    
    // Use this to control whether parallal partition is used.
    // For large data sets > 500_000 items, you will see an improvement of about ~50% per thread.
    const bool PARALLEL_PARTITION = true;
    
    // Use this to control whether parallel merge is used. 
    // For large data sets > 1_000_000 items, you will see an improvement of about 15%.
    const bool PARALLEL_MERGE = true;
    
    // If you make this number too small, e.g. <= 2, you may cause synchronsation issues, because you will force parallelisation
    // for base cases which actually need to be sequential to ensure that comparison cache is generated correctly.
    const std::size_t PARALLEL_MERGE_MINIMUM_COUNT = 128;
    
    // Provide an array, and an upper and lower bound, along with the number of threads to use.
    template <typename ArrayT, typename ComparatorT>
    void partition(ArrayT & source, ArrayT & destination, const ComparatorT & comparator, std::size_t lower_bound, std::size_t upper_bound, std::size_t threaded) {
        std::size_t count = upper_bound - lower_bound;
        
        if (count > 1) {
            std::size_t middle_bound = (lower_bound + upper_bound) / 2;
            
            //Benchmark::Timer tp;
            if (PARALLEL_PARTITION && threaded > 0) {
                // We could check whether there is any work to do before creating threads, but we assume
                // that threads will only be created high up in the tree by default, so there *should*
                // be a significant work available per-thread.
                ParallelPartition<ArrayT, ComparatorT> 
                    lower_partition = {destination, source, comparator, lower_bound, middle_bound, threaded - 1}, 
                    upper_partition = {destination, source, comparator, middle_bound, upper_bound, threaded - 1};
                
                std::thread 
                    lower_thread(lower_partition),
                    upper_thread(upper_partition);
                
                upper_thread.join();
                lower_thread.join();                
            } else {
                // We have hit the bottom of our thread limit.
                partition(destination, source, comparator, lower_bound, middle_bound);
                partition(destination, source, comparator, middle_bound, upper_bound);
            }
            //std::cerr << "Partition Time: " << tp.time() << " [" << lower_bound << " -> " << upper_bound << " : " << threaded << " ]" << std::endl;
            
            //Benchmark::Timer tm;
            if (PARALLEL_MERGE && threaded > 0 && count > PARALLEL_MERGE_MINIMUM_COUNT) {
                // By the time we get here, we are sure that both left and right partitions have been merged, e.g. we have two ordered sequences [lower_bound, middle_bound] and [middle_bound, upper_bound]. Now, we need to join them together:
                ParallelLeftMerge<ArrayT, ComparatorT> left_merge = {source, destination, comparator, lower_bound, middle_bound};
                ParallelRightMerge<ArrayT, ComparatorT> right_merge = {source, destination, comparator, lower_bound, middle_bound, upper_bound};
                
                std::thread
                    left_thread(left_merge),
                    right_thread(right_merge);
                
                left_thread.join();
                right_thread.join();
            } else {
                // We have hit the bottom of our thread limit, or the merge minimum count.
                merge(source, destination, comparator, lower_bound, middle_bound, upper_bound);
            }
            //std::cerr << "Merge Time: " << tm.time() << " [" << lower_bound << " -> " << upper_bound << " : " << threaded << " ]" << std::endl;
        }
    }
    
    /** Parallel Merge Sort, main entry point.
     
        Given an array of items, a comparator functor, use at most 2^threaded threads to sort the items.   
     
     */
    template <typename ArrayT, typename ComparatorT>
    void sort(ArrayT & array, const ComparatorT & comparator, std::size_t threaded = 2) {
        ArrayT temporary(array.begin(), array.end());
        
        //Benchmark::Timer ts;
        if (threaded == 0)
            partition(temporary, array, comparator, 0, array.size());
        else
            partition(temporary, array, comparator, 0, array.size(), threaded);
        //std::cerr << "Total sort time: " << ts.time() << std::endl;
    }
}


#endif
