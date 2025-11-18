/*=====================================================================================

    Filename:     Solver.cpp

    Description:
        Version:  1.0

    Author:       Peng Lin, penglincs@outlook.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "Solver.h"

Solver::Solver()
{
  modelConUtil = new ModelConUtil();
  modelVarUtil = new ModelVarUtil();
  readerMPS = new ReaderMPS(modelConUtil, modelVarUtil);
  localMIP = new LocalMIP(modelConUtil, modelVarUtil);
}

Solver::~Solver()
{
}

void Solver::Run()
{
  ParseObj();
  readerMPS->Read(fileName);
  int Result = localMIP->LocalSearch(optimalObj, clkStart);
  localMIP->PrintResult();
}

void Solver::ParseObj()
{
  fileName = (char *)OPT(instance).c_str();
  optimalObj = __global_paras.identify_opt(fileName);
}