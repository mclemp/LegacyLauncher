#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

using namespace DirectX;

#define USING_PANORAMA 1

class CubemapLoader {
public:
    static ID3D11ShaderResourceView* LoadCubemap(
        ID3D11Device* device,
        const std::string& right,
        const std::string& left,
        const std::string& top,
        const std::string& bottom,
        const std::string& front,
        const std::string& back
    );
};

class PanoramaRenderer {
public:
    void Initialize(HMODULE handle, ID3D11Device* device, int width, int height);
    void Render(ID3D11DeviceContext* ctx, float time);
    ID3D11ShaderResourceView* GetSRV();

private:
    void CreateRenderTexture(ID3D11Device* device);
    bool LoadCubemap(HMODULE handle, ID3D11Device* device);

    int width, height;

    // Render target
    ID3D11Texture2D* renderTexture = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;

    // Cubemap
    ID3D11ShaderResourceView* cubeSRV = nullptr;
    ID3D11SamplerState* sampler = nullptr;

    // Pipeline
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* layout = nullptr;

    //Shader
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;

    ID3D11RasterizerState* rasterState = nullptr;

    ID3D11Buffer* constantBuffer = nullptr;
};