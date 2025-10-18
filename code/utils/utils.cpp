/*=====================================================================================

    Filename:     utils.cpp

    Description:
        Version:  1.0

    Author:       Peng Lin, penglincs@outlook.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "header.h"

std::chrono::system_clock::time_point TimeNow()
{
  return chrono::system_clock::now();
}

double ElapsedTime(
    const std::chrono::system_clock::time_point &a,
    const std::chrono::system_clock::time_point &b)
{
  return chrono::duration_cast<chrono::milliseconds>(a - b).count() / 1000.0;
}

bool IsBlank(
    const string &a)
{
  for (auto x : a)
    if (x != ' ' && x != '\n' && x != '\r')
      return false;
  return true;
}

void PrintfError(
    const string &a)
{
  printf("c error line: %s\n", a.c_str());
  exit(-1);
}
