/*=====================================================================================

    Filename:     Model_Reader.h

    Description:  Unified model file reading entry point
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#pragma once

#include <string>

class Model_Manager;

void read_model_file(const std::string& p_model_file,
                     Model_Manager& p_model_manager);
