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
    
    class WallTime {
    protected:
        mutable TimeT _last, _total;
        
    public:
        WallTime ();
        
        void reset ();
        TimeT total () const;
    };

	class ProcessorTime {
	protected:
		mutable std::clock_t _last, _total;

	public:
		ProcessorTime();

		void reset ();
		TimeT total () const;
	};

	class Timer {
	protected:
		WallTime _wall_time;
		ProcessorTime _processor_time;

	public:
		Timer();

		const WallTime & wall_time() const { return _wall_time; }
		const ProcessorTime & processor_time() const { return _processor_time; }

		void reset ();

		struct Sample {
			TimeT wall_time_total;
			TimeT processor_time_total;

			TimeT approximate_processor_usage() const {
				return processor_time_total / wall_time_total;
			}
		};

		Sample sample() const;
	};

	
}

#endif
