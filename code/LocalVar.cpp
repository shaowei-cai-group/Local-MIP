
#include "LocalVar.h"

LocalVar::LocalVar()
    : allowIncStep(0),
      allowDecStep(0),
      lastIncStep(0),
      lastDecStep(0)
{
}

LocalVar::~LocalVar()
{
}

LocalVarUtil::LocalVarUtil()
{
}

void LocalVarUtil::Allocate(
    size_t _varNum,
    size_t _varNumInObj)
{
  tempDeltas.reserve(_varNum);
  tempVarIdxs.reserve(_varNum);
  affectedVar.reserve(_varNum);
  varSet.resize(_varNum);
  scoreTable.resize(_varNum, false);
  lowerDeltaInLiftMove.resize(_varNumInObj);
  upperDeltaInLifiMove.resize(_varNumInObj);
}

LocalVarUtil::~LocalVarUtil()
{
  lowerDeltaInLiftMove.clear();
  upperDeltaInLifiMove.clear();
  scoreTable.clear();
  affectedVar.clear();
  varSet.clear();
  tempDeltas.clear();
  tempVarIdxs.clear();
}

LocalVar &LocalVarUtil::GetVar(
    size_t _idx)
{
  assert(_idx < varSet.size());
  return varSet[_idx];
}