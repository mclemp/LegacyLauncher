#include "PanoramaRenderer.h"
#include <vector>
#include <iostream>

#include "../../resource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <d3dcompiler.h>
#include <unordered_set>
#include <unordered_map>
#include "../ResourcePuller/ResourcePuller.h"

using namespace DirectX;

struct Vertex {
    XMFLOAT3 pos;
};

const char* g_vs = R"(
cbuffer CameraBuffer : register(b0)
{
    float4x4 rotation;
};

struct VS_IN {
    float3 pos : POSITION;
};

struct VS_OUT {
    float4 pos : SV_POSITION;
    float3 dir : TEXCOORD0;
};

VS_OUT main(VS_IN input) {
    VS_OUT o;

    float3 rotated = mul((float3x3)rotation, input.pos);

    o.pos = float4(input.pos, 1.0);
    o.dir = rotated;

    return o;
}
)";

const char* g_ps = R"(
TextureCube skyboxTex : register(t0);
SamplerState samp : register(s0);

struct PS_IN {
    float4 pos : SV_POSITION;
    float3 dir : TEXCOORD0;
};

float4 main(PS_IN input) : SV_Target {
    return skyboxTex.Sample(samp, normalize(input.dir));
}
)";

void PanoramaRenderer::Initialize(HMODULE handle, ID3D11Device* device, int w, int h) {
    width = w;
    height = h;

    D3D11_RASTERIZER_DESC rs = {};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE; // 🔥 IMPORTANT
    rs.DepthClipEnable = TRUE;

    device->CreateRasterizerState(&rs, &rasterState);

    CreateRenderTexture(device);

    if (!LoadCubemap(handle, device)) {
        OutputDebugStringA("CUBEMAP FAILED\n");
    }
    else {
        OutputDebugStringA("CUBEMAP OK\n");
    }

    // Sampler
    D3D11_SAMPLER_DESC samp = {};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&samp, &sampler);

    // 🔥 FULL CUBE (36 verts)
    Vertex verts[] = {
        {{-1,-1,-1}}, {{-1, 1,-1}}, {{ 1, 1,-1}},
        {{-1,-1,-1}}, {{ 1, 1,-1}}, {{ 1,-1,-1}},

        {{-1,-1, 1}}, {{ 1,-1, 1}}, {{ 1, 1, 1}},
        {{-1,-1, 1}}, {{ 1, 1, 1}}, {{-1, 1, 1}},

        {{-1,-1,-1}}, {{-1,-1, 1}}, {{-1, 1, 1}},
        {{-1,-1,-1}}, {{-1, 1, 1}}, {{-1, 1,-1}},

        {{ 1,-1,-1}}, {{ 1, 1,-1}}, {{ 1, 1, 1}},
        {{ 1,-1,-1}}, {{ 1, 1, 1}}, {{ 1,-1, 1}},

        {{-1, 1,-1}}, {{-1, 1, 1}}, {{ 1, 1, 1}},
        {{-1, 1,-1}}, {{ 1, 1, 1}}, {{ 1, 1,-1}},

        {{-1,-1,-1}}, {{ 1,-1,-1}}, {{ 1,-1, 1}},
        {{-1,-1,-1}}, {{ 1,-1, 1}}, {{-1,-1, 1}},
    };

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = sizeof(XMMATRIX);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    device->CreateBuffer(&cbd, nullptr, &constantBuffer);

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(verts);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = verts;

    device->CreateBuffer(&bd, &init, &vertexBuffer);

    // 🔥 COMPILE SHADERS FROM MEMORY
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    D3DCompile(g_vs, strlen(g_vs), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    D3DCompile(g_ps, strlen(g_ps), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

    // Input Layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    device->CreateInputLayout(
        layout, 1,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &inputLayout
    );

    vsBlob->Release();
    psBlob->Release();
}

void PanoramaRenderer::Render(ID3D11DeviceContext* ctx, float time) {
    ID3D11RenderTargetView* oldRTV = nullptr;
    ctx->OMGetRenderTargets(1, &oldRTV, nullptr);

    ctx->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    ctx->RSSetViewports(1, &vp);

    ctx->RSSetState(rasterState);

    float speed = -0.075f;

    // rotate over time
    XMMATRIX rotY = XMMatrixRotationY(time * speed);
    //XMMATRIX rotX = XMMatrixRotationX(sinf(time * 0.5f) * 0.2f);

    // combine
    //XMMATRIX rotation = rotX * rotY;
    XMMATRIX rotation = rotY;

    // transpose for HLSL
    rotation = XMMatrixTranspose(rotation);

    // upload
    D3D11_MAPPED_SUBRESOURCE mapped;
    ctx->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &rotation, sizeof(rotation));
    ctx->Unmap(constantBuffer, 0);

    // bind
    ctx->VSSetConstantBuffers(0, 1, &constantBuffer);

    float clear[4] = { 0,0,0,1 };
    ctx->ClearRenderTargetView(rtv, clear);

    ctx->IASetInputLayout(inputLayout);
    ctx->VSSetShader(vertexShader, nullptr, 0);
    ctx->PSSetShader(pixelShader, nullptr, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    ctx->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (!cubeSRV) {
        OutputDebugStringA("CUBEMAP IS NULL\n");
    }

    ctx->PSSetShaderResources(0, 1, &cubeSRV);
    ctx->PSSetSamplers(0, 1, &sampler);

    ctx->Draw(36, 0);

    ctx->OMSetRenderTargets(1, &oldRTV, nullptr);
    if (oldRTV) oldRTV->Release();
}

void PanoramaRenderer::CreateRenderTexture(ID3D11Device* device) {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    device->CreateTexture2D(&desc, nullptr, &renderTexture);
    device->CreateRenderTargetView(renderTexture, nullptr, &rtv);
    device->CreateShaderResourceView(renderTexture, nullptr, &srv);
}

ID3D11ShaderResourceView* PanoramaRenderer::GetSRV() {
    return srv;
}

static const int panoramaMap[6] = { 1, 3, 4, 5, 0, 2 };
std::unordered_map<int, std::vector<int>> panoramaImages = {
    {1, { IDB_PNG1,  IDB_PNG2,  IDB_PNG3,  IDB_PNG4,  IDB_PNG5,  IDB_PNG6 }},
    {2, { IDB_PNG7,  IDB_PNG8,  IDB_PNG9,  IDB_PNG10, IDB_PNG11, IDB_PNG12 }},
    {3, { IDB_PNG13, IDB_PNG14, IDB_PNG15, IDB_PNG16, IDB_PNG17, IDB_PNG18 }},
    {4, { IDB_PNG19, IDB_PNG20, IDB_PNG21, IDB_PNG22, IDB_PNG23, IDB_PNG24 }},
};

std::vector<ResourcePuller::ResourceData> GetPanoramaImages(HMODULE handle, int index) {
    std::vector<ResourcePuller::ResourceData> imageData;

    if (index > panoramaImages.size()) {
        index = 1;
    }

    for (int resourceIndex : panoramaImages[index]) {
        imageData.push_back(ResourcePuller::LoadResourceData((uintptr_t)handle, resourceIndex));
    }

    
    return imageData;
}


bool PanoramaRenderer::LoadCubemap(HMODULE handle, ID3D11Device* device) {
    std::vector<ResourcePuller::ResourceData> panoramaImages = GetPanoramaImages(handle, 4);

    int w, h, c;
    unsigned char* images[6];

    stbi_set_flip_vertically_on_load(false);

    for (int i = 0; i < 6; i++) {
        ResourcePuller::ResourceData panoramaData = panoramaImages[panoramaMap[i]];

        images[i] = stbi_load_from_memory(panoramaData.data, panoramaData.size, &w, &h, &c, 4);
        if (!images[i]) {
            OutputDebugStringA(std::string("Failed: " + std::to_string(i) + "\n").c_str());
            return false;
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 6;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_SUBRESOURCE_DATA data[6];
    for (int i = 0; i < 6; i++) {
        data[i].pSysMem = images[i];
        data[i].SysMemPitch = w * 4;
    }

    ID3D11Texture2D* tex = nullptr;
    device->CreateTexture2D(&desc, data, &tex);

    for (int i = 0; i < 6; i++) stbi_image_free(images[i]);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = 1;

    device->CreateShaderResourceView(tex, &srvDesc, &cubeSRV);
    tex->Release();

    return true;
}

ID3D11ShaderResourceView* CubemapLoader::LoadCubemap(
    ID3D11Device* device,
    const std::string& right,
    const std::string& left,
    const std::string& top,
    const std::string& bottom,
    const std::string& front,
    const std::string& back
) {
    std::string paths[6] = { right, left, top, bottom, front, back };

    int width = 0, height = 0, channels = 0;
    std::vector<unsigned char*> images(6);

    stbi_set_flip_vertically_on_load(false); // important for cubemaps

    // Load all 6 images
    for (int i = 0; i < 6; i++) {
        images[i] = stbi_load(paths[i].c_str(), &width, &height, &channels, 4);
        if (!images[i]) {
            OutputDebugStringA(std::string("Failed to load: " + paths[i]).c_str());
            return nullptr;
        }
    }

    // Describe cubemap texture
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 6;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    // Fill subresources
    D3D11_SUBRESOURCE_DATA subresources[6];
    for (int i = 0; i < 6; i++) {
        subresources[i].pSysMem = images[i];
        subresources[i].SysMemPitch = width * 4;
        subresources[i].SysMemSlicePitch = 0;
    }

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, subresources, &texture);

    // Free image data
    for (int i = 0; i < 6; i++) {
        stbi_image_free(images[i]);
    }

    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create cubemap texture\n");
        return nullptr;
    }

    // Create SRV (cubemap view)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(texture, &srvDesc, &srv);

    texture->Release();

    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create SRV\n");
        return nullptr;
    }

    return srv;
}