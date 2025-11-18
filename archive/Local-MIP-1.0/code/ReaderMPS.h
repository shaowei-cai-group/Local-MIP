/*=====================================================================================

    Filename:     ReaderMPS.h

    Description:
        Version:  1.0

    Author:       Peng Lin, penglincs@outlook.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#pragma once
#include "utils/paras.h"
#include "ModelCon.h"
#include "ModelVar.h"

class ReaderMPS
{
private:
  ModelConUtil *modelConUtil;
  ModelVarUtil *modelVarUtil;
  istringstream iss;
  string readLine;
  bool integralityMarker;
  bool TightenBound();
  void TightenBoundVar(ModelCon &_modelCon);
  bool TightBoundGlobally();
  bool SetVarType();
  void SetVarIdx2ObjIdx();
  vector<size_t> fixedIdxs;
  size_t deleteConNum;
  size_t deleteVarNum;
  size_t inferVarNum;
  inline void IssSetup();
  void PushCoeffVarIdx(
      const size_t _conIdx,
      Value _coeff,
      const string &_varName);

public:
  ReaderMPS(
      ModelConUtil *_modelConUtil,
      ModelVarUtil *_modelVarUtil);
  ~ReaderMPS();
  void Read(
      const char *_fileName);
};