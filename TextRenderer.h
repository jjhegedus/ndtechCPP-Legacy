#pragma once
#include "pch.h"
#include "DeviceResources.h"
#include <sstream>

namespace ndtech
{
    class TextRenderer
    {
    public:
        TextRenderer() = delete;
        ~TextRenderer() = default;
        TextRenderer(const TextRenderer&) = delete;
        TextRenderer& operator=(const TextRenderer&) & = delete;
        TextRenderer(TextRenderer&&) = delete;
        TextRenderer& operator=(TextRenderer&&) & = delete;

        TextRenderer(DeviceResources* deviceResources, unsigned int const& textureWidth, unsigned int const& textureHeight);

        void RenderTextOffscreen(const std::wstring& str);

        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();

        ID3D11ShaderResourceView* GetTextureView() const { return m_shaderResourceView.get(); };
        ID3D11Texture2D*          GetTexture() const {return m_texture.get(); };
        ID3D11SamplerState*       GetSampler() const { return m_pointSampler.get(); };

    private:
        // Cached pointer to device resources.
        DeviceResources* m_deviceResources;

        // Direct3D resources for rendering text to an off-screen render target.
        winrt::com_ptr<ID3D11Texture2D>             m_texture;
        winrt::com_ptr<ID3D11ShaderResourceView>    m_shaderResourceView;
        winrt::com_ptr<ID3D11SamplerState>          m_pointSampler;
        winrt::com_ptr<ID3D11RenderTargetView>      m_renderTargetView;
        winrt::com_ptr<ID2D1RenderTarget>           m_d2dRenderTarget;
        winrt::com_ptr<ID2D1SolidColorBrush>        m_whiteBrush;
        winrt::com_ptr<IDWriteTextFormat>           m_textFormat = nullptr;

        // CPU-based variables for configuring the offscreen render target.
        const unsigned int m_textureWidth;
        const unsigned int m_textureHeight;
    };
}