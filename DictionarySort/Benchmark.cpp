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
    TimeT syste_time () {
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
        this->_last = syste_time();
        this->_total = 0.0;
    }
    
    TimeT Timer::time () const {
        TimeT current = syste_time();
        this->_total += current - this->_last;
        this->_last = current;
        return this->_total;
    }
}