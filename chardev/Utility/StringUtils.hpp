// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once 

#include <string>
#include <vector>

namespace Utils {
  /// <summary>
  /// This function provides a split version of provided string by provided delimiter characters.
  /// 
  /// This function return empty strings on multiple successive delimiter characters.
  /// </summary>
  /// <example>
  /// split(";a,,b;",";,") will result in ["","a","","b",""]
  /// </example>
  /// <typeparam name="delimiters_type">The type of the 'delimiters' parameter. Can be const char* or std::string</typeparam>
  /// <param name="string_to_split">The string to split.</param>
  /// <param name="delimiters">The delimiter characters.</param>
  /// <returns>A vector containing all substrings in string_to_split separated by characters from delimiters.</returns>
  template <typename delimiters_type>
static inline std::vector<std::string> split(const std::string& string_to_split, delimiters_type delimiters)
{
    std::vector<std::string> result;
    std::string::size_type start_token=0;
    while (true)
    {
        std::string::size_type end_token = string_to_split.find_first_of(delimiters, start_token);
        if (end_token == std::string::npos)
        {
          result.push_back(string_to_split.substr(start_token));
          break;
        }
        result.push_back(string_to_split.substr(start_token,end_token-start_token));
        start_token = end_token+1;
    }

    return result;
}

}