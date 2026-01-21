/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*               This file is part of the program and library                */
/*    PaPILO --- Parallel Presolve for Integer and Linear Optimization       */
/*                                                                           */
/* Copyright (C) 2020-2025 Zuse Institute Berlin (ZIB)                       */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License");           */
/* you may not use this file except in compliance with the License.          */
/* You may obtain a copy of the License at                                   */
/*                                                                           */
/*     http://www.apache.org/licenses/LICENSE-2.0                            */
/*                                                                           */
/* Unless required by applicable law or agreed to in writing, software       */
/* distributed under the License is distributed on an "AS IS" BASIS,         */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  */
/* See the License for the specific language governing permissions and       */
/* limitations under the License.                                            */
/*                                                                           */
/* You should have received a copy of the Apache-2.0 license                 */
/* along with PaPILO; see the file LICENSE. If not visit scipopt.org.        */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once
#include <fstream>
#include <map>
#include <ranges>

#include <sstream>


template <typename REAL>
struct SolParser
{

  static bool read(const std::string& filename,
                   const std::unordered_map<std::string, size_t>& colnames,
                   std::vector<REAL>& solution_vector)
  {
    auto sol_filename = replace_mps_with_sol(filename);
    std::ifstream in(sol_filename, std::ifstream::in);

    if (!in)
      return false;

    /* TODO: if Boost iostreams is available one can could read zipped files*/
    // boost::iostreams::filtering_istream in;
    // if (boost::algorithm::ends_with(sol_filename, ".gz"))
    // in.push(boost::iostreams::gzip_decompressor());
    // if (boost::algorithm::ends_with(sol_filename, ".bz2"))
    // in.push(boost::iostreams::bzip2_decompressor());

    // in.push(file);

    std::map<std::string, int> nameToCol;

    nameToCol.insert(colnames.begin(), colnames.end());


    solution_vector.resize(colnames.size(), REAL{0});
    std::string strline;

    skip_header(colnames, in, strline);

    std::pair<bool, REAL> result;
    do
    {
      auto tokens = split(strline.c_str());
      assert(!tokens.empty());

      if (auto it = nameToCol.find(tokens[0]); it != nameToCol.end())
      {
        assert(tokens.size() > 1);
        result = parse_number(tokens[1]);
        if (result.first)
        {

          // fmt::print("Could not parse solution {}\n", tokens[1]);
          return false;
        }
        solution_vector[it->second] = result.second;
      }
      else if (strline.empty())
      {
      }
      else
      {
        // fmt::print( stderr,
        //             "WARNING: skipping unknown column {} in solution\n",
        //             tokens[0] );
      }
    } while (getline(in, strline));

    return true;
  }

  static std::string replace_mps_with_sol(const std::string& filename)
  {
    constexpr std::string_view from = ".mps";
    constexpr std::string_view to   = ".sol";

    if (filename.size() >= from.size() &&
        filename.compare(filename.size() - from.size(), from.size(), from) == 0)
    {
      return filename.substr(0, filename.size() - from.size()) + std::string(to);
    }

    return filename; // unchanged if it does not end in .mps
  }

  static std::pair<bool, REAL> parse_number(const std::string& s)
  {
    REAL number;
    try
    {
      std::stringstream string_stream;
      string_stream.str(s);
      string_stream >> number;
      if (!string_stream.fail() && string_stream.eof())
        return {false, number};
    }
    catch (...)
    {
    }

    return {true, 0};
  }


private:
  static void
  skip_header(const std::unordered_map<std::string,size_t>& colnames,
              std::ifstream& filteringIstream,
              std::string& strline)
  {
    while (getline(filteringIstream, strline))
    {
      for (const auto& colname : colnames | std::views::keys)
      {
        if (strline.find(colname) != std::string::npos)
          return;
      }
    }
  }

  std::vector<std::string> static split(const char* str)
  {
    std::vector<std::string> tokens;
    const char c1 = ' ';
    const char c2 = '\t';

    do
    {
      const char* begin = str;

      while (*str != c1 && *str != c2 && *str)
        str++;

      tokens.emplace_back(begin, str);

      while ((*str == c1 || *str == c2) && *str)
        str++;

    } while (0 != *str);

    return tokens;
  }
};
