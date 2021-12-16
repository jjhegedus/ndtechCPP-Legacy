#include "DistanceFieldRenderer.h"
#include "Utilities.h"

ndtech::DistanceFieldRenderer::DistanceFieldRenderer(DeviceResources* deviceResources, unsigned int const& textureWidth, unsigned int const& textureHeight)
  : m_deviceResources(deviceResources),
  m_textureWidth(textureWidth),
  m_textureHeight(textureHeight)
{
  CreateDeviceDependentResources();
  CreateDeviceDependentResourcesPart2();
};

winrt::Windows::Foundation::IAsyncAction ndtech::DistanceFieldRenderer::CreateDeviceDependentResources()
{
  using namespace winrt;

  co_await winrt::resume_background();

  // Create the texture that will be used as the offscreen render target.
  CD3D11_TEXTURE2D_DESC textureDesc(
    DXGI_FORMAT_R8G8_UNORM,
    m_textureWidth,
    m_textureHeight,
    1,
    1,
    D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
  );
  m_deviceResources->GetD3DDevice()->CreateTexture2D(&textureDesc, nullptr, m_texture.put());

#ifdef _DEBUG
  std::string m_textureName("ndtech::DistanceFieldRenderer::m_texture");
  m_texture->SetPrivateData(WKPDID_D3DDebugObjectName, m_textureName.size(), m_textureName.c_str());
#endif

            // Create read and write views for the offscreen render target.
  m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_texture.get(), nullptr, m_shaderResourceView.put());
#ifdef _DEBUG
  std::string m_shaderresourceViewName("ndtech::DistanceFieldRenderer::m_shaderresourceView");
  m_shaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, m_shaderresourceViewName.size(), m_shaderresourceViewName.c_str());
#endif
  m_deviceResources->GetD3DDevice()->CreateRenderTargetView(m_texture.get(), nullptr, m_renderTargetView.put());

#ifdef _DEBUG
  std::string m_renderTargetViewName("ndtech::DistanceFieldRenderer::m_renderTargetView");
  m_shaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, m_renderTargetViewName.size(), m_renderTargetViewName.c_str());
#endif

            // Create a depth stencil view.
            // NOTE: This is used only for drawing. It could be recycled for additional distance rendering 
            //       passes, and discarded after all distance fields have been created (as could the index 
            //       and vertex buffers).
  CD3D11_TEXTURE2D_DESC depthStencilDesc(
    DXGI_FORMAT_D16_UNORM,
    m_textureWidth,
    m_textureHeight,
    1, // One array level.
    1, // Use a single mipmap level.
    D3D11_BIND_DEPTH_STENCIL
  );

  com_ptr<ID3D11Texture2D> depthStencil;
  check_hresult(
    m_deviceResources->GetD3DDevice()->CreateTexture2D(
      &depthStencilDesc,
      nullptr,
      depthStencil.put()
    )
  );

  CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
  check_hresult(
    m_deviceResources->GetD3DDevice()->CreateDepthStencilView(
      depthStencil.get(),
      &depthStencilViewDesc,
      m_d3dDepthStencilView.put()
    )
  );


  //// Load shaders asynchronously.
  //std::vector<byte> vertexShaderFileData = co_await ndtech::utilities::ReadDataCoAwait(L"ms-appx:///FullscreenQuadVertexShader.cso");

  //// After the vertex shader file is loaded, create the shader and input layout.
  //check_hresult(
  //    m_deviceResources->GetD3DDevice()->CreateVertexShader(
  //        vertexShaderFileData.data(),
  //        vertexShaderFileData.size(),
  //        nullptr,
  //        put(m_vertexShader)
  //    )
  //);

  //constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1> vertexDesc =
  //{ {
  //    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
  //    } };

  //check_hresult(
  //    m_deviceResources->GetD3DDevice()->CreateInputLayout(
  //        vertexDesc.data(),
  //        vertexDesc.size(),
  //        vertexShaderFileData.data(),
  //        vertexShaderFileData.size(),
  //        put(m_inputLayout)
  //    )
  //);

  //std::vector<byte> pixelShaderFileData = co_await ndtech::utilities::ReadDataCoAwait(L"ms-appx:///CreateDistanceFieldPixelShader.cso");
  //// After the pixel shader file is loaded, create the shader and constant buffer.

  //check_hresult(
  //    m_deviceResources->GetD3DDevice()->CreatePixelShader(
  //        pixelShaderFileData.data(),
  //        pixelShaderFileData.size(),
  //        nullptr,
  //        put(m_pixelShader)
  //    )
  //);


  //// Once all shaders are loaded, create the mesh.

  //// Load mesh vertices for a full-screen quad.
  //static const std::array<DirectX::XMFLOAT2, 4> quadVertices =
  //{ {
  //    { DirectX::XMFLOAT2(-1.0f,  1.0f) },
  //    { DirectX::XMFLOAT2(1.0f,  1.0f) },
  //    { DirectX::XMFLOAT2(1.0f, -1.0f) },
  //    { DirectX::XMFLOAT2(-1.0f, -1.0f) },
  //    } };

  //m_vertexStride = sizeof(DirectX::XMFLOAT2);

  //D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
  //vertexBufferData.pSysMem = quadVertices.data();
  //vertexBufferData.SysMemPitch = 0;
  //vertexBufferData.SysMemSlicePitch = 0;
  //const CD3D11_BUFFER_DESC vertexBufferDesc(m_vertexStride * quadVertices.size(), D3D11_BIND_VERTEX_BUFFER);
  //check_hresult(
  //    m_deviceResources->GetD3DDevice()->CreateBuffer(
  //        &vertexBufferDesc,
  //        &vertexBufferData,
  //        put(m_vertexBuffer)
  //    )
  //);

  //// Load mesh indices. Each trio of indices represents
  //// a triangle to be rendered on the screen.
  //// For example: 2,1,0 means that the vertices with indexes
  //// 2, 1, and 0 from the vertex buffer compose the
  //// first triangle of this mesh.
  //// Note that the winding order is clockwise by default.
  //constexpr std::array<unsigned short, 6> quadIndices =
  //{ {
  //        // -z
  //        0,2,3,
  //        0,1,2,
  //    } };

  //m_indexCount = quadIndices.size();

  //D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
  //indexBufferData.pSysMem = quadIndices.data();
  //indexBufferData.SysMemPitch = 0;
  //indexBufferData.SysMemSlicePitch = 0;
  //const CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short) * quadIndices.size(), D3D11_BIND_INDEX_BUFFER);
  //check_hresult(
  //    m_deviceResources->GetD3DDevice()->CreateBuffer(
  //        &indexBufferDesc,
  //        &indexBufferData,
  //        put(m_indexBuffer)
  //    )
  //);



  //m_loadingComplete = true;

};

winrt::Windows::Foundation::IAsyncAction ndtech::DistanceFieldRenderer::CreateDeviceDependentResourcesPart2()
{
  using namespace winrt;

  // co_await winrt::resume_background();


  // Load shaders asynchronously.
  std::vector<::byte> vertexShaderFileData = co_await ndtech::utilities::ReadDataCoAwait(L"ms-appx:///FullscreenQuadVertexShader.cso");

  // After the vertex shader file is loaded, create the shader and input layout.
  check_hresult(
    m_deviceResources->GetD3DDevice()->CreateVertexShader(
      vertexShaderFileData.data(),
      vertexShaderFileData.size(),
      nullptr,
      m_vertexShader.put()
    )
  );

  constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1> vertexDesc =
  { {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
      } };

  check_hresult(
    m_deviceResources->GetD3DDevice()->CreateInputLayout(
      vertexDesc.data(),
      vertexDesc.size(),
      vertexShaderFileData.data(),
      vertexShaderFileData.size(),
      m_inputLayout.put()
    )
  );

  std::vector<::byte> pixelShaderFileData = co_await ndtech::utilities::ReadDataCoAwait(L"ms-appx:///CreateDistanceFieldPixelShader.cso");
  // After the pixel shader file is loaded, create the shader and constant buffer.

  check_hresult(
    m_deviceResources->GetD3DDevice()->CreatePixelShader(
      pixelShaderFileData.data(),
      pixelShaderFileData.size(),
      nullptr,
      m_pixelShader.put()
    )
  );


  // Once all shaders are loaded, create the mesh.

  // Load mesh vertices for a full-screen quad.
  static const std::array<DirectX::XMFLOAT2, 4> quadVertices =
  { {
      { DirectX::XMFLOAT2(-1.0f,  1.0f) },
      { DirectX::XMFLOAT2(1.0f,  1.0f) },
      { DirectX::XMFLOAT2(1.0f, -1.0f) },
      { DirectX::XMFLOAT2(-1.0f, -1.0f) },
      } };

  m_vertexStride = sizeof(DirectX::XMFLOAT2);

  D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
  vertexBufferData.pSysMem = quadVertices.data();
  vertexBufferData.SysMemPitch = 0;
  vertexBufferData.SysMemSlicePitch = 0;
  const CD3D11_BUFFER_DESC vertexBufferDesc(m_vertexStride * quadVertices.size(), D3D11_BIND_VERTEX_BUFFER);
  check_hresult(
    m_deviceResources->GetD3DDevice()->CreateBuffer(
      &vertexBufferDesc,
      &vertexBufferData,
      m_vertexBuffer.put()
    )
  );

  // Load mesh indices. Each trio of indices represents
  // a triangle to be rendered on the screen.
  // For example: 2,1,0 means that the vertices with indexes
  // 2, 1, and 0 from the vertex buffer compose the
  // first triangle of this mesh.
  // Note that the winding order is clockwise by default.
  constexpr std::array<unsigned short, 6> quadIndices =
  { {
          // -z
          0,2,3,
          0,1,2,
      } };

  m_indexCount = quadIndices.size();

  D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
  indexBufferData.pSysMem = quadIndices.data();
  indexBufferData.SysMemPitch = 0;
  indexBufferData.SysMemSlicePitch = 0;
  const CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short) * quadIndices.size(), D3D11_BIND_INDEX_BUFFER);
  check_hresult(
    m_deviceResources->GetD3DDevice()->CreateBuffer(
      &indexBufferDesc,
      &indexBufferData,
      m_indexBuffer.put()
    )
  );



  m_loadingComplete = true;

};

void ndtech::DistanceFieldRenderer::RenderDistanceField(ID3D11ShaderResourceView* texture)
{
  using namespace winrt;

  // Loading is asynchronous. Resources must be created before drawing can occur.
  if (!m_loadingComplete || texture == nullptr)
  {
    return;
  }

  const auto context = m_deviceResources->GetD3DDeviceContext();

  // Set and clear the off-screen render target.    
  ID3D11RenderTargetView *const targets[1] = { m_renderTargetView.get() };
  context->OMSetRenderTargets(1, targets, m_d3dDepthStencilView.get());
  context->ClearRenderTargetView(targets[0], DirectX::Colors::Transparent);
  context->ClearDepthStencilView(m_d3dDepthStencilView.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

  // Set the viewport to cover the whole texture.
  CD3D11_VIEWPORT viewport = CD3D11_VIEWPORT(
    0.f, 0.f,
    static_cast<float>(m_textureWidth),
    static_cast<float>(m_textureHeight));
  context->RSSetViewports(1, &viewport);

  // Each vertex is one instance of the XMFLOAT2 struct.
  const UINT stride = m_vertexStride;
  const UINT offset = 0;
  ID3D11Buffer * buffer = m_vertexBuffer.get();
  context->IASetVertexBuffers(
    0,
    1,
    &buffer,
    &stride,
    &offset
  );

  context->IASetIndexBuffer(
    m_indexBuffer.get(),
    DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
    0
  );
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->IASetInputLayout(m_inputLayout.get());

  // Attach the vertex shader.
  context->VSSetShader(
    m_vertexShader.get(),
    nullptr,
    0
  );

  // Attach the pixel shader.
  context->PSSetShader(
    m_pixelShader.get(),
    nullptr,
    0
  );
  context->PSSetShaderResources(
    0,
    1,
    &texture
  );

  // Draw the objects.
  context->DrawIndexed(
    m_indexCount,   // Index count.
    0,              // Start index location.
    0               // Base vertex location.
  );

  ++m_renderCount;
};

void ndtech::DistanceFieldRenderer::ReleaseDeviceDependentResources()
{
  m_loadingComplete = false;

  m_vertexShader = nullptr;
  m_inputLayout = nullptr;
  m_pixelShader = nullptr;

  m_vertexBuffer = nullptr;
  m_indexBuffer = nullptr;

  m_texture = nullptr;
  m_shaderResourceView = nullptr;
  m_renderTargetView = nullptr;
};