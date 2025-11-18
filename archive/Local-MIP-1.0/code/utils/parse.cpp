/*=====================================================================================

    Filename:     parse.cpp

    Description:
        Version:  1.0

    Author:       Peng Lin, penglincs@outlook.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include <cstring>
#include <fstream>
#include "header.h"
#include "paras.h"

Value paras::identify_opt(const char *file)
{
    char name[strlen(file) + 1], p = -1, l = strlen(file);
    for (int i = l - 1; i >= 0; i--)
        if (file[i] == '/')
        {
            p = i;
            break;
        }
    strncpy(name, file + p + 1, l - p - 1);
    name[l - p - 1] = '\0';
    printf("c File name (with path): %s\n", file);
    printf("c File name: %s\n", name);
    return NegativeInfinity;
}
