/*=====================================================================================

    Filename:     ModelVar.h

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

class ModelVar
{
public:
	string name;
	size_t idx;
	Value upperBound;
	Value lowerBound;
	vector<size_t> conIdxSet;
	vector<size_t> posInCon;
	size_t termNum;
	VarType type;

	ModelVar(
			const string &_name,
			size_t _idx,
			bool _integrality);
	~ModelVar();
	bool InBound(
			Value _value) const;
	void SetType(
			VarType _varType);
	void SetUpperBound(
			Value _upperBound);
	void SetLowerBound(
			Value _lowerBound);
	bool IsFixed();
	bool IsBinary();
};

class ModelVarUtil
{
public:
	unordered_map<string, size_t> name2idx;
	vector<ModelVar> varSet;
	vector<size_t> varIdx2ObjIdx;
	bool isBin;
	size_t varNum;
	size_t integerNum;
	size_t binaryNum;
	size_t fixedNum;
	size_t realNum;
	Value objBias;

	ModelVarUtil();
	~ModelVarUtil();
	size_t MakeVar(
			const string &_name,
			const bool _integrality);
	const ModelVar &GetVar(
			const size_t _idx) const;
	ModelVar &GetVar(
			const size_t _idx);
	ModelVar &GetVar(
			const string &_name);
};