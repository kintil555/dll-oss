#pragma once

#include <d3d11.h>
#include <mutex>
#include <iostream>
#include <winrt/base.h>
#include "Client.hpp"
#include "../../../../Hook/Hooks/Render/DirectX/DXGI/SwapchainHook.hpp"

// Backbuffer -> SRV helpers (previously part of MotionBlur, used by Blur).
namespace BackbufferSRV {
    inline winrt::com_ptr<ID3D11ShaderResourceView> FromTexture(const winrt::com_ptr<ID3D11Texture2D>& tex) {
        if (!tex || !SwapchainHook::d3d11Device) return nullptr;
        D3D11_TEXTURE2D_DESC d;
        tex->GetDesc(&d);
        winrt::com_ptr<ID3D11ShaderResourceView> outSRV;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = d.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = d.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        HRESULT hr = SwapchainHook::d3d11Device->CreateShaderResourceView(tex.get(), &srvDesc, outSRV.put());
        if (FAILED(hr)) std::cout << "Failed to create shader resource view: " << std::hex << hr << std::endl;
        return outSRV;
    }

    inline winrt::com_ptr<ID3D11ShaderResourceView> Saved() {
        winrt::com_ptr<ID3D11Texture2D> buf;
        {
            std::lock_guard<std::mutex> lock(SwapchainHook::backbufferMutex);
            buf = SwapchainHook::SavedD3D11BackBuffer;
        }
        return FromTexture(buf);
    }

    inline winrt::com_ptr<ID3D11ShaderResourceView> Extra(bool underui = false) {
        if (!FlarialGUI::needsBackBuffer) return nullptr;
        if (SwapchainHook::isDX12) return Saved();
        if (auto srv = SwapchainHook::GetCurrentBackbufferSRV(underui)) return srv;
        winrt::com_ptr<ID3D11Texture2D> buf;
        {
            std::lock_guard<std::mutex> lock(SwapchainHook::backbufferMutex);
            buf = SwapchainHook::ExtraSavedD3D11BackBuffer;
        }
        return FromTexture(buf);
    }
}
