#include "paras.h"
#include "cmdline.h"
#include <cstdio>
#include <iostream>
#include <string>

paras __global_paras;

void paras::parse_args(int argc, char *argv[]) {
    cmdline::parser parser;

    #define STR_PARA(N, S, M, D, C) \
    parser.add<std::string>(#N, S, C, M, D);
    STR_PARAS
    #undef STR_PARA

    #define PARA(N, T, S, M, D, L, H, C) \
    if (!strcmp(#T, "int")) parser.add<int>(#N, S, C, M, D, cmdline::range((int)L, (int)H)); \
    else parser.add<double>(#N, S, C, M, D, cmdline::range((double)L, (double)H));
    PARAS
    #undef PARA

    parser.parse_check(argc, argv);

    #define STR_PARA(N, S, M, D, C) \
    OPT(N) = parser.get<std::string>(#N);
    STR_PARAS
    #undef STR_PARA

    #define PARA(N, T, S, M, D, L, H, C) \
    if (!strcmp(#T, "int")) OPT(N) = parser.get<int>(#N); \
    else OPT(N) = parser.get<double>(#N);
    PARAS
    #undef PARA
}

void paras::print_change() {
    printf("c ------------------- Paras list -------------------\n");
    printf("c %-20s\t %-10s\t %-10s\t %-10s\t %s\n",
           "Name", "Type", "Now", "Default", "Comment");

#define PARA(N, T, S, M, D, L, H, C) \
    if (!strcmp(#T, "int")) printf("c %-20s\t %-10s\t %-10d\t %-10s\t %s\n", (#N), (#T), N, (#D), (C)); \
    else printf("c %-20s\t %-10s\t %-10f\t %-10s\t %s\n", (#N), (#T), N, (#D), (C)); 
    PARAS
#undef PARA

#define STR_PARA(N, S, M, D, C) \
    printf("c %-20s\t string\t\t %-10s\t %-10s\t %s\n", (#N), N.c_str(), (#D), (C));
    STR_PARAS
#undef STR_PARA

    printf("c --------------------------------------------------\n");
}