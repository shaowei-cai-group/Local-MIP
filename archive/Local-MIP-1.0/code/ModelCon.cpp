/*=====================================================================================

    Filename:     ModelCon.cpp

    Description:
        Version:  1.0

    Author:       Peng Lin, penglincs@outlook.com

    Organization: Shaowei Cai Group,
                  State Key Laboratory of Computer Science,
                  Institute of Software, Chinese Academy of Sciences,
                  Beijing, China

=====================================================================================*/

#include "ModelCon.h"

ModelCon::ModelCon(
    const string &_name,
    const size_t _idx)
    : name(_name),
      isEqual(false),
      isLarge(false),
      idx(_idx),
      RHS(0),
      inferSAT(false),
      termNum(-1)
{
}

ModelCon::~ModelCon()
{
  coeffSet.clear();
  varIdxSet.clear();
  posInVar.clear();
}

ModelConUtil::ModelConUtil()
    : conNum(-1)
{
}

ModelConUtil::~ModelConUtil()
{
  conSet.clear();
  name2idx.clear();
}

size_t ModelConUtil::MakeCon(
    const string &_name)
{
  auto iter = name2idx.find(_name);
  if (iter != name2idx.end())
    return iter->second;
  size_t conIdx = conSet.size();
  conSet.emplace_back(_name, conIdx);
  name2idx[_name] = conIdx;
  return conIdx;
}

size_t ModelConUtil::GetConIdx(
    const string &_name)
{
  if (_name == objName)
    return 0;
  auto iter = name2idx.find(_name);
  return iter->second;
}

const ModelCon &ModelConUtil::GetCon(
    const size_t _idx) const
{
  return conSet[_idx];
}

ModelCon &ModelConUtil::GetCon(
    const size_t _idx)
{
  return conSet[_idx];
}

ModelCon &ModelConUtil::GetCon(
    const string &_name)
{
  if (_name == objName)
    return conSet[0];
  return conSet[name2idx[_name]];
}
