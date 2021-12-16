#pragma once

#include "pch.h"
#include "DeviceResources.h"

#include "DirectXHelper.h"
#include "ShaderStructures.h"



namespace ndtech
{
    class DistanceFieldRenderer
    {
    private:
        // Cached pointer to device resources.
        DeviceResources*          m_deviceResources;

        // Direct3D resources for filtering a texture to an off-screen render target.
        winrt::com_ptr<ID3D11InputLayout>           m_inputLayout = nullptr;
        winrt::com_ptr<ID3D11Buffer>                m_vertexBuffer = nullptr;
        winrt::com_ptr<ID3D11Buffer>                m_indexBuffer = nullptr;
        winrt::com_ptr<ID3D11VertexShader>          m_vertexShader = nullptr;
        winrt::com_ptr<ID3D11PixelShader>           m_pixelShader = nullptr;
        winrt::com_ptr<ID3D11Texture2D>             m_texture = nullptr;
        winrt::com_ptr<ID3D11ShaderResourceView>    m_shaderResourceView = nullptr;
        winrt::com_ptr<ID3D11DepthStencilView>      m_d3dDepthStencilView = nullptr;
        winrt::com_ptr<ID3D11RenderTargetView>      m_renderTargetView = nullptr;

        // CPU-based variables for configuring the offscreen render target.
        const UINT                                          m_textureWidth;
        const UINT                                          m_textureHeight;

        // System resources for quad geometry.
        uint32_t                                              m_indexCount = 0;
        uint32_t                                              m_vertexStride = 0;

        // Variables used with the rendering loop.
        bool                                                m_loadingComplete = false;
        unsigned int                                        m_renderCount = 0;

    public:
        DistanceFieldRenderer(DeviceResources* deviceResources, unsigned int const& textureWidth, unsigned int const& textureHeight);

        winrt::Windows::Foundation::IAsyncAction CreateDeviceDependentResources();

        winrt::Windows::Foundation::IAsyncAction CreateDeviceDependentResourcesPart2();



        void RenderDistanceField(ID3D11ShaderResourceView* texture);

        void ReleaseDeviceDependentResources();

        inline ID3D11ShaderResourceView*   GetTextureView()    const { return m_shaderResourceView.get(); };
        inline ID3D11Texture2D*            GetTexture() const { return m_texture.get(); };
        inline ID3D11RenderTargetView*     GetRenderTargetView() const {return m_renderTargetView.get(); };
        inline unsigned int const&         GetRenderCount()    const { return m_renderCount; };
    };
}