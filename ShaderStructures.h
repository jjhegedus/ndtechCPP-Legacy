#pragma once
#include "pch.h"


namespace ndtech
{
#if NDTECH_HOLO
    // Constant buffer used to send hologram position transform to the shader pipeline.
    struct ModelConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
        //DirectX::XMFLOAT4   hologramColorFadeMultiplier;
        float instanceId;
        float pad1;
        float pad2;
        float pad3;
    };

    // Assert that the constant buffer remains 16-byte aligned (best practice).
    static_assert((sizeof(ModelConstantBuffer) % (sizeof(float) * 4)) == 0, "Model constant buffer size must be 16-byte aligned (16 bytes is the length of four floats).");


    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionColor
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 color;
    };
#endif

#if NDTECH_ML
    struct VertexPositionColor {
      glm::vec3 pos;
      glm::vec3 color;
    };
#endif
    //// Used to send per-vertex data to the vertex shader.
    //struct VertexPositionColorTex
    //{
    //    DirectX::XMFLOAT3 pos;
    //    DirectX::XMFLOAT3 color;
    //    DirectX::XMFLOAT2 texCoord;
    //};
}