#pragma once

#include <string>
#include <sstream>
#include <ostream>
#include <locale>
#include <codecvt>

namespace ndtech
{
    /* static */
    template<class T>
    T string_to_T(const std::string& s)
    {
        // Convert from a string to a T
        // Type T must support >> operator
        T t;
        std::istringstream ist(s);
        ist >> t;
        return t;
    }


    /* static */
    template<>
    inline std::string string_to_T<std::string>(const std::string& s)
    {
        // Convert from a string to a string
        // In other words, do nothing
        return s;
    }


    /* static */
    template<>
    inline bool string_to_T<bool>(const std::string& s)
    {
        // Convert from a string to a bool
        // Interpret "false", "F", "no", "n", "0" as false
        // Interpret "true", "T", "yes", "y", "1", "-1", or anything else as true
        bool b = true;
        std::string sup = s;
        for (std::string::iterator p = sup.begin(); p != sup.end(); ++p)
            *p = (char)toupper(*p);  // make string all caps
        if (sup == std::string("FALSE") || sup == std::string("F") ||
            sup == std::string("NO") || sup == std::string("N") ||
            sup == std::string("0") || sup == std::string("NONE"))
            b = false;
        return b;
    }


    /* static */
    template<>
    inline std::wstring string_to_T<std::wstring>(const std::string& s)
    {
        std::wstringstream wss;
        wss << s.c_str();
        return wss.str();
    }

    /* static */
    template<class T>
    inline std::string T_to_string(const T& t)
    {
        // Convert from a T to a string
        // Type T must support << operator
        std::ostringstream ost;
        ost << t;
        return ost.str();
    }

    template<>
    inline std::string T_to_string<std::wstring>(const std::wstring& wideString) {
      using convert_typeX = std::codecvt_utf8<wchar_t>;
      std::wstring_convert<convert_typeX, wchar_t> converterX;

      return converterX.to_bytes(wideString);
    }

};
