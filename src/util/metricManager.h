#pragma once
#include <bvar/bvar.h>
#include <bvar/multi_dimension.h>
namespace utils{
    bvar::Adder<int> time;
    bvar::Window<bvar::Adder<int> > time_minitue("metric", "time", &time, 60);
       
}