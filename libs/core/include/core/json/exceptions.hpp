#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/tokenizer/token.hpp"

#include <exception>
#include <sstream>
#include <string>
#include <utility>

namespace fetch {
namespace json {

class UnrecognisedJSONSymbolException : public std::exception
{
  std::string str_;

public:
  UnrecognisedJSONSymbolException(byte_array::Token const &token)
  {
    std::stringstream msg;
    msg << "Unrecognised symbol '";
    msg << token << '\'' << " at line " << token.line() << ", character " << token.character()
        << "\n";
    str_ = msg.str();
  }

  virtual char const *what() const throw()
  {
    return str_.c_str();
  }
};

class JSONParseException : public std::exception
{
public:
  JSONParseException(std::string err)
    : error_(std::move(err))
  {}
  virtual ~JSONParseException()
  {}
  virtual char const *what() const throw()
  {
    return error_.c_str();
  }

private:
  std::string error_;
};
}  // namespace json
}  // namespace fetch
