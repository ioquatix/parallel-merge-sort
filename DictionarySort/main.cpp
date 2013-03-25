//
//  main.cpp
//  DictionarySort
//
//  Created by Samuel Williams on 31/10/11.
//  Copyright (c) 2011 Orion Transfer Ltd. All rights reserved.
//

#include <iostream>

#include "Benchmark.h"
#include "DictionarySort.h"

// Print out vectors using a simple [item0, item1, ... itemn] format.
template <typename AnyT>
std::ostream& operator<< (std::ostream &o, const std::vector<AnyT> & v)
{
    bool first = true;
    
    o << "[";
    for (typename std::vector<AnyT>::const_iterator i = v.begin(); i != v.end(); ++i) {
        if (first)
            first = false;
        else
            o << ", ";
        
        o << *i;
    }
    o << "]";
    
    return o;
}

static void test_parallel_merge ()
{
    typedef std::vector<long long> ArrayT;
    typedef std::less<long long> ComparatorT;
    ComparatorT comparator;

    const long long data[] = {
        2, 4, 6, 8, 12,
        1, 3, 5, 10, 11
    };
    
    ArrayT a(data, data+(sizeof(data)/sizeof(*data)));
    ArrayT b(a.size());
    
    ParallelMergeSort::ParallelLeftMerge<ArrayT, ComparatorT> left_merge = {a, b, comparator, 0, a.size() / 2};
    left_merge();
    
    std::cout << "After Left: " << b << std::endl;
    
    ParallelMergeSort::ParallelRightMerge<ArrayT, ComparatorT> right_merge = {a, b, comparator, 0, a.size() / 2, a.size()};
    right_merge();
    
    std::cout << "After Right: " << b << std::endl;
}

static void test_sort ()
{
    typedef std::vector<long long> ArrayT;
    typedef std::less<long long> ComparatorT;
    ComparatorT comparator;
    
    const long long data[] = {
        11, 2, 4, 6, 8, 10, 12, 1, 3, 5, 7, 9, 13
    };
    
    std::vector<long long> v(data, data+(sizeof(data)/sizeof(*data)));
    
    std::cerr << "Sorting " << v << std::endl;

    ParallelMergeSort::sort(v, comparator, 0);
    
    std::cerr << "Sorted  " << v << std::endl;   
}

static void test_dictionary ()
{
    // This defines a dictionary based on ASCII characters.
    typedef DictionarySort::Dictionary<char, DictionarySort::IndexT[256]> ASCIIDictionaryT;
    
    // For unicode characters, you could use something like this:
    // typedef DictionarySort::Dictionary<uint32_t, std::map<uint32_t, DictionarySort::IndexT>> UCS32DictionaryT;
    // Be aware that 
    
    std::string s = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";
    ASCIIDictionaryT::WordT alphabet(s.begin(), s.end());
    ASCIIDictionaryT dictionary(alphabet);
    
    ASCIIDictionaryT::WordsT words, sorted_words;
    const std::size_t MAX_LENGTH = 25;
    const std::size_t MAX_COUNT = 2500000;
    for (std::size_t i = 0; i < MAX_COUNT; i += 1) {
        ASCIIDictionaryT::WordT word;
        for (std::size_t j = i; (j-i) <= (i ^ (i * 21)) % MAX_LENGTH; j += 1) {
            word.push_back(alphabet[(j ^ (j << (i % 4))) % alphabet.size()]);
        }
        words.push_back(word);
    }
        
    std::cerr << "Sorting " << words.size() << " words..." << std::endl;
    
    const int K = 5;
    Benchmark::Timer t;
    uint64_t checksum;
    for (std::size_t i = 0; i < K; i += 1) {
        checksum = dictionary.sort(words, sorted_words);
    }
    Benchmark::TimeT elapsed_time = t.time();
    
    std::cerr << "Checksum: " << checksum << " ? " << (checksum == 479465310674138860) << std::endl;
    std::cerr << "Time: " << (elapsed_time / K) << std::endl;
        
    std::cerr << "Finished." << std::endl;
}

int main (int argc, const char * argv[])
{   
    //test_parallel_merge();
    //test_sort();
    test_dictionary();
    
    return 0;
}

