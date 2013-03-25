//
//  Benchmark.h
//  DictionarySort
//
//  Created by Samuel Williams on 2/11/11.
//  Copyright (c) 2011 Orion Transfer Ltd. All rights reserved.
//

#ifndef DictionarySort_Benchmark_h
#define DictionarySort_Benchmark_h

#include <ctime>

// A timer class for quickly checking the wall-clock performance of code.
namespace Benchmark {
    typedef double TimeT;
    
    class Timer {
    protected:
        mutable TimeT _last, _total;
        
    public:
        Timer ();
        
        void reset ();
        TimeT time () const;
    };

	class ProcessorTime {
	protected:
		mutable std::clock_t _last, _total;

	public:
		ProcessorTime();

		void reset ();
		TimeT time ();
	};
}

#endif
