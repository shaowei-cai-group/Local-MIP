/*=====================================================================================

    Filename:     ModelCon.h

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

class ModelCon
{
public:
  string name;
  size_t idx;
  bool isEqual;
  bool isLarge;
  vector<Value> coeffSet;
  vector<size_t> varIdxSet;
  vector<size_t> posInVar;
  Value RHS;
  bool inferSAT;
  size_t termNum;

  ModelCon(
      const string &_name,
      const size_t _idx);
  ~ModelCon();
};

class ModelConUtil
{
public:
  unordered_map<string, size_t> name2idx;
  vector<ModelCon> conSet;
  string objName;
  size_t conNum;
  int MIN = 1;

  ModelConUtil();
  ~ModelConUtil();
  size_t MakeCon(
      const string &_name);
  size_t GetConIdx(
      const string &_name);
  const ModelCon &GetCon(
      const size_t _idx) const;
  ModelCon &GetCon(
      const size_t _idx);
  ModelCon &GetCon(
      const string &_name);
};