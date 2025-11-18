/*=====================================================================================

    Filename:     LP_Reader.cpp

    Description:  LP format file parser implementation
        Version:  2.0

    Author:       Peng Lin, peng.lin.csor@gmail.com

    Organization: Shaowei Cai Group

=====================================================================================*/

#include "../model_data/Model_Con.h"
#include "../model_data/Model_Var.h"
#include "../utils/global_defs.h"
#include "../utils/solver_error.h"
#include "LP_Reader.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lp_internal
{

// Identifier character set (CPLEX LP format compliant)
// Allowed: letters, digits, underscore, dot, hash, brackets, parentheses, dollar sign, @ sign
// Forbidden: semicolon, comma, backslash, slash, quotes (to avoid parsing ambiguity)
bool is_identifier_char(char p_char)
{
  unsigned char ch = static_cast<unsigned char>(p_char);
  if (std::isalnum(ch))
    return true;
  switch (p_char)
  {
    case '_':
    case '.':
    case '#':
    case '[':
    case ']':
    case '(':
    case ')':
    case '$':
    case '@':
      return true;
    default:
      return false;
  }
}

enum class Token_Type
{
  identifier,
  number,
  colon,
  semicolon,
  less_equal,
  greater_equal,
  equal,
  plus,
  minus,
  end
};

struct Token
{
  Token_Type type;
  std::string text;
  double value;
  bool has_sign;

  Token(Token_Type p_type = Token_Type::end,
        std::string p_text = "",
        double p_value = 0.0,
        bool p_has_sign = false)
      : type(p_type), text(std::move(p_text)), value(p_value),
        has_sign(p_has_sign)
  {
  }
};

class Tokenizer
{
public:
  explicit Tokenizer(std::string p_content)
      : m_content(std::move(p_content)), m_pos(0)
  {
  }

  Token next()
  {
    if (!m_buffer.empty())
    {
      Token token = m_buffer.back();
      m_buffer.pop_back();
      return token;
    }
    return read_token();
  }

  Token peek()
  {
    Token token = next();
    push_back(token);
    return token;
  }

  void push_back(const Token& p_token)
  {
    m_buffer.push_back(p_token);
  }

private:
  Token read_token()
  {
    const size_t size = m_content.size();
    while (m_pos < size &&
           std::isspace(static_cast<unsigned char>(m_content[m_pos])))
      ++m_pos;
    if (m_pos >= size)
      return Token(Token_Type::end);
    char ch = m_content[m_pos];
    if (ch == ':')
    {
      ++m_pos;
      return Token(Token_Type::colon, ":");
    }
    if (ch == ';')
    {
      ++m_pos;
      return Token(Token_Type::semicolon, ";");
    }
    if (ch == '<')
    {
      ++m_pos;
      if (m_pos < size && m_content[m_pos] == '=')
      {
        ++m_pos;
        return Token(Token_Type::less_equal, "<=");
      }
      return Token(Token_Type::less_equal, "<");
    }
    if (ch == '>')
    {
      ++m_pos;
      if (m_pos < size && m_content[m_pos] == '=')
      {
        ++m_pos;
        return Token(Token_Type::greater_equal, ">=");
      }
      return Token(Token_Type::greater_equal, ">");
    }
    if (ch == '=')
    {
      ++m_pos;
      return Token(Token_Type::equal, "=");
    }
    if ((ch == '+' || ch == '-') && m_pos + 1 < size &&
        (std::isdigit(static_cast<unsigned char>(m_content[m_pos + 1])) ||
         m_content[m_pos + 1] == '.'))
      return read_number();
    if (ch == '+' || ch == '-')
    {
      ++m_pos;
      return Token(ch == '+' ? Token_Type::plus : Token_Type::minus,
                   std::string(1, ch));
    }
    if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.')
      return read_number();
    if (is_identifier_char(static_cast<char>(ch)))
      return read_identifier();
    printf("o unsupported character in LP file: %c\n", ch);
    char message[128];
    std::snprintf(message,
                  sizeof(message),
                  "unsupported character in LP file: %c",
                  ch);
    throw Solver_Error(message);
  }

  Token read_identifier()
  {
    size_t start = m_pos;
    const size_t size = m_content.size();
    while (m_pos < size)
    {
      char ch = m_content[m_pos];
      if (!is_identifier_char(ch))
        break;
      ++m_pos;
    }
    return Token(Token_Type::identifier,
                 m_content.substr(start, m_pos - start));
  }

  Token read_number()
  {
    size_t start = m_pos;
    const size_t size = m_content.size();
    bool has_sign = false;
    if (m_content[m_pos] == '+' || m_content[m_pos] == '-')
    {
      has_sign = true;
      ++m_pos;
    }
    while (m_pos < size &&
           std::isdigit(static_cast<unsigned char>(m_content[m_pos])))
      ++m_pos;
    if (m_pos < size && m_content[m_pos] == '.')
    {
      ++m_pos;
      while (m_pos < size &&
             std::isdigit(static_cast<unsigned char>(m_content[m_pos])))
        ++m_pos;
    }
    if (m_pos < size &&
        (m_content[m_pos] == 'e' || m_content[m_pos] == 'E'))
    {
      size_t exp_pos = m_pos + 1;
      if (exp_pos < size &&
          (m_content[exp_pos] == '+' || m_content[exp_pos] == '-'))
        ++exp_pos;
      bool has_digit = false;
      while (exp_pos < size &&
             std::isdigit(static_cast<unsigned char>(m_content[exp_pos])))
      {
        has_digit = true;
        ++exp_pos;
      }
      if (has_digit)
        m_pos = exp_pos;
    }
    std::string number_string = m_content.substr(start, m_pos - start);
    double value;
    try
    {
      value = std::stod(number_string);
    }
    catch (const std::out_of_range&)
    {
      // Value out of double range, set to infinity
      value = (number_string[0] == '-') ? k_neg_inf : k_inf;
    }
    catch (const std::invalid_argument&)
    {
      // Should not happen in theory, as we have validated the format
      printf("o invalid number format in LP file: %s\n",
             number_string.c_str());
      char message[256];
      std::snprintf(message,
                    sizeof(message),
                    "invalid number format: %s",
                    number_string.c_str());
      throw Solver_Error(message);
    }
    return Token(Token_Type::number, number_string, value, has_sign);
  }

  std::string m_content;
  size_t m_pos;
  std::vector<Token> m_buffer;
};

} // namespace lp_internal

namespace
{

using lp_internal::Token;
using lp_internal::Token_Type;
using lp_internal::Tokenizer;

struct Linear_Expression
{
  std::vector<std::pair<std::string, double>> terms;
  double constant = 0.0;
};

std::string to_upper(std::string p_value)
{
  std::transform(p_value.begin(),
                 p_value.end(),
                 p_value.begin(),
                 [](unsigned char ch)
                 { return static_cast<char>(std::toupper(ch)); });
  return p_value;
}

bool is_section_keyword(const std::string& p_upper)
{
  static const std::unordered_set<std::string> k_keywords = {"SUBJECT",
                                                             "SUCH",
                                                             "ST",
                                                             "S.T.",
                                                             "S.T",
                                                             "CONSTRAINTS",
                                                             "CONSTRAINT",
                                                             "BOUNDS",
                                                             "BOUND",
                                                             "BINARIES",
                                                             "BINARY",
                                                             "BIN",
                                                             "GENERAL",
                                                             "GENERALS",
                                                             "INTEGER",
                                                             "INTEGERS",
                                                             "INT",
                                                             "END"};
  return k_keywords.count(p_upper) > 0;
}

bool is_constraints_keyword(const std::string& p_upper)
{
  return p_upper == "SUBJECT" || p_upper == "SUCH" || p_upper == "ST" ||
         p_upper == "S.T." || p_upper == "S.T" ||
         p_upper == "CONSTRAINTS" || p_upper == "CONSTRAINT";
}

bool is_bounds_keyword(const std::string& p_upper)
{
  return p_upper == "BOUNDS" || p_upper == "BOUND";
}

bool is_integers_keyword(const std::string& p_upper)
{
  return p_upper == "GENERAL" || p_upper == "GENERALS" ||
         p_upper == "INTEGER" || p_upper == "INTEGERS" || p_upper == "INT";
}

bool is_binary_keyword(const std::string& p_upper)
{
  return p_upper == "BINARY" || p_upper == "BINARIES" || p_upper == "BIN";
}

[[noreturn]] void parse_error(const std::string& p_message)
{
  printf("o invalid LP file: %s\n", p_message.c_str());
  throw Solver_Error(p_message.c_str());
}

std::string preprocess_lp_content(const std::string& p_raw)
{
  std::string result;
  result.reserve(p_raw.size());

  bool in_block_comment = false;
  const size_t size = p_raw.size();
  size_t line_start = 0;

  for (size_t idx = 0; idx < size; ++idx)
  {
    if (!in_block_comment && idx + 1 < size && p_raw[idx] == '/' &&
        p_raw[idx + 1] == '*')
    {
      in_block_comment = true;
      ++idx;
      continue;
    }
    if (in_block_comment)
    {
      if (idx + 1 < size && p_raw[idx] == '*' && p_raw[idx + 1] == '/')
      {
        in_block_comment = false;
        ++idx;
      }
      continue;
    }
    if (p_raw[idx] == '\n')
    {
      size_t line_length = idx - line_start;
      if (line_length > 0)
      {
        const char* line_ptr = p_raw.data() + line_start;
        size_t comment_pos = line_length;
        for (size_t i = 0; i + 1 < line_length; ++i)
        {
          if (line_ptr[i] == '/' && line_ptr[i + 1] == '/')
          {
            comment_pos = i;
            break;
          }
        }
        size_t first_nonspace = 0;
        while (first_nonspace < comment_pos &&
               std::isspace(
                   static_cast<unsigned char>(line_ptr[first_nonspace])))
          ++first_nonspace;
        if (first_nonspace < comment_pos &&
            line_ptr[first_nonspace] == '\\')
        {
          line_start = idx + 1;
          continue;
        }
        result.append(line_ptr, comment_pos);
      }

      result.push_back('\n');
      line_start = idx + 1;
      continue;
    }
  }
  if (line_start < size && !in_block_comment)
  {
    size_t line_length = size - line_start;
    const char* line_ptr = p_raw.data() + line_start;

    size_t comment_pos = line_length;
    for (size_t i = 0; i + 1 < line_length; ++i)
    {
      if (line_ptr[i] == '/' && line_ptr[i + 1] == '/')
      {
        comment_pos = i;
        break;
      }
    }

    size_t first_nonspace = 0;
    while (
        first_nonspace < comment_pos &&
        std::isspace(static_cast<unsigned char>(line_ptr[first_nonspace])))
      ++first_nonspace;

    if (first_nonspace >= comment_pos || line_ptr[first_nonspace] != '\\')
    {
      result.append(line_ptr, comment_pos);
      result.push_back('\n');
    }
  }
  if (in_block_comment)
  {
    printf("o Warning: unclosed block comment in LP file\n");
  }

  return result;
}

Linear_Expression parse_linear_expression(
    Tokenizer& p_tokenizer,
    const std::function<bool(const Token&)>& p_should_stop)
{
  Linear_Expression expression;
  double pending_sign = 1.0;
  while (true)
  {
    Token token = p_tokenizer.peek();
    if (token.type == Token_Type::end || p_should_stop(token))
      break;
    if (token.type == Token_Type::plus)
    {
      p_tokenizer.next();
      pending_sign = 1.0;
      continue;
    }
    if (token.type == Token_Type::minus)
    {
      p_tokenizer.next();
      pending_sign = -1.0;
      continue;
    }
    if (token.type == Token_Type::number)
    {
      token = p_tokenizer.next();
      double coeff = token.value;
      if (!token.has_sign)
        coeff *= pending_sign;
      pending_sign = 1.0;
      Token next_token = p_tokenizer.peek();
      if (next_token.type == Token_Type::identifier &&
          !is_section_keyword(to_upper(next_token.text)))
      {
        next_token = p_tokenizer.next();
        expression.terms.emplace_back(next_token.text, coeff);
      }
      else
        expression.constant += coeff;
      continue;
    }
    if (token.type == Token_Type::identifier)
    {
      token = p_tokenizer.next();
      std::string upper = to_upper(token.text);
      if (is_section_keyword(upper))
      {
        p_tokenizer.push_back(token);
        break;
      }
      expression.terms.emplace_back(token.text, pending_sign);
      pending_sign = 1.0;
      continue;
    }
    if (token.type == Token_Type::semicolon)
    {
      p_tokenizer.next();
      break;
    }
    parse_error("unexpected token inside linear expression");
  }
  return expression;
}

double parse_numeric_value(Tokenizer& p_tokenizer)
{
  double sign = 1.0;
  while (true)
  {
    Token token = p_tokenizer.next();
    if (token.type == Token_Type::plus)
    {
      sign = 1.0;
      continue;
    }
    if (token.type == Token_Type::minus)
    {
      sign = -1.0;
      continue;
    }
    if (token.type == Token_Type::number)
    {
      double value = token.value;
      if (!token.has_sign)
        value *= sign;
      return value;
    }
    if (token.type == Token_Type::identifier)
    {
      std::string upper = to_upper(token.text);
      if (upper == "INF" || upper == "INFINITY")
        return sign * k_inf;
      parse_error("invalid numeric value: " + token.text);
    }
    parse_error("expecting numeric value");
  }
}

} // namespace

using lp_internal::Token;
using lp_internal::Token_Type;
using lp_internal::Tokenizer;

LP_Reader::LP_Reader(Model_Manager* p_model_manager)
    : m_model_manager(p_model_manager), m_auto_con_counter(0)
{
}

void LP_Reader::read(const char* p_file_name)
{
  auto start_time = std::chrono::high_resolution_clock::now();
  std::ifstream infile(p_file_name);
  if (!infile)
  {
    printf("o The input filename %s is invalid.\n", p_file_name);
    char message[256];
    std::snprintf(message,
                  sizeof(message),
                  "failed to open input LP file: %s",
                  p_file_name);
    throw Solver_Error(message);
  }
  std::ostringstream buffer;
  buffer << infile.rdbuf();
  infile.close();
  std::string cleaned_content = preprocess_lp_content(buffer.str());
  lp_internal::Tokenizer tokenizer(cleaned_content);
  m_model_manager->make_con("");
  parse_objective(tokenizer);
  while (true)
  {
    Token token = tokenizer.peek();
    if (token.type == Token_Type::end)
      break;
    if (token.type == Token_Type::semicolon)
    {
      tokenizer.next();
      continue;
    }
    if (token.type != Token_Type::identifier)
    {
      parse_error("unexpected token outside of sections");
    }
    std::string upper = to_upper(token.text);
    if (is_constraints_keyword(upper))
    {
      parse_constraints(tokenizer);
      continue;
    }
    if (is_bounds_keyword(upper))
    {
      parse_bounds(tokenizer);
      continue;
    }
    if (is_integers_keyword(upper))
    {
      parse_integers(tokenizer);
      continue;
    }
    if (is_binary_keyword(upper))
    {
      parse_binaries(tokenizer);
      continue;
    }
    if (upper == "END")
    {
      tokenizer.next();
      break;
    }
    parse_error("unknown section keyword: " + token.text);
  }
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);
  printf("c reading lp file takes %.2lf seconds.\n",
         duration.count() / 1000.0);
}

void LP_Reader::parse_objective(Tokenizer& p_tokenizer)
{
  Token sense_token = p_tokenizer.next();
  if (sense_token.type != Token_Type::identifier)
    parse_error("LP objective must start with MINIMIZE or MAXIMIZE");
  std::string sense = to_upper(sense_token.text);
  if (sense == "MIN" || sense == "MINIMIZE" || sense == "MINIMUM")
  {
    // default is minimize
  }
  else if (sense == "MAX" || sense == "MAXIMIZE" || sense == "MAXIMUM")
    m_model_manager->setup_max();
  else
    parse_error("unexpected objective sense: " + sense_token.text);
  std::string obj_name = "";
  Token next_token = p_tokenizer.peek();
  if (next_token.type == Token_Type::identifier)
  {
    Token possible_name = p_tokenizer.next();
    Token colon_token = p_tokenizer.peek();
    if (colon_token.type == Token_Type::colon)
    {
      p_tokenizer.next();
      obj_name = possible_name.text;
      m_model_manager->set_obj_name(obj_name);
    }
    else
      p_tokenizer.push_back(possible_name);
  }
  auto stop_predicate = [](const Token& token)
  {
    if (token.type != Token_Type::identifier)
      return false;
    std::string upper = to_upper(token.text);
    return is_section_keyword(upper);
  };
  Linear_Expression obj_expr =
      parse_linear_expression(p_tokenizer, stop_predicate);
  for (const auto& term : obj_expr.terms)
    add_term(obj_name, term.first, term.second);
  double rhs_value = -m_model_manager->is_min() * obj_expr.constant;
  m_model_manager->con(0).set_rhs(rhs_value);
}

void LP_Reader::parse_constraints(Tokenizer& p_tokenizer)
{
  Token keyword = p_tokenizer.next();
  std::string upper = to_upper(keyword.text);
  if (upper == "SUBJECT")
  {
    Token maybe_to = p_tokenizer.peek();
    if (maybe_to.type == Token_Type::identifier)
    {
      std::string next_upper = to_upper(maybe_to.text);
      if (next_upper == "TO")
        p_tokenizer.next();
    }
  }
  else if (upper == "SUCH")
  {
    Token maybe_that = p_tokenizer.peek();
    if (maybe_that.type == Token_Type::identifier)
    {
      std::string next_upper = to_upper(maybe_that.text);
      if (next_upper == "THAT")
        p_tokenizer.next();
    }
  }
  else if (upper == "ST" || upper == "S.T." || upper == "S.T" ||
           upper == "CONSTRAINT" || upper == "CONSTRAINTS")
  {
    // nothing else to consume
  }
  else
    parse_error("invalid constraint section keyword: " + keyword.text);
  while (true)
  {
    Token token = p_tokenizer.peek();
    if (token.type == Token_Type::end)
      break;
    if (token.type == Token_Type::identifier)
    {
      std::string section_upper = to_upper(token.text);
      if (is_section_keyword(section_upper))
        break;
    }
    if (token.type == Token_Type::semicolon)
    {
      p_tokenizer.next();
      continue;
    }
    std::string con_name;
    Token possible_name = p_tokenizer.peek();
    if (possible_name.type == Token_Type::identifier)
    {
      Token name_token = p_tokenizer.next();
      Token colon_token = p_tokenizer.peek();
      if (colon_token.type == Token_Type::colon)
      {
        p_tokenizer.next();
        con_name = name_token.text;
      }
      else
        p_tokenizer.push_back(name_token);
    }
    if (con_name.empty())
      con_name = generate_constraint_name();
    auto stop = [](const Token& tk)
    {
      return tk.type == Token_Type::less_equal ||
             tk.type == Token_Type::greater_equal ||
             tk.type == Token_Type::equal || tk.type == Token_Type::end;
    };
    Linear_Expression lhs = parse_linear_expression(p_tokenizer, stop);
    Token relation = p_tokenizer.next();
    char con_symbol = '<';
    if (relation.type == Token_Type::less_equal)
      con_symbol = '<';
    else if (relation.type == Token_Type::greater_equal)
      con_symbol = '>';
    else if (relation.type == Token_Type::equal)
      con_symbol = '=';
    else
      parse_error("constraint must contain relation operator");
    double rhs = parse_numeric_value(p_tokenizer);
    size_t con_idx = m_model_manager->make_con(con_name, con_symbol);
    auto& con = m_model_manager->con(con_idx);
    con.set_rhs(rhs - lhs.constant);
    for (const auto& term : lhs.terms)
      add_term(con_name, term.first, term.second);
    Token maybe_semi = p_tokenizer.peek();
    if (maybe_semi.type == Token_Type::semicolon)
      p_tokenizer.next();
  }
}

void LP_Reader::parse_bounds(Tokenizer& p_tokenizer)
{
  Token keyword = p_tokenizer.next();
  (void)keyword;
  while (true)
  {
    Token token = p_tokenizer.peek();
    if (token.type == Token_Type::end)
      break;
    if (token.type == Token_Type::identifier)
    {
      std::string upper = to_upper(token.text);
      if (is_section_keyword(upper))
        break;
    }
    if (token.type == Token_Type::semicolon)
    {
      p_tokenizer.next();
      continue;
    }
    if (token.type == Token_Type::number)
    {
      double first_value = parse_numeric_value(p_tokenizer);
      Token first_relation = p_tokenizer.next();
      if (first_relation.type != Token_Type::less_equal &&
          first_relation.type != Token_Type::greater_equal)
        parse_error("invalid bounds statement");
      Token var_token = p_tokenizer.next();
      if (var_token.type != Token_Type::identifier)
        parse_error("expecting variable name in bounds");
      std::string var_name = var_token.text;
      size_t var_idx = m_model_manager->make_var(var_name, false);
      auto& var = m_model_manager->var(var_idx);
      Token maybe_second = p_tokenizer.peek();
      if (first_relation.type == Token_Type::less_equal)
      {
        var.set_lower_bound(first_value);
        if (maybe_second.type == Token_Type::less_equal ||
            maybe_second.type == Token_Type::greater_equal)
        {
          Token second_relation = p_tokenizer.next();
          if (second_relation.type != Token_Type::less_equal)
            parse_error("invalid chained bounds order");
          double upper_value = parse_numeric_value(p_tokenizer);
          var.set_upper_bound(upper_value);
        }
      }
      else
      {
        var.set_upper_bound(first_value);
        if (maybe_second.type == Token_Type::greater_equal)
        {
          p_tokenizer.next();
          double lower_value = parse_numeric_value(p_tokenizer);
          var.set_lower_bound(lower_value);
        }
      }
      continue;
    }
    Token var_token = p_tokenizer.next();
    if (var_token.type != Token_Type::identifier)
      parse_error("unexpected token in bounds");
    std::string var_name = var_token.text;
    size_t var_idx = m_model_manager->make_var(var_name, false);
    auto& var = m_model_manager->var(var_idx);
    Token next_token = p_tokenizer.peek();
    if (next_token.type == Token_Type::identifier)
    {
      std::string keyword_upper = to_upper(next_token.text);
      if (keyword_upper == "FREE")
      {
        p_tokenizer.next();
        var.set_lower_bound(k_neg_inf);
        var.set_upper_bound(k_inf);
        continue;
      }
    }
    Token relation = p_tokenizer.next();
    if (relation.type == Token_Type::less_equal)
    {
      double upper = parse_numeric_value(p_tokenizer);
      var.set_upper_bound(upper);
    }
    else if (relation.type == Token_Type::greater_equal)
    {
      double lower = parse_numeric_value(p_tokenizer);
      var.set_lower_bound(lower);
    }
    else if (relation.type == Token_Type::equal)
    {
      double value = parse_numeric_value(p_tokenizer);
      var.set_lower_bound(value);
      var.set_upper_bound(value);
      var.set_type(Var_Type::fixed);
    }
    else
      parse_error("invalid bounds operator");
  }
}

void LP_Reader::parse_integers(Tokenizer& p_tokenizer)
{
  Token keyword = p_tokenizer.next();
  (void)keyword;
  while (true)
  {
    Token token = p_tokenizer.peek();
    if (token.type == Token_Type::end)
      break;
    if (token.type == Token_Type::identifier)
    {
      std::string upper = to_upper(token.text);
      if (is_section_keyword(upper))
        break;
    }
    if (token.type == Token_Type::semicolon)
    {
      p_tokenizer.next();
      continue;
    }
    token = p_tokenizer.next();
    if (token.type != Token_Type::identifier)
      parse_error("invalid integer declaration");
    size_t var_idx = m_model_manager->make_var(token.text, false);
    auto& var = m_model_manager->var(var_idx);
    if (var.type() != Var_Type::binary)
      var.set_type(Var_Type::general_integer);
  }
}

void LP_Reader::parse_binaries(Tokenizer& p_tokenizer)
{
  Token keyword = p_tokenizer.next();
  (void)keyword;
  while (true)
  {
    Token token = p_tokenizer.peek();
    if (token.type == Token_Type::end)
      break;
    if (token.type == Token_Type::identifier)
    {
      std::string upper = to_upper(token.text);
      if (is_section_keyword(upper))
        break;
    }
    if (token.type == Token_Type::semicolon)
    {
      p_tokenizer.next();
      continue;
    }
    token = p_tokenizer.next();
    if (token.type != Token_Type::identifier)
      parse_error("invalid binary declaration");
    size_t var_idx = m_model_manager->make_var(token.text, false);
    auto& var = m_model_manager->var(var_idx);
    var.set_type(Var_Type::binary);
    if (var.lower_bound() < 0.0)
      var.set_lower_bound(0.0);
    if (var.upper_bound() > 1.0)
      var.set_upper_bound(1.0);
  }
}

void LP_Reader::add_term(const std::string& p_con_name,
                         const std::string& p_var_name,
                         double p_coeff)
{
  if (std::fabs(p_coeff) < k_zero_tolerance)
    return;
  size_t con_idx;
  Model_Con* con;
  if (p_con_name == m_model_manager->get_obj_name())
  {
    con_idx = 0;
    con = &m_model_manager->con(0);
  }
  else if (p_con_name.empty())
  {
    con_idx = 0;
    con = &m_model_manager->con(0);
  }
  else
  {
    con_idx = m_model_manager->con_idx(p_con_name);
    con = &m_model_manager->con(con_idx);
  }
  size_t var_idx = m_model_manager->make_var(p_var_name, false);
  auto& var = m_model_manager->var(var_idx);
  var.add_con(con_idx, con->term_num());
  double coeff = p_coeff;
  if (con_idx == 0)
    coeff *= m_model_manager->is_min();
  con->add_var(var_idx, coeff, var.term_num() - 1);
}

std::string LP_Reader::generate_constraint_name()
{
  return "lp_auto_con_" + std::to_string(m_auto_con_counter++);
}
