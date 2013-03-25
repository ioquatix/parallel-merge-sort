//
//  Benchmark.cpp
//  DictionarySort
//
//  Created by Samuel Williams on 2/11/11.
//  Copyright (c) 2011 Orion Transfer Ltd. All rights reserved.
//

#include "Benchmark.h"

#include <sys/time.h>

// A timer class for quickly checking the wall-clock performance of code.
namespace Benchmark {
    static TimeT system_time () {
        static struct timeval t;
        gettimeofday (&t, (struct timezone*)0);
        return ((TimeT)t.tv_sec) + ((TimeT)t.tv_usec / 1000000.0);
    }
    
    WallTime::WallTime () {
        this->reset();
    }
    
    void WallTime::reset () {
        this->_last = system_time();
        this->_total = 0.0;
    }
    
    TimeT WallTime::total () const {
        TimeT current = system_time();
        this->_total += current - this->_last;
        this->_last = current;
        return this->_total;
    }

	ProcessorTime::ProcessorTime()
	{
		this->reset();
	}

	void ProcessorTime::reset ()
	{
		this->_last = std::clock();
		this->_total = 0;
	}

	TimeT ProcessorTime::total () const
	{
		std::clock_t current = std::clock();
		this->_total += std::clock() - this->_last;
		this->_last = current;
		
		return TimeT(this->_total) / TimeT(CLOCKS_PER_SEC);
	}

	Timer::Timer()
	{
	}

	void Timer::reset ()
	{
		_wall_time.reset();
		_processor_time.reset();
	}

	Timer::Sample Timer::sample() const
	{
		return {_wall_time.total(), _processor_time.total()};
	}
}