#include "pch.h"
#include "BaseApp.h"
#include "Utilities.h"

namespace ndtech {

  BaseApp::~BaseApp() {
    m_fileDataStore.Stop();
    m_fileDataStoreFiber.join();
    LOG(INFO) << "After fiber joined";
  }

  bool BaseApp::Initialize() {
    m_fileDataStore.RunOn(m_fileDataStoreFiber);
    return true;
  }

  void BaseApp::Run() {
    Loop();
  };

  bool BaseApp::AfterGraphicsInitialized() {
    return true;
  }

  void BaseApp::AfterWindowSet() {};



  std::vector<::byte> BaseApp::GetFileData(std::wstring shaderFileName) {

    if (!m_fileDataStore.ItemExists(shaderFileName)) {

      std::vector<::byte> shaderFileData = ndtech::utilities::ReadSmallBinaryFileSync(shaderFileName);
      m_fileDataStore.AddItem(shaderFileName, shaderFileData);

    }

    return m_fileDataStore.GetItem(shaderFileName);

  }

}
