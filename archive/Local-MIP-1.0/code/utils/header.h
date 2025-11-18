/*=====================================================================================

    Filename:     header.h

    Description:
        Version:  1.0

    Author:       Peng Lin, penglincs@outlook.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <stdlib.h>
#include <chrono>
using namespace std;
using Value = double;
const Value Infinity = 1e20;
const Value NegativeInfinity = -Infinity;
const Value DefaultIntegerUpperBound = 1.0;
const Value DefaultRealUpperBound = Infinity;
const Value DefaultLowerBound = 0.0;
const Value InfiniteUpperBound = Infinity;
const Value InfiniteLowerBound = NegativeInfinity;
const Value FeasibilityTol = 1e-6;
const Value OptimalTol = 1e-4;
enum class VarType
{
    Binary,
    Integer,
    Real,
    Fixed
};
std::chrono::_V2::system_clock::time_point TimeNow();
double ElapsedTime(
    const std::chrono::_V2::system_clock::time_point &a,
    const std::chrono::_V2::system_clock::time_point &b);
bool IsBlank(
    const string &a);
void PrintfError(
    const string &a);