#include "pch.h"

#include "CameraResources.h"
#include "DirectXHelper.h"
#include "DeviceResources.h"
#include "Utilities.h"

#if _DEBUG
#include <sstream>
#include <string>
#include <iostream>
#endif

using namespace DirectX;
using namespace winrt;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Graphics::Holographic;
using namespace winrt::Windows::Perception::Spatial;
using namespace ndtech::utilities;

ndtech::CameraResources::CameraResources(winrt::Windows::Graphics::Holographic::HolographicCamera camera) :
  m_holographicCamera(camera),
  m_isStereo(camera.IsStereo()),
  m_d3dRenderTargetSize(camera.RenderTargetSize())
{
  m_d3dViewport = CD3D11_VIEWPORT(
    0.f, 0.f,
    m_d3dRenderTargetSize.Width,
    m_d3dRenderTargetSize.Height
  );
};


// Updates resources associated with a holographic camera's swap chain.
// The app does not access the swap chain directly, but it does create
// resource views for the back buffer.
void ndtech::CameraResources::CreateResourcesForBackBuffer(
  ndtech::DeviceResources* pDeviceResources,
  HolographicCameraRenderingParameters cameraParameters
)
{
  const auto device = pDeviceResources->GetD3DDevice();

  // Get the WinRT object representing the holographic camera's back buffer.
  IDirect3DSurface surface = cameraParameters.Direct3D11BackBuffer();

  // Get a DXGI interface for the holographic camera's back buffer.
  // Holographic cameras do not provide the DXGI swap chain, which is owned
  // by the system. The Direct3D back buffer resource is provided using WinRT
  // interop APIs.
  winrt::com_ptr<ID3D11Resource> resource;
  winrt::check_hresult(
    DirectXHelper::GetDXGIInterfaceFromObject(surface, IID_PPV_ARGS(resource.put()))
  );


#ifdef _DEBUG
  std::string surfaceResourceName("ndtech::CameraResources::surfaceResource");
  resource->SetPrivateData(WKPDID_D3DDebugObjectName, surfaceResourceName.size(), surfaceResourceName.c_str());
#endif

  // Get a Direct3D interface for the holographic camera's back buffer.
  winrt::com_ptr<ID3D11Texture2D> cameraBackBuffer;
  cameraBackBuffer = resource.as<ID3D11Texture2D>();

  // Determine if the back buffer has changed. If so, ensure that the render target view
  // is for the current back buffer.
  if (m_d3dBackBuffer.get() != cameraBackBuffer.get())
  {
    // This can change every frame as the system moves to the next buffer in the
    // swap chain. This mode of operation will occur when certain rendering modes
    // are activated.
    m_d3dBackBuffer = cameraBackBuffer;

    // Create a render target view of the back buffer.
    // Creating this resource is inexpensive, and is better than keeping track of
    // the back buffers in order to pre-allocate render target views for each one.

    m_d3dRenderTargetView = nullptr;

    winrt::check_hresult(
      device->CreateRenderTargetView(
        m_d3dBackBuffer.get(),
        nullptr,
        m_d3dRenderTargetView.put()
      )
    );

#ifdef _DEBUG
    std::string m_d3dRenderTargetViewName("ndtech::CameraResources::m_d3dRenderTargetView");
    m_d3dRenderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, m_d3dRenderTargetViewName.size(), m_d3dRenderTargetViewName.c_str());
#endif

    // Get the DXGI format for the back buffer.
    // This information can be accessed by the app using CameraResources::GetBackBufferDXGIFormat().
    D3D11_TEXTURE2D_DESC backBufferDesc;
    m_d3dBackBuffer->GetDesc(&backBufferDesc);
    m_dxgiFormat = backBufferDesc.Format;

    // Check for render target size changes.
    winrt::Windows::Foundation::Size currentSize = m_holographicCamera.RenderTargetSize();

    if (NotEqual(m_d3dRenderTargetSize, currentSize))
    {
      // Set render target size.
      m_d3dRenderTargetSize = currentSize;

      // A new depth stencil view is also needed.
      m_d3dDepthStencilView = nullptr;
    }
  }

  // Refresh depth stencil resources, if needed.
  if (m_d3dDepthStencilView.get() == nullptr)
  {
    //// Create a depth stencil view for use with 3D rendering if needed.
    //CD3D11_TEXTURE2D_DESC depthStencilDesc(
    //  DXGI_FORMAT_D16_UNORM,
    //  static_cast<UINT>(m_d3dRenderTargetSize.Width),
    //  static_cast<UINT>(m_d3dRenderTargetSize.Height),
    //  m_isStereo ? 2 : 1, // Create two textures when rendering in stereo.
    //  1, // Use a single mipmap level.
    //  D3D11_BIND_DEPTH_STENCIL
    //);
        
    // Create a depth stencil view for use with 3D rendering if needed.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
      DXGI_FORMAT_R16_TYPELESS,
      static_cast<UINT>(m_d3dRenderTargetSize.Width),
      static_cast<UINT>(m_d3dRenderTargetSize.Height),
      m_isStereo ? 2 : 1, // Create two textures when rendering in stereo.
      1, // Use a single mipmap level.
      D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
    );

    winrt::com_ptr<ID3D11Texture2D> depthStencil;
    winrt::check_hresult(
      device->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        depthStencil.put()
      )
    );

#ifdef _DEBUG
    std::string depthStencilName("ndtech::CameraResources::depthStencil");
    depthStencil->SetPrivateData(WKPDID_D3DDebugObjectName, depthStencilName.size(), depthStencilName.c_str());
#endif

    //CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
    //  m_isStereo ? D3D11_DSV_DIMENSION_TEXTURE2DARRAY : D3D11_DSV_DIMENSION_TEXTURE2D
    //);
    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
      m_isStereo ? D3D11_DSV_DIMENSION_TEXTURE2DARRAY : D3D11_DSV_DIMENSION_TEXTURE2D,
      DXGI_FORMAT_D16_UNORM
    );

    winrt::check_hresult(
      device->CreateDepthStencilView(
        depthStencil.get(),
        &depthStencilViewDesc,
        m_d3dDepthStencilView.put()
      )
    );
  }

#ifdef _DEBUG
  std::string m_d3dDepthStencilViewName("ndtech::CameraResources::m_d3dDepthStencilView");
  m_d3dDepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, m_d3dDepthStencilViewName.size(), m_d3dDepthStencilViewName.c_str());
#endif

  // Create the constant buffer, if needed.
  if (m_viewProjectionConstantBuffer.get() == nullptr)
  {
    // Create a constant buffer to store view and projection matrices for the camera.
    CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    winrt::check_hresult(
      device->CreateBuffer(
        &constantBufferDesc,
        nullptr,
        m_viewProjectionConstantBuffer.put()
      )
    );

#ifdef _DEBUG
    std::string m_viewProjectionConstantBufferName("ndtech::CameraResources::m_viewProjectionConstantBuffer");
    m_viewProjectionConstantBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, m_viewProjectionConstantBufferName.size(), m_viewProjectionConstantBufferName.c_str());
#endif
  }
}

// Releases resources associated with a back buffer.
void ndtech::CameraResources::ReleaseResourcesForBackBuffer(ndtech::DeviceResources* pDeviceResources)
{
  const auto context = pDeviceResources->GetD3DDeviceContext();


  // Release camera-specific resources.
  m_d3dBackBuffer = nullptr;
  m_d3dRenderTargetView = nullptr;
  m_d3dDepthStencilView = nullptr;
  m_viewProjectionConstantBuffer = nullptr;

  // Ensure system references to the back buffer are released by clearing the render
  // target from the graphics pipeline state, and then flushing the Direct3D context.
  ID3D11RenderTargetView* nullViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };
  context->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
  context->Flush();
}

// Updates the view/projection constant buffer for a holographic camera.
void ndtech::CameraResources::UpdateViewProjectionBuffer(
  std::shared_ptr<DeviceResources> deviceResources,
  HolographicCameraPose cameraPose,
  SpatialCoordinateSystem coordinateSystem
)
{
  // The system changes the viewport on a per-frame basis for system optimizations.
  auto viewport = cameraPose.Viewport();
  m_d3dViewport = CD3D11_VIEWPORT(
    viewport.X,
    viewport.Y,
    viewport.Width,
    viewport.Height
  );

  // The projection transform for each frame is provided by the HolographicCameraPose.
  HolographicStereoTransform cameraProjectionTransform = cameraPose.ProjectionTransform();

  // Get a container object with the view and projection matrices for the given
  // pose in the given coordinate system.
  winrt::Windows::Foundation::IReference<HolographicStereoTransform> viewTransformContainer = cameraPose.TryGetViewTransform(coordinateSystem);

  // If TryGetViewTransform returns a null pointer, that means the pose and coordinate
  // system cannot be understood relative to one another; content cannot be rendered
  // in this coordinate system for the duration of the current frame.
  // This usually means that positional tracking is not active for the current frame, in
  // which case it is possible to use a SpatialLocatorAttachedFrameOfReference to render
  // content that is not world-locked instead.
  ndtech::ViewProjectionConstantBuffer viewProjectionConstantBufferData;
  bool viewTransformAcquired = viewTransformContainer != nullptr;
  if (viewTransformAcquired)
  {
    // Otherwise, the set of view transforms can be retrieved.
    HolographicStereoTransform viewCoordinateSystemTransform = viewTransformContainer.Value();

    XMMATRIX cameraWorldMatrix = XMMATRIX(
      viewCoordinateSystemTransform.Left.m11,
      viewCoordinateSystemTransform.Left.m12,
      viewCoordinateSystemTransform.Left.m13,
      viewCoordinateSystemTransform.Left.m14,
      viewCoordinateSystemTransform.Left.m21,
      viewCoordinateSystemTransform.Left.m22,
      viewCoordinateSystemTransform.Left.m23,
      viewCoordinateSystemTransform.Left.m24,
      viewCoordinateSystemTransform.Left.m31,
      viewCoordinateSystemTransform.Left.m32,
      viewCoordinateSystemTransform.Left.m33,
      viewCoordinateSystemTransform.Left.m34,
      viewCoordinateSystemTransform.Left.m41,
      viewCoordinateSystemTransform.Left.m42,
      viewCoordinateSystemTransform.Left.m43,
      viewCoordinateSystemTransform.Left.m44
      );

    XMMATRIX viewProjectionMatrix = XMMatrixTranspose(XMLoadFloat4x4(&viewCoordinateSystemTransform.Left) * XMLoadFloat4x4(&cameraProjectionTransform.Left));

    // Update the view matrices. Holographic cameras (such as Microsoft HoloLens) are
    // constantly moving relative to the world. The view matrices need to be updated
    // every frame.
    XMStoreFloat4x4(
      &viewProjectionConstantBufferData.viewProjection[0],
      XMMatrixTranspose(XMLoadFloat4x4(&viewCoordinateSystemTransform.Left) * XMLoadFloat4x4(&cameraProjectionTransform.Left))
    );
    XMStoreFloat4x4(
      &viewProjectionConstantBufferData.viewProjection[1],
      XMMatrixTranspose(XMLoadFloat4x4(&viewCoordinateSystemTransform.Right) * XMLoadFloat4x4(&cameraProjectionTransform.Right))
    );

//#if _DEBUG
//    std::stringstream ss;
//    ss << "viewProjectConstantBufferData.viewProjection[0] " << "\n"
//      << viewProjectionConstantBufferData.viewProjection[0]._11 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._12 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._13 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._14 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._21 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._22 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._23 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._34 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._31 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._32 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._33 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._34 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._41 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._42 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._43 << ","
//      << viewProjectionConstantBufferData.viewProjection[0]._44 << "\n";
//
//    OutputDebugStringA(ss.str().c_str());
//
//    ss = std::stringstream();
//
//
//    ss << "viewProjectConstantBufferData.viewProjection[1] " << "\n"
//      << viewProjectionConstantBufferData.viewProjection[1]._11 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._12 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._13 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._14 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._21 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._22 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._23 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._34 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._31 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._32 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._33 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._34 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._41 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._42 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._43 << ","
//      << viewProjectionConstantBufferData.viewProjection[1]._44 << "\n";
//
//    OutputDebugStringA(ss.str().c_str());
//#endif
  }

  // Use the D3D device context to update Direct3D device-based resources.
  const auto context = deviceResources->GetD3DDeviceContext();

  // Loading is asynchronous. Resources must be created before they can be updated.
  if (context == nullptr || m_viewProjectionConstantBuffer.get() == nullptr || !viewTransformAcquired)
  {
    m_framePending = false;
  }
  else
  {
    // Update the view and projection matrices.
    context->UpdateSubresource(
      m_viewProjectionConstantBuffer.get(),
      0,
      nullptr,
      &viewProjectionConstantBufferData,
      0,
      0
    );

    m_framePending = true;
  }
}

// Gets the view-projection constant buffer for the HolographicCamera and attaches it
// to the shader pipeline.
bool ndtech::CameraResources::AttachViewProjectionBuffer(
  std::shared_ptr<DeviceResources>& deviceResources
)
{
  // This method uses Direct3D device-based resources.
  const auto context = deviceResources->GetD3DDeviceContext();

  // Loading is asynchronous. Resources must be created before they can be updated.
  // Cameras can also be added asynchronously, in which case they must be initialized
  // before they can be used.
  if (context == nullptr || m_viewProjectionConstantBuffer.get() == nullptr || m_framePending == false)
  {
    return false;
  }

  // Set the viewport for this camera.
  context->RSSetViewports(1, &m_d3dViewport);

  ID3D11Buffer * buffer = m_viewProjectionConstantBuffer.get();

  // Send the constant buffer to the vertex shader.
  context->VSSetConstantBuffers(
    1,
    1,
    &buffer
  );

  // The template includes a pass-through geometry shader that is used by
  // default on systems that don't support the D3D11_FEATURE_D3D11_OPTIONS3::
  // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer extension. The shader 
  // will be enabled at run-time on systems that require it.
  // If your app will also use the geometry shader for other tasks and those
  // tasks require the view/projection matrix, uncomment the following line 
  // of code to send the constant buffer to the geometry shader as well.
  /*context->GSSetConstantBuffers(
  1,
  1,
  m_viewProjectionConstantBuffer.GetAddressOf()
  );*/

  m_framePending = false;

  return true;
}
