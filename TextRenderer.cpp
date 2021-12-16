#include "pch.h"

#include "DirectXHelper.h"
#include "TextRenderer.h"

#ifdef _DEBUG
#include "Utilities.h"
#include "Conversion.h"
#endif

namespace ndtech
{
    using namespace winrt;
    using namespace ndtech::utilities;

    TextRenderer::TextRenderer(DeviceResources* deviceResources, unsigned int const& textureWidth, unsigned int const& textureHeight)
        : m_deviceResources(deviceResources),
        m_textureWidth(textureWidth),
        m_textureHeight(textureHeight)
    {
        CreateDeviceDependentResources();
    }

    void TextRenderer::RenderTextOffscreen(const std::wstring& str)
    {
        // Clear the off-screen render target.
        m_deviceResources->GetD3DDeviceContext()->ClearRenderTargetView(m_renderTargetView.get(), DirectX::Colors::Transparent);

        // Begin drawing with D2D.
        m_d2dRenderTarget->BeginDraw();

        // Create a text layout to match the screen.
        com_ptr<IDWriteTextLayout> textLayout;
        m_deviceResources->GetDWriteFactory()->CreateTextLayout(
            str.c_str(),
            static_cast<UINT32>(str.length()),
            m_textFormat.get(),
            (float)m_textureWidth,
            (float)m_textureHeight,
            textLayout.put()
        );

        // Get the text metrics from the text layout.
        DWRITE_TEXT_METRICS metrics;
        check_hresult(textLayout->GetMetrics(&metrics));

        // In this example, we position the text in the center of the off-screen render target.
        D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(
            m_textureWidth * 0.5f,
            m_textureHeight * 0.5f + metrics.height * 0.5f
        );
        m_whiteBrush->SetTransform(screenTranslation);

        // Render the text using DirectWrite.
        m_d2dRenderTarget->DrawTextLayout(
            D2D1::Point2F(0.0f, 0.0f),
            textLayout.get(),
            m_whiteBrush.get()
        );

        // End drawing with D2D.
        HRESULT hr = m_d2dRenderTarget->EndDraw();
        if (hr != D2DERR_RECREATE_TARGET)
        {
            //Catch errors from D2D.

            try
            {
                check_hresult(hr);
            }
            catch (...)
            {
#ifdef _DEBUG
                std::wstring wstr;
                std::string hrstr(GetHRAsString(hr));
                wstr = string_to_T<std::wstring>(hrstr);

                //winrt::hstring message = (wstr);
                //OutputDebugStringW(message.data());
#endif
                throw;
            }
        }
    }

    void TextRenderer::ReleaseDeviceDependentResources()
    {
        m_texture = nullptr;
        m_shaderResourceView = nullptr;
        m_pointSampler = nullptr;
        m_renderTargetView = nullptr;
        m_d2dRenderTarget = nullptr;
        m_whiteBrush = nullptr;
        m_textFormat = nullptr;
    }

    void TextRenderer::CreateDeviceDependentResources()
    {
        std::wstringstream s;
        s << "TextRenderer::CreateDeviceDependentResources():BEGIN" << std::endl;
        OutputDebugStringW(s.str().c_str());

        // Create a default sampler state, which will use point sampling.
        CD3D11_SAMPLER_DESC desc(D3D11_DEFAULT);
        m_deviceResources->GetD3DDevice()->CreateSamplerState(&desc, m_pointSampler.put());

        // Create the texture that will be used as the offscreen render target.
        CD3D11_TEXTURE2D_DESC textureDesc(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            m_textureWidth,
            m_textureHeight,
            1,
            1,
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
        );
        m_deviceResources->GetD3DDevice()->CreateTexture2D(&textureDesc, nullptr, m_texture.put());

        // Create read and write views for the offscreen render target.
        m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_texture.get(), nullptr, m_shaderResourceView.put());
        m_deviceResources->GetD3DDevice()->CreateRenderTargetView(m_texture.get(), nullptr, m_renderTargetView.put());

        // In this example, we are using D2D and DirectWrite; so, we need to create a D2D render target as well.
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96,
            96
        );

        // The DXGI surface is used to create the render target.
        com_ptr<IDXGISurface> dxgiSurface;
        dxgiSurface = m_texture.as<IDXGISurface>();
        check_hresult(m_deviceResources->GetD2DFactory()->CreateDxgiSurfaceRenderTarget(dxgiSurface.get(), &props, m_d2dRenderTarget.put()));

        // Create a solid color brush that will be used to render the text.
        check_hresult(m_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Cornsilk), m_whiteBrush.put()));

        // This is where we format the text that will be written on the render target.
        check_hresult(
            m_deviceResources->GetDWriteFactory()->CreateTextFormat(
                L"Consolas",
                NULL,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                400.0f,
                L"",
                m_textFormat.put()
            )
        );
        check_hresult(m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
        check_hresult(m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

        std::wstringstream s2;
        s2 << "TextRenderer::CreateDeviceDependentResources():END" << std::endl;
        OutputDebugStringW(s2.str().c_str());
    }

}