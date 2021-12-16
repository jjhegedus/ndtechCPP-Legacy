#pragma once
#include "pch.h"
#include "Conversion.h"
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iterator>
#include "Conversion.h"

#if NDTECH_ML
#include "Utilities-MagicLeap.h"
#elif NDTECH_HOLO
#include "Utilities-HoloLens.h"
#endif


namespace ndtech
{
  namespace utilities {

    static const float pi = 3.1415926535897932384626433832f;
    
    using namespace std::chrono;
    using namespace std::chrono_literals;

    template <typename FunctionToCall, typename... Args>
    decltype(auto) CallAfterTimeDuration(std::chrono::duration<int, std::ratio<1, 100000000>> duration, FunctionToCall functionToCall, Args... args) {
      std::this_thread::sleep_for(duration);
      functionToCall(args...);
    };

    static std::vector<::byte> ReadSmallBinaryFileSync(std::wstring filePath) {

      // Open the file in binary mode
      std::ifstream fileStream(T_to_string(filePath), std::ios::binary);

      // Don't skip whitespace
      fileStream.unsetf(std::ios::skipws);

      // Move to the end to get the file to get the size
      fileStream.seekg(0, std::ios::end);
      std::streampos fileSize = fileStream.tellg();

      // reserve capacity according to the size of the file
      std::vector<::byte> fileData;
      fileData.reserve(fileSize);

      // Move back to the beginning to read the whole file
      fileStream.seekg(0, std::ios::beg);

      // read the data:
      fileData.insert(fileData.begin(),
        std::istream_iterator<::byte>(fileStream),
        std::istream_iterator<::byte>());

      return fileData;

    }


    static std::string ReadSmallTextFileSync_old(std::wstring filePath) {
      std::string fileData;
      std::ifstream fileStream(T_to_string(filePath), std::ios::in);
      if (fileStream.is_open()) {
        std::stringstream sstr;
        sstr << fileStream.rdbuf();
        fileData = sstr.str();
        fileStream.close();
      }
      else {
        LOG(FATAL) << "Impossible to open " << T_to_string(filePath) << ". Are you in the right directory? Don't forget to read the FAQ !\n";
        return std::string("");
      }
    }


    static std::string ReadSmallTextFileSync(std::wstring filePath) {

      // Open the file in text mode
      std::ifstream fileStream(T_to_string(filePath), std::ios::in);

      // Don't skip whitespace
      fileStream.unsetf(std::ios::skipws);
      
      // Move to the end to get the file to get the size
      fileStream.seekg(0, std::ios::end);
      std::streampos fileSize = fileStream.tellg();

      // reserve capacity according to the size of the file
      std::string fileData;
      fileData.reserve(fileSize);

      // Move back to the beginning to read the whole file
      fileStream.seekg(0, std::ios::beg);

      // read the data:
      fileData.insert(fileData.begin(),
        std::istream_iterator<char>(fileStream),
        std::istream_iterator<char>());

      return fileData;

    }

  }

}