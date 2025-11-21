#pragma once
#include <d3d9.h>
#include <cstdint>

namespace VTFLoader
{
	// VTF Format enums
	enum ImageFormat
	{
		IMAGE_FORMAT_NONE = -1,
		IMAGE_FORMAT_RGBA8888 = 0,
		IMAGE_FORMAT_ABGR8888,
		IMAGE_FORMAT_RGB888,
		IMAGE_FORMAT_BGR888,
		IMAGE_FORMAT_RGB565,
		IMAGE_FORMAT_I8,
		IMAGE_FORMAT_IA88,
		IMAGE_FORMAT_P8,
		IMAGE_FORMAT_A8,
		IMAGE_FORMAT_RGB888_BLUESCREEN,
		IMAGE_FORMAT_BGR888_BLUESCREEN,
		IMAGE_FORMAT_ARGB8888,
		IMAGE_FORMAT_BGRA8888,
		IMAGE_FORMAT_DXT1,
		IMAGE_FORMAT_DXT3,
		IMAGE_FORMAT_DXT5,
		IMAGE_FORMAT_BGRX8888,
		IMAGE_FORMAT_BGR565,
		IMAGE_FORMAT_BGRX5551,
		IMAGE_FORMAT_BGRA4444,
		IMAGE_FORMAT_DXT1_ONEBITALPHA,
		IMAGE_FORMAT_BGRA5551,
		IMAGE_FORMAT_UV88,
		IMAGE_FORMAT_UVWQ8888,
		IMAGE_FORMAT_RGBA16161616F,
		IMAGE_FORMAT_RGBA16161616,
		IMAGE_FORMAT_UVLX8888
	};

	// VTF file header structure (version 7.2+)
	struct VTFHeader
	{
		char signature[4];           // "VTF\0"
		uint32_t version[2];         // version[0].version[1]
		uint32_t headerSize;
		uint16_t width;
		uint16_t height;
		uint32_t flags;
		uint16_t frames;
		uint16_t firstFrame;
		uint8_t padding0[4];
		float reflectivity[3];
		uint8_t padding1[4];
		float bumpmapScale;
		uint32_t highResImageFormat;
		uint8_t mipmapCount;
		uint32_t lowResImageFormat;
		uint8_t lowResImageWidth;
		uint8_t lowResImageHeight;
		uint16_t depth;               // v7.2+
	};

	// Convert BGRA to RGBA
	void ConvertBGRAtoRGBA(uint8_t* pImageBuffer, int nSize);

	// Create D3D9 texture from raw RGBA data
	bool CreateTexture(uint8_t* pImageBuffer, int iWidth, int iHeight, IDirect3DTexture9** ppTexture, IDirect3DDevice9* pDevice);

	// Load VTF from VPK and return RGBA pixel data
	uint8_t* ReadVTFFromVPK(const char* szVPKPath, int* iWidth, int* iHeight);

	// Free image data
	void FreeImage(uint8_t* pImageBuffer);

	// DXT decompression helpers
	void DecompressDXT1(const uint8_t* src, uint8_t* dst, int width, int height);
	void DecompressDXT5(const uint8_t* src, uint8_t* dst, int width, int height);
}
