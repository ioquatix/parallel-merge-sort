//
//  DictionarySort.h
//  DictionarySort
//
//  Created by Samuel Williams on 2/11/11.
//  Copyright (c) 2011 Orion Transfer Ltd. All rights reserved.
//

#ifndef DictionarySort_DictionarySort_h
#define DictionarySort_DictionarySort_h

#include <cmath>
#include <vector>
#include <map>

#include "ParallelMergeSort.h"

template <typename AnyT>
struct pointer_less_than
{
    bool operator()(const AnyT a, const AnyT b) {
        return *a < *b;
    }
};

namespace DictionarySort {
    // Use std::sort
    const int SORT_MODE = -1;
    // Use ParallelMergeSort with 2^n threads
    //const int SORT_MODE = 5; // = n
    
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
        
        static int compare(Dictionary * dictionary, const WordT & lhs, const WordT & rhs) {
            std::size_t offset = 0;
            
            while (offset < lhs.size() && offset < rhs.size()) {
                IndexT left_order = dictionary->_characterOrder[lhs[offset]];
                IndexT right_order = dictionary->_characterOrder[rhs[offset]];
                
                if (left_order < right_order)
                    return ORDERED_LT;
                else if (left_order > right_order)
                    return ORDERED_GT;
                
                offset += 1;
            }
            
            if (lhs.size() == rhs.size())
                return ORDERED_EQ;
            
            if (offset < lhs.size())
                return ORDERED_GT;
            
            return ORDERED_LT;
        }
        
    private:
        WordT _alphabet;
        
        MapT _characterOrder;
        //int _characterOrder[256];
        //std::map<CharT, IndexT> _characterOrder;
        
        IndexT width;
        IndexT characters_per_segment;
        
        // This is a light weight wrapper over WordT which caches its OrderT, an integer representation of position based on the given dictionary.
        struct OrderedWord {
            WordT word;
            
            // We can generate this as part of the sorting process. Because the sort is parallel, generation of word order (which is relatively expensive) is distributed across multiple processors.
            mutable OrderT order;
            
            // The first time this function is called, it must be guaranteed from a single thread.
            // After that, it can be called from multiple threads at the same time.
            // The parallel merge sort algorithm guarantees this.
            const OrderT & fetch_order(Dictionary * dictionary) const {
                if (order.size() == 0 && word.size() > 0)
                    order = dictionary->sum(word);
                
                return order;
            }
        };
        
        struct CompareWordsAscending {
            Dictionary * dictionary;
            
            CompareWordsAscending (Dictionary * _dictionary)
                : dictionary(_dictionary)
            {
            }
            
            bool operator()(const WordT * a, const WordT * b) const {
                return compare(dictionary, a, b) == ORDERED_LT;
            }
            
            bool operator()(const OrderedWord * a, const OrderedWord * b) const {
                return compare(a->fetch_order(dictionary), b->fetch_order(dictionary)) == ORDERED_LT;
            }
        };
        
        struct UnorderedWord {
            WordT word;
            Dictionary * dictionary;
            
            UnorderedWord(const WordT & _word, Dictionary * _dictionary)
            : word(_word), dictionary(_dictionary) {
                
            }
            
            bool operator<(const UnorderedWord & other) const {
                return compare(dictionary, *this, other) == ORDERED_LT;
            }
        };
        
    public:
        Dictionary(WordT alphabet)
        : _alphabet(alphabet)
        {
            IndexT index = 1;
            
            // Build up the character order map
            for (typename WordT::iterator i = _alphabet.begin(); i != _alphabet.end(); ++i) {
                _characterOrder[*i] = index;
                index += 1;
            }
            
            width = std::ceil(std::log(alphabet.size()) / std::log(2));
            
            // Naturally floor the result by integer division/truncation.
            characters_per_segment = (sizeof(IndexT) * 8) / width;
        }
        
        OrderT sum(const WordT & word) {
            OrderT order;
            std::size_t index = 0;
            
            while (index < word.size()) {
                IndexT count = characters_per_segment;
                IndexT sum = 0;
                
                while (index < word.size()) {
                    count -= 1;
                    
                    sum <<= width;
                    sum += _characterOrder[word[index]];
                    
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
                
        // The words will be sorted in-place.
        template <typename ToSortT>
        void sort (ToSortT & words, int mode = 2)
        {
            Benchmark::Timer ts;
            CompareWordsAscending comparator(this);
            
            if (mode == -1) {
                // Sort the words using built-in sorting algorithm, for comparison:
                std::sort(words.begin(), words.end(), comparator);
            } else {                
                ParallelMergeSort::sort(words, comparator, std::size_t(mode));
            }
            std::cerr << "Dictionary sort time: " << ts.time() << std::endl;
        }
        
        // This function can be slow due to the large amount of memory required for large datasets.
        uint64_t sort(const WordsT & input, WordsT & output)
        {
            typedef std::vector<OrderedWord*> OrderedWordsT;

            // Allocate all words in one go:
            OrderedWord * allocation = new OrderedWord[input.size()];
            
            // Copy pointers to intermediate list which will be used for sorting:
            OrderedWordsT words(input.size());
            
            // Calculate order vector for each word in preparation for sort.
            for (std::size_t i = 0; i < input.size(); i += 1) {
                words[i] = &allocation[i];
                
                words[i]->word = input[i];
                
                // We can force generation of the order cache, but performance may be reduced.
                //words[i]->fetch_order(this);
            }
            
            // Change the mode from -1 for std::sort, to 0..n for ParallelMergeSort where 2^n is the number of threads to use.
            sort(words, SORT_MODE);
            
            // Prepare container for sorted output:
            output.reserve(input.size());
            output.resize(0);
            
            uint64_t checksum = 1, offset = 1;
            // Copy sorted words to output vector.
            for (typename OrderedWordsT::iterator i = words.begin(); i != words.end(); ++i) {
                output.push_back((*i)->word);
                
                // Compute a very simple checksum for verifying sorted order.
                const OrderT & order = (*i)->fetch_order(this);
                for (typename OrderT::const_iterator j = order.begin(); j != order.end(); ++j) {
                    checksum ^= *j + (offset++ % checksum);
                }                
            }
            
            delete[] allocation;
            
            return checksum;
        }
    };
}

#endif
