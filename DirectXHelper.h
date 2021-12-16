#pragma once

#include <inspectable.h>
#include <dxgi.h>
//#include "IAsyncSpecializations.h"
#include <future>
#include <boost/fiber/all.hpp>

#include "winrt/Windows.Storage.Streams.h"

STDAPI CreateDirect3D11DeviceFromDXGIDevice(
  _In_         IDXGIDevice* dxgiDevice,
  _COM_Outptr_ IInspectable** graphicsDevice);


namespace ndtech
{
  struct DirectXHelper {

    struct IndexedInstancedIndirectCallArgs {
      UINT IndexCountPerInstance;
      UINT InstanceCount;
      UINT StartIndexLocation;
      INT  BaseVertexLocation;
      UINT StartInstanceLocation;
    };

#if defined(_DEBUG)
    // Check for SDK Layer support.
    static inline bool SdkLayersAvailable()
    {
      HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
        0,
        D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
        nullptr,                    // Any feature level will do.
        0,
        D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
        nullptr,                    // No need to keep the D3D device reference.
        nullptr,                    // No need to know the feature level.
        nullptr                     // No need to keep the D3D device context reference.
      );

      return SUCCEEDED(hr);
    }
#endif

#if defined(__cplusplus)
    interface __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
      IDirect3DDxgiInterfaceAccess : public IUnknown
    {
      IFACEMETHOD(GetInterface)(REFIID iid, _COM_Outptr_ void** p) = 0;
    };
#endif /* __cplusplus */

    static inline HRESULT GetDXGIInterfaceFromObject(
      _In_         winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface object,
      _In_         REFIID iid,
      _COM_Outptr_ void** p)
    {
      winrt::com_ptr<IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess = object.as<IDirect3DDxgiInterfaceAccess>();
      return dxgiInterfaceAccess->GetInterface(iid, p);
    }
    


    static void CreateBuffer(ID3D11Device4* d3dDevice, winrt::com_ptr<ID3D11Buffer> & buffer, UINT byteWidth, D3D11_USAGE usage, UINT bindFlags, UINT cpuAccessFlags, UINT miscFlags, UINT structureByteStride)
    {
      D3D11_BUFFER_DESC bufferDesc;
      ZeroMemory(&bufferDesc, sizeof(bufferDesc));
      bufferDesc.ByteWidth = byteWidth;
      bufferDesc.Usage = usage;
      bufferDesc.BindFlags = bindFlags;
      bufferDesc.CPUAccessFlags = cpuAccessFlags;
      bufferDesc.MiscFlags = miscFlags;
      bufferDesc.StructureByteStride = structureByteStride;

      winrt::check_hresult(
        d3dDevice->CreateBuffer(
          &bufferDesc,
          nullptr,
          buffer.put()
        ));
    }


    static void CreateCommandBuffer(ID3D11Device4* d3dDevice, winrt::com_ptr<ID3D11Buffer>& commandBuffer)
    {
      CreateBuffer(d3dDevice, commandBuffer, sizeof(UINT) * 5, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS, 0, D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS, sizeof(float));
    }


    static void CreateUnorderedAcceesViewBuffer(ID3D11Device4* d3dDevice, winrt::com_ptr<ID3D11Buffer>& buffer, winrt::com_ptr<ID3D11UnorderedAccessView>& uav, DXGI_FORMAT format, UINT numElements, UINT flags = 0, UINT firstElement = 0)
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
      ZeroMemory(&uavDesc, sizeof(uavDesc));
      uavDesc.Format = format;
      uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
      uavDesc.Buffer.FirstElement = 0;
      uavDesc.Buffer.Flags = 0;
      uavDesc.Buffer.NumElements = numElements;

      winrt::check_hresult(
        d3dDevice->CreateUnorderedAccessView(
          buffer.get(),
          &uavDesc,
          uav.put()
        ));
    }

    static void CreateShaderResourceView(ID3D11Device4* d3dDevice, winrt::com_ptr<ID3D11Buffer>& buffer, winrt::com_ptr<ID3D11ShaderResourceView>& srv, DXGI_FORMAT format, UINT numElements, UINT firstElement = 0) {

      D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
      ZeroMemory(&srvDesc, sizeof(srvDesc));
      srvDesc.Format = format;
      srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      srvDesc.Buffer.FirstElement = firstElement;
      srvDesc.Buffer.NumElements = numElements;

      winrt::check_hresult(
        d3dDevice->CreateShaderResourceView(
          buffer.get(),
          &srvDesc,
          srv.put()
        ));
    }

  };

}