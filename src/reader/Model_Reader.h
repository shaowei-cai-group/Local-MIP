/*=====================================================================================

    Filename:     Model_Reader.h

    Description:  Base class interface for model file readers
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once

class Model_Reader
{
public:
  virtual ~Model_Reader() = default;

  virtual void read(const char* p_model_file) = 0;
};
