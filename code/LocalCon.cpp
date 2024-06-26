
#include "LocalCon.h"

LocalCon::LocalCon()
    : weight(1),
      RHS(0),
      LHS(0)
{
}

LocalCon::~LocalCon()
{
}

bool LocalCon::SAT()
{
  return LHS < RHS + FeasibilityTol;
}

bool LocalCon::UNSAT()
{
  return LHS >= RHS + FeasibilityTol;
}

LocalConUtil::LocalConUtil()
{
}

void LocalConUtil::Allocate(
    const size_t _conNum)
{
  unsatConIdxs.reserve(_conNum);
  tempSatConIdxs.reserve(_conNum);
  tempUnsatConIdxs.reserve(_conNum);
  conSet.resize(_conNum);
}

LocalConUtil::~LocalConUtil()
{
  tempSatConIdxs.clear();
  tempUnsatConIdxs.clear();
  conSet.clear();
  unsatConIdxs.clear();
}

LocalCon &LocalConUtil::GetCon(
    const size_t _idx)
{
  return conSet[_idx];
}

void LocalConUtil::insertUnsat(
    const size_t _conIdx)
{
  conSet[_conIdx].posInUnsatConIdxs = unsatConIdxs.size();
  unsatConIdxs.push_back(_conIdx);
}

void LocalConUtil::RemoveUnsat(
    const size_t _conIdx)
{
  assert(unsatConIdxs.size() > 0);
  if (unsatConIdxs.size() == 1)
  {
    unsatConIdxs.pop_back();
    return;
  }
  size_t pos = conSet[_conIdx].posInUnsatConIdxs;
  unsatConIdxs[pos] = *unsatConIdxs.rbegin();
  unsatConIdxs.pop_back();
  conSet[unsatConIdxs[pos]].posInUnsatConIdxs = pos;
}