#ifndef _paras_hpp_INCLUDED
#define _paras_hpp_INCLUDED

#include <string>
#include <cstring>
#include <unordered_map>
#include "header.h"

//        name          , type  , short-name, must-need, default, low, high, comments
#define PARAS \
    PARA( cutoff        , double, '\0' , false , 7200       , 0  , 1e8      , "Cutoff time") \
    PARA( PrintSol      , int   , '\0' , false , 1          , 0  , 1        , "Print best found solution or not")\
    PARA( DEBUG         , int   , '\0' , false , 0          , 0  , 1        , "")


//            name,   short-name, must-need, default, comments
#define STR_PARAS \
    STR_PARA( instance   , 'i'   ,  true    , "" , ".mps format instance")
    
struct paras 
{
#define PARA(N, T, S, M, D, L, H, C) \
    T N = D;
    PARAS 
#undef PARA

#define STR_PARA(N, S, M, D, C) \
    std::string N = D;
    STR_PARAS
#undef STR_PARA

void parse_args(int argc, char *argv[]);
void print_change();
Value identify_opt(const char *file);
};

#define INIT_ARGS __global_paras.parse_args(argc, argv);

extern paras __global_paras;

#define OPT(N) (__global_paras.N)

#endif