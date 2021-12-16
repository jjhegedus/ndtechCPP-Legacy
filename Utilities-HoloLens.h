#pragma once

#include "pch.h"
#include <future>
#include <boost/fiber/all.hpp>
#include "Utilities.h"

namespace ndtech {
  namespace utilities {

    //Returns the last Win32 error, in string format. Returns an empty string if there is no error.
    inline std::string GetLastErrorAsString()
    {
      //Get the error message, if any.
      DWORD errorMessageID = ::GetLastError();
      if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

      LPSTR messageBuffer = nullptr;
      size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

      std::string message(messageBuffer, size);

      //Free the buffer.
      LocalFree(messageBuffer);

      return message;
    }

    inline std::string GetHRAsString(HRESULT hr)
    {
      LPSTR messageBuffer = nullptr;
      size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

      std::string message(messageBuffer, size);

      //Free the buffer.
      LocalFree(messageBuffer);

      return message;
    }

    // Synchronous call for reading data from a file using ReadBufferAsync
    inline std::vector<::byte> ReadDataFiber(const std::wstring& filename) {
      using namespace winrt::Windows::Storage;
      using namespace winrt::Windows::Storage::Streams;
      boost::fibers::promise<std::vector<::byte>> promise;
      boost::fibers::future<std::vector<::byte>> future(promise.get_future());

      winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::Streams::IBuffer> aio = PathIO::ReadBufferAsync(filename);
      aio.Completed(
        [promise = std::move(promise)]
      (winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::Streams::IBuffer> op,
        winrt::Windows::Foundation::AsyncStatus const status) mutable {
        //promise.set_value(op.GetResults());
        if (status == winrt::Windows::Foundation::AsyncStatus::Error) {

          std::string errorString = ndtech::utilities::GetHRAsString(op.ErrorCode());
          LOG(INFO) << "ndtech::utilities::ReadDataFiber received an Error event.  The ErrorCode = " << op.ErrorCode() << " ErrorString = " << errorString << " . Kill the program as we can't complete without the data.";
          promise.set_exception(std::make_exception_ptr(std::exception(errorString.c_str())));
        }
        else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled) {

          LOG(INFO) << "ndtech::utilities::ReadDataFiber received a Cancelled event.  This should never happen, but if it does, return an empty buffer.";
          promise.set_value(std::vector<::byte>());
        }
        else if (status == winrt::Windows::Foundation::AsyncStatus::Started) {
          LOG(INFO) << "ndtech::utilities::ReadDataFiber received a Started event.  Keep reading until complete";
        }
        else if (status == winrt::Windows::Foundation::AsyncStatus::Completed) {

          IBuffer fileBuffer = op.GetResults();
          std::vector<::byte> returnBuffer;
          returnBuffer.resize(fileBuffer.Length());
          DataReader::FromBuffer(fileBuffer).ReadBytes(winrt::array_view<uint8_t>(returnBuffer));
          promise.set_value(returnBuffer);
        }
      });

      return future.get();
    }

    // Async data reader using co_await
    inline std::future<std::vector<::byte>> ReadDataCoAwait(const std::wstring& filename)
    {
      winrt::Windows::Storage::Streams::IBuffer fileBuffer = co_await winrt::Windows::Storage::PathIO::ReadBufferAsync(winrt::hstring{ filename.c_str() });

      std::vector<::byte> returnBuffer;
      returnBuffer.resize(fileBuffer.Length());
      winrt::Windows::Storage::Streams::DataReader::FromBuffer(fileBuffer).ReadBytes(winrt::array_view<::byte>(returnBuffer));
      return returnBuffer;
    }


    // Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
    inline float ConvertDipsToPixels(float dips, float dpi)
    {
      constexpr float dipsPerInch = 96.0f;
      return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
    }

    static bool inline NotEqual(const winrt::Windows::Foundation::Size lhs, const winrt::Windows::Foundation::Size rhs)
    {
      if (lhs.Height != rhs.Height)
        return false;

      if (lhs.Width != rhs.Width)
        return false;

      return true;
    }

    // TODO:  JJH: This won't work because StorageFile doesn't have a default constructor
    //             Not sure how to work around this just yet

    //inline std::future<winrt::Windows::Storage::StorageFile> GetCreateFile(const wchar_t* fileName)
    //{
    //    winrt::Windows::Storage::StorageFile file =
    //        await winrt::Windows::Storage::ApplicationData::Current().LocalFolder().CreateFileAsync(
    //            fileName,
    //            winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting);

    //    return file;
    //}

    inline winrt::Windows::Foundation::IAsyncAction CreateFile(const wchar_t* fileName)
    {
      winrt::Windows::Storage::StorageFile sampleFile =
        await winrt::Windows::Storage::ApplicationData::Current().LocalFolder().CreateFileAsync(
          fileName,
          winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting);
    }

    inline std::future<void> WriteFile(winrt::Windows::Storage::StorageFile file, winrt::array_view<const uint8_t> buffer)
    {
      winrt::Windows::Storage::FileIO::WriteBytesAsync(file, buffer);
    }


    inline void OutputDebugThreadId() {
      DWORD threadId = GetCurrentThreadId();
      std::stringbuf strBuf;
      strBuf.sputn("ThreadId = ", strlen("ThreadId = "));
      strBuf.sputn(T_to_string(threadId).c_str(), strlen(T_to_string(threadId).c_str()));
      strBuf.sputn("\n", strlen("\n"));
      OutputDebugStringA(strBuf.str().c_str());
    }

    // Gets the current number of ticks from QueryPerformanceCounter. Throws an
    // exception if the call to QueryPerformanceCounter fails.
    static inline void OutputDebugTicks()
    {

      char buf[60];

      LARGE_INTEGER ticks;
      QueryPerformanceCounter(&ticks);

      sprintf_s(buf, 60, "ndtech::Utilities::OutputDebugTics(): ticks = %I64u\n", ticks.QuadPart);

      LARGE_INTEGER ticks2;
      QueryPerformanceCounter(&ticks2);

      OutputDebugStringA(buf);

      LARGE_INTEGER ticks3;
      QueryPerformanceCounter(&ticks3);
    }

    inline std::future<void> OutputDebugTicksCo()
    {

      char buf[60];

      LARGE_INTEGER ticks;
      QueryPerformanceCounter(&ticks);

      co_await winrt::resume_background();

      sprintf_s(buf, 60, "ndtech::Utilities::OutputDebugTics(): ticks = %I64u\n", ticks.QuadPart);

      LARGE_INTEGER ticks2;
      QueryPerformanceCounter(&ticks2);

      OutputDebugStringA(buf);

      LARGE_INTEGER ticks3;
      QueryPerformanceCounter(&ticks3);
    }

    static inline int VDebugPrintF(const char* format, va_list arglist) {
      const UINT32 MAX_CHARS = 1024;
      static char s_buffer[MAX_CHARS];

      int charsWritten = vsnprintf_s(s_buffer, MAX_CHARS, format, arglist);

      OutputDebugStringA(s_buffer);

      return charsWritten;
    }

    static inline int DebugPrintF(const char* format, ...) {
      va_list args;
      va_start(args, format);

      int charsWritten = VDebugPrintF(format, args);

      va_end(args);

      return charsWritten;
    }

    static inline void OutputDebugTicks2()
    {
      LARGE_INTEGER ticks;
      QueryPerformanceCounter(&ticks);

      DebugPrintF("ndtech::Utilities::OutputDebugTics2(): ticks = %I64u\n", ticks.QuadPart);
    }

    static inline void debugTimeStart(const char* marker) {
      OutputDebugStringA(marker);
      //OutputDebugTicksCo();
    }

    static inline void debugTimeEnd(const char* marker) {
      OutputDebugTicks();
      OutputDebugStringA(marker);
    }

    static inline auto getBenchmarkerOld() {
      return ([](auto lambda, int iterations)
        {
          int64_t beginCycles = __rdtsc();
          for (int i = 0; i < iterations; i++)
          {
            lambda();
          }

          return ((__rdtsc() - beginCycles) / iterations);
        });
    }

    static inline auto getBenchmarkerOld2() {
      return ([](auto lambda, int iterations)
        {
          int cpuInfo[4] = { -1 };
          unsigned int ui;

          __cpuid(cpuInfo, 0);
          int64_t beginCycles = __rdtscp(&ui);
          for (int i = 0; i < iterations; i++)
          {
            lambda();
          }

          int64_t endCycles = __rdtscp(&ui);
          __cpuid(cpuInfo, 0);

          return ((endCycles - beginCycles) / iterations);
        });
    }

    static inline auto getBenchmarker() {
      return ([](auto lambda, int64_t iterations)
        {
          unsigned int ui;
          int64_t beginCycles = __rdtscp(&ui);
          for (int i = 0; i < iterations; i++)
          {
            lambda();
          }

          int64_t endCycles = __rdtscp(&ui);

          return ((endCycles - beginCycles) / iterations);
        });
    }

  }
}