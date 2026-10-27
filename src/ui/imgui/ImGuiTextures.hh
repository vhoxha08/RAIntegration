#ifndef RA_UI_IMGUI_TEXTURES_HH
#define RA_UI_IMGUI_TEXTURES_HH
#pragma once

#include <cstdint>

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// Texture handling provided by the host, so the toolkit doesn't depend on a graphics API or image decoders.
/// All functions are only called on the UI thread, with the host's rendering context current.
/// A texture handle is the ImTextureID the host's ImGui renderer understands; 0 means failure.
/// </summary>
struct TextureHandlers
{
    uintptr_t (*fpCreateFromFile)(const char* sUtf8Path, int* pWidth, int* pHeight) = nullptr;
    uintptr_t (*fpCreateFromPixels)(const uint32_t* pARGB, int nWidth, int nHeight) = nullptr;
    void (*fpDestroy)(uintptr_t nTexture) = nullptr;
};

TextureHandlers& GetTextureHandlers() noexcept;

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_TEXTURES_HH
