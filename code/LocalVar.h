
#pragma once
#include "utils/paras.h"

class LocalVar
{
public:
  Value nowValue;
  Value bestValue;
  size_t allowIncStep;
  size_t allowDecStep;
  size_t lastIncStep;
  size_t lastDecStep;

  LocalVar();
  ~LocalVar();
};

class LocalVarUtil
{
public:
  vector<LocalVar> varSet;
  vector<Value> lowerDeltaInLiftMove;
  vector<Value> upperDeltaInLifiMove;
  vector<Value> tempDeltas;
  vector<size_t> tempVarIdxs;
  vector<bool> scoreTable;
  vector<size_t> binaryIdx;
  unordered_set<size_t> affectedVar;

  LocalVarUtil();
  ~LocalVarUtil();
  void Allocate(
      size_t _varNum,
      size_t _varNumInObj);
  LocalVar &GetVar(
      size_t _idx);
};