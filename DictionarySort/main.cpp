//
//  main.cpp
//  DictionarySort
//
//  Created by Samuel Williams on 31/10/11.
//  Copyright (c) 2011 Orion Transfer Ltd. All rights reserved.
//

#include <iostream>

#include <cmath>
#include <vector>
#include <thread>
#include <map>
#include <memory>
#include <fstream>

#include <sys/time.h>

// A timer class for quickly checking the wall-clock performance of code.
namespace Benchmark {
    typedef double TimeT;

    class Timer {
    protected:
        mutable TimeT m_last, m_total;
        
    public:	
        Timer ();
        virtual ~Timer ();
        
        virtual void reset ();
        virtual TimeT time () const;
    };

    TimeT systemTime () {
        static struct timeval t;
        gettimeofday (&t, (struct timezone*)0);
        return ((TimeT)t.tv_sec) + ((TimeT)t.tv_usec / 1000000.0);
    }

    Timer::Timer () {
        this->reset();
    }

    Timer::~Timer () {
        
    }

    void Timer::reset () {
        this->m_last = systemTime();
        this->m_total = 0.0;
    }

    TimeT Timer::time () const {
        TimeT current = systemTime();
        this->m_total += current - this->m_last;
        this->m_last = current;
        return this->m_total;
    }
}

// Print out vectors using a simple {item0, item1, ... itemn} format.
template <typename AnyT>
std::ostream& operator<< (std::ostream &o, const std::vector<AnyT> & v)
{
    bool first = true;
    
    o << "{";
    for (typename std::vector<AnyT>::const_iterator i = v.begin(); i != v.end(); ++i) {
        if (first)
            first = false;
        else
            o << ", ";
        
        o << *i;
    }
    o << "}";
    
    return o;
}

template <typename AnyT>
struct pointer_less_than
{
    bool operator()(const AnyT *a, const AnyT * b) {
        return *a < *b;
    }
};

// A parallel merge sort algorithm template implemented using C++0x11 threads.
namespace ParallelMergeSort {
    template <typename ArrayT, typename ComparatorT>
    void partition(ArrayT & array, ArrayT & temporary, ComparatorT & comparator, std::size_t lowerBound, std::size_t upperBound, std::size_t threaded);
    
    template <typename ArrayT, typename ComparatorT>
    void merge (ArrayT & array, ArrayT & merged, ComparatorT & comparator, std::size_t lowerBound, std::size_t middleBound, std::size_t upperBound);
    
    template <typename ArrayT, typename ComparatorT>
    struct ParallelPartition {
        ArrayT & array, & temporary;
        ComparatorT & comparator;
        std::size_t lowerBound, upperBound, threaded;
        
        void operator()() {
            partition(array, temporary, comparator, lowerBound, upperBound, threaded);
        }
    };
    
    // Sequential partition algorithm. Provide an array, and an upper and lower bound to sort.
    template <typename ArrayT, typename ComparatorT>
    void partition(ArrayT & source, ArrayT & destination, ComparatorT & comparator, std::size_t lowerBound, std::size_t upperBound) {
        // At this point we are trying to optimise the linear performance,
        // so we unfold the bottom cases to avoid about half of the recursion.
        std::size_t count = upperBound - lowerBound;
        
        if (count == 0) {
            // Nothing to do here =(
            return;
        } else if (count == 1) {
            destination[lowerBound] = source[lowerBound];
        } else if (count == 2) {
            destination[lowerBound] = source[lowerBound];
            destination[lowerBound + 1] = source[lowerBound + 1];
            
            if (comparator(source[lowerBound], source[lowerBound+1])) {
                destination[lowerBound] = source[lowerBound];
                destination[lowerBound + 1] = source[lowerBound + 1];
            } else {
                destination[lowerBound] = source[lowerBound + 1];
                destination[lowerBound + 1] = source[lowerBound];
            }
        } else if (count > 1) {
            std::size_t middle = (lowerBound + upperBound) / 2;
            
            partition(destination, source, comparator, lowerBound, middle);
            partition(destination, source, comparator, middle, upperBound);
            
            merge(source, destination, comparator, lowerBound, middle, upperBound);
        }
    }
    
    template <typename ArrayT, typename ComparatorT, int DIRECTION>
    struct ParallelMerge {
        ArrayT & array, & merged;
        ComparatorT & comparator;
        std::size_t start, finish;
        
        void operator()() {
            std::size_t outer = start;
            std::size_t inner = finish;
            std::size_t offset = start;
                        
            while (offset != finish) {
                // This will be true if the items are ordered correctly.
                bool comparison;
                
                // If two items are the same, we give priority to the outer most partition.
                // Since this is a constant, we assume the compiler will optimize this block of code.
                if (DIRECTION == 1) {
                    comparison = comparator(array[inner], array[outer]);
                } else if (DIRECTION == -1) {
                    comparison = comparator(array[outer], array[inner]);
                }
                
                if (comparison) {
                    merged[offset] = array[inner];
                    offset += DIRECTION;
                    inner += DIRECTION;
                } else {
                    merged[offset] = array[outer];
                    offset += DIRECTION;
                    outer += DIRECTION;
                }
            }            
        }
    };
    
    // Use this to control whether parallal partition is used.
    const bool PARALLEL_PARTITION = true;
    
    // Use this to control whether parallel merge is used.
    const bool PARALLEL_MERGE = true;
    
    // If you make this number too small, e.g. <= 2, you may cause synchronsation issues, because you will force parallelisation
    // for base cases which actually need to be sequential to ensure that comparison cache is generated correctly.
    const std::size_t PARALLEL_MERGE_MINIMUM_COUNT = 128;
    
    // Provide an array, and an upper and lower bound, along with the number of threads to use.
    // If parity is true, the data is in array,
    // If parity is false, the data is in temporary.
    template <typename ArrayT, typename ComparatorT>
    void partition(ArrayT & source, ArrayT & destination, ComparatorT & comparator, std::size_t lowerBound, std::size_t upperBound, std::size_t threaded) {
        std::size_t count = upperBound - lowerBound;

        //std::cerr << "Partition: " << lowerBound << " -> " << upperBound << std::endl;
        if (count > 1) {
            std::size_t middle = (lowerBound + upperBound) / 2;
            
            if (PARALLEL_PARTITION && threaded > 0) {
                // We could check whether there is any work to do before creating threads, but we assume
                // that threads will only be created high up in the tree by default, so there *should*
                // be a significant work available per-thread.
                ParallelPartition<ArrayT, ComparatorT> 
                    lowerPartition = {destination, source, comparator, lowerBound, middle, threaded - 1}, 
                    upperPartition = {destination, source, comparator, middle, upperBound, threaded - 1};
                
                std::thread 
                    lowerThread(lowerPartition),
                    upperThread(upperPartition);
                
                upperThread.join();
                lowerThread.join();                
            } else {
                partition(destination, source, comparator, lowerBound, middle);
                partition(destination, source, comparator, middle, upperBound);
            }
            
            if (PARALLEL_MERGE && threaded > 0 && count > PARALLEL_MERGE_MINIMUM_COUNT) {
                // By the time we get here, we are sure that both left and right partitions have been merged
                ParallelMerge<ArrayT, ComparatorT, +1> leftMerge = {source, destination, comparator, lowerBound, middle};
                ParallelMerge<ArrayT, ComparatorT, -1> rightMerge = {source, destination, comparator, upperBound-1, middle-1};
                
                std::thread
                    leftThread(leftMerge),
                    rightThread(rightMerge);
                
                leftThread.join();
                rightThread.join();
            } else {
                merge(source, destination, comparator, lowerBound, middle, upperBound);
            }
        }
    }
    
    // Merge two sorted sub-sequences.
    template <typename ArrayT, typename ComparatorT>
    void merge (ArrayT & array, ArrayT & merged, ComparatorT & comparator, std::size_t lowerBound, std::size_t middleBound, std::size_t upperBound) {
        //std::cerr << "Merging [" << lowerBound << " -> " << middleBound << " -> " << upperBound << "]" << std::endl;
        //std::cerr << "From " << array << " to " << merged << std::endl;
        
        std::size_t left = lowerBound;
        std::size_t right = middleBound;
        std::size_t offset = lowerBound;
        
        // We merge both sub-sequences, defined as [lowerBound, middleBound] and [middleBound, upperBound].
        while (true) {
            if (comparator(array[left], array[right])) {
                merged[offset++] = array[left++];
                
                // If we have adjusted left, we may have exhausted left side:
                if (left == middleBound) {
                    // We have no more elements in lower half.
                    std::copy(array.begin() + right, array.begin() + upperBound, merged.begin() + offset);
                    break;
                }
            } else {
                merged[offset++] = array[right++];
                
                // As above, we may have exhausted right side:
                if (right == upperBound) {
                    // We have no more elements in upper half.
                    std::copy(array.begin() + left, array.begin() + middleBound, merged.begin() + offset);
                    break;
                }
            }
        }
    }
    
    template <typename ArrayT, typename ComparatorT>
    void sort(ArrayT & array, ComparatorT & comparator, std::size_t threaded = 2) {
        ArrayT temporary(array.begin(), array.end());
        
        if (threaded == 0)
            partition(temporary, array, comparator, 0, array.size());
        else
            partition(temporary, array, comparator, 0, array.size(), threaded);
    }
}

namespace DictionarySort {
    typedef std::uint64_t IndexT;
    
    template <typename CharT, typename MapT>
    class Dictionary {    
    public:
        typedef std::vector<CharT> WordT;
        typedef std::vector<WordT> WordsT;        
        typedef std::vector<IndexT> OrderT;
        
        static const int ORDERED_LT = -1;
        static const int ORDERED_EQ = 0;
        static const int ORDERED_GT = 1;
        
        // Compare two order vectors to determine the relative nature of lhs compared to rhs.
        // We assume that lhs and rhs have at least one element each.
        // ORDERED_LT => (lhs < rhs)
        // ORDERED_EQ => (lhs == rhs)
        // ORDERED_GT => (lhs > rhs)
        static int compare(const OrderT & lhs, const OrderT & rhs)
        {
            std::size_t offset = 0;
            
            while (offset < lhs.size() && offset < rhs.size()) {
                if (lhs[offset] < rhs[offset])
                    return ORDERED_LT;
                else if (lhs[offset] > rhs[offset])
                    return ORDERED_GT;
                
                offset += 1;
            }
            
            if (lhs.size() == rhs.size())
                return ORDERED_EQ;
            
            // lhs was longer, 
            if (offset < lhs.size())
                return ORDERED_GT;
            
            return ORDERED_LT;
        }
        
    private:
        WordT m_alphabet;
        
        MapT m_characterOrder;
        //int m_characterOrder[256];
        //std::map<CharT, IndexT> m_characterOrder;
        
        IndexT width;
        IndexT charactersPerSegment;
        
        struct OrderedWord {
            WordT word;
            Dictionary * dictionary;
            mutable OrderT order;
            
            // The first time this function is called, it must be guaranteed from a single thread.
            // After that, it can be called from multiple threads at the same time.
            // The parallel merge sort algorithm guarantees this.
            OrderT & fetchOrder() const {
                if (order.size() == 0 && word.size() > 0)
                    order = dictionary->sum(word);
                
                return order;
            }
            
            bool operator<(const OrderedWord & other) const {
                return compare(this->fetchOrder(), other.fetchOrder()) == ORDERED_LT;
            }
        };

    public:
        Dictionary(WordT alphabet)
        : m_alphabet(alphabet)
        {
            IndexT index = 1;
            
            // Build up the character order map
            for (typename WordT::iterator i = m_alphabet.begin(); i != m_alphabet.end(); ++i) {
                m_characterOrder[*i] = index;
                index += 1;
            }
            
            width = std::ceil(std::log(alphabet.size()) / std::log(2));
            
            // Naturally floor the result by integer division/truncation.
            charactersPerSegment = (sizeof(IndexT) * 8) / width;
        }
        
        OrderT sum(const WordT & word) {
            OrderT order;
            std::size_t index = 0;
            
            while (index < word.size()) {
                IndexT count = charactersPerSegment;
                IndexT sum = 0;
                
                while (index < word.size()) {
                    count -= 1;

                    sum <<= width;
                    sum += m_characterOrder[word[index]];
                    
                    index += 1;
                    
                    if (count == 0)
                        break;
                }
                            
                // Shift along any remaining count, since we are ordering using the left most significant character.
                sum <<= (count * width);
                order.push_back(sum);
            }
            
            return order;
        }
        
        typedef std::vector<OrderedWord*> OrderedWordsT;
        
        void sort(const WordsT & input, WordsT & output)
        {
            OrderedWordsT words;
            words.reserve(input.size());
            
            output.resize(0);
            output.reserve(input.size());
            
            // Calculate order vector for each word in preparation for sort.
            for (typename WordsT::const_iterator i = input.begin(); i != input.end(); ++i) {
                OrderedWord * orderedWord = new OrderedWord;
                orderedWord->word = *i;
                orderedWord->dictionary = this;
                
                // We can generate this as part of the sorting process. Because the sort is parallel,
                // generation of word order (which is relatively expensive) is distributed across multiple processors.
                //orderedWord->order = sum(*i);
                
                words.push_back(orderedWord);
                
                //std::cout << "Word: " << *i << " Sum: " << sum(*i) << std::endl;
            }
            
            Benchmark::Timer ts;
            // Sort the words using built-in sorting algorithm.
            pointer_less_than<OrderedWord> comparator;
            //std::sort(words.begin(), words.end(), comparator);
            ParallelMergeSort::sort(words, comparator, 3);
            std::cerr << "Sort time: " << ts.time() << std::endl;
            
            // Copy sorted words to output vector.
            for (typename OrderedWordsT::iterator i = words.begin(); i != words.end(); ++i) {
                output.push_back((*i)->word);
                delete *i;
            }
        }
    };
}

void testSort ()
{
    const int data[] = {6, 9, 5, 4, 8, 3, 2, 1, 7};
    std::vector<int> v(data, data+(sizeof(data)/sizeof(int)));
    
    std::cerr << "Sorting " << v << std::endl;

    std::less<int> comparator;
    ParallelMergeSort::sort(v, comparator, 1);
    
    std::cerr << "Sorted  " << v << std::endl;   
}

void testDictionary ()
{
    // This defines a dictionary based on ASCII characters.
    typedef DictionarySort::Dictionary<char, DictionarySort::IndexT[256]> ASCIIDictionaryT;
    
    // For unicode characters, you could use something like this:
    // typedef DictionarySort::Dictionary<uint32_t, std::map<uint32_t, DictionarySort::IndexT>> UCS32DictionaryT;
    // Be aware that 
    
    std::string s = "abcdefghijklmnopqrstuvwxyz";
    ASCIIDictionaryT::WordT alphabet(s.begin(), s.end());
    
    ASCIIDictionaryT dictionary(alphabet);
    
    ASCIIDictionaryT::WordsT words, sortedWords;
    std::ifstream inputFile("/usr/share/dict/words");
    while (inputFile.good()) {
        std::string inputData;
        inputFile >> inputData;
        
        ASCIIDictionaryT::WordT word(inputData.begin(), inputData.end());
        words.push_back(word);
    }
    
    //words.resize(200);
    
    std::cerr << "Sorting " << words.size() << " words..." << std::endl;
    //std::cerr << "Unsorted: " << words << std::endl;
    
    const int K = 10;
    Benchmark::Timer t;
    for (std::size_t i = 0; i < K; i += 1) {
        sortedWords.resize(0);
        
        dictionary.sort(words, sortedWords);
    }
    Benchmark::TimeT elapsedTime = t.time();
    
    std::cerr << "Time: " << (elapsedTime / K) << std::endl;
    //std::cerr << "Sorted: " << sortedWords << std::endl;
        
    std::cerr << "Finished." << std::endl;
}

int main (int argc, const char * argv[])
{   
    //testSort();
    
    testDictionary();
    
    return 0;
}

