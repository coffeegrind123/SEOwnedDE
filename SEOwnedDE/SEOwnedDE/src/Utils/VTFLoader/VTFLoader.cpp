#include "VTFLoader.h"
#include "../../SDK/SDK.h"
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

// Set to 1 to enable VTF horizontal mirroring, 0 to disable
#define VTF_ENABLE_MIRRORING 1

// DXT1 block decompression
static void DecompressDXT1Block(const uint8_t* block, uint8_t* output, int stride, int blockX, int blockY, int width, int height)
{
	uint16_t color0 = block[0] | (block[1] << 8);
	uint16_t color1 = block[2] | (block[3] << 8);
	uint32_t bits = block[4] | (block[5] << 8) | (block[6] << 16) | (block[7] << 24);

	// Use reference implementation's RGB565->RGB888 conversion for accuracy
	uint32_t temp;

	temp = (color0 >> 11) * 255 + 16;
	uint8_t r0 = (uint8_t)((temp/32 + temp)/32);
	temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
	uint8_t g0 = (uint8_t)((temp/64 + temp)/64);
	temp = (color0 & 0x001F) * 255 + 16;
	uint8_t b0 = (uint8_t)((temp/32 + temp)/32);

	temp = (color1 >> 11) * 255 + 16;
	uint8_t r1 = (uint8_t)((temp/32 + temp)/32);
	temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
	uint8_t g1 = (uint8_t)((temp/64 + temp)/64);
	temp = (color1 & 0x001F) * 255 + 16;
	uint8_t b1 = (uint8_t)((temp/32 + temp)/32);

	uint8_t colors[4][3];
	colors[0][0] = r0; colors[0][1] = g0; colors[0][2] = b0;
	colors[1][0] = r1; colors[1][1] = g1; colors[1][2] = b1;

	if (color0 > color1)
	{
		colors[2][0] = (2 * r0 + r1) / 3;
		colors[2][1] = (2 * g0 + g1) / 3;
		colors[2][2] = (2 * b0 + b1) / 3;

		colors[3][0] = (r0 + 2 * r1) / 3;
		colors[3][1] = (g0 + 2 * g1) / 3;
		colors[3][2] = (b0 + 2 * b1) / 3;
	}
	else
	{
		colors[2][0] = (r0 + r1) / 2;
		colors[2][1] = (g0 + g1) / 2;
		colors[2][2] = (b0 + b1) / 2;

		colors[3][0] = 0;
		colors[3][1] = 0;
		colors[3][2] = 0;
	}

	// DXT1 stores indices in rows (4 pixels/row, 4 rows)
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			// Bounds check - don't write pixels outside texture dimensions
			int pixelX = blockX * 4 + x;
			int pixelY = blockY * 4 + y;

			// DXT1 has 1-pixel vertical offset on right half - shift it up
			// Testing: try 4-pixel shift for DXT
			int yAdjust = (pixelX >= width / 2) ? -4 : 0;

			if (pixelY + yAdjust < 0 || pixelY + yAdjust >= height || pixelX >= width)
				continue;

			// Extract 2-bit index for this pixel
			int bitOffset = (y * 4 + x) * 2;
			int index = (bits >> bitOffset) & 3;

			// Write to output buffer at correct position (with Y adjustment)
			uint8_t* pixel = output + ((y + yAdjust) * stride + x * 4);
			pixel[0] = colors[index][0];  // R
			pixel[1] = colors[index][1];  // G
			pixel[2] = colors[index][2];  // B
			pixel[3] = (color0 > color1 || index < 3) ? 255 : 0;  // Alpha: 0 only for index 3 in 1-bit mode
		}
	}
}

// DXT5 block decompression (combined alpha + color, matching reference implementation)
static void DecompressDXT5Block(const uint8_t* block, uint8_t* output, int stride, int blockX, int blockY, int width, int height)
{
	// First 8 bytes: alpha block
	uint8_t alpha0 = block[0];
	uint8_t alpha1 = block[1];

	const uint8_t* alphaBits = block + 2;
	uint32_t alphaCode1 = alphaBits[2] | (alphaBits[3] << 8) | (alphaBits[4] << 16) | (alphaBits[5] << 24);
	uint16_t alphaCode2 = alphaBits[0] | (alphaBits[1] << 8);

	// Next 8 bytes: color block
	uint16_t color0 = block[8] | (block[9] << 8);
	uint16_t color1 = block[10] | (block[11] << 8);
	uint32_t colorBits = block[12] | (block[13] << 8) | (block[14] << 16) | (block[15] << 24);

	// RGB565 -> RGB888 conversion (reference implementation method)
	uint32_t temp;

	temp = (color0 >> 11) * 255 + 16;
	uint8_t r0 = (uint8_t)((temp/32 + temp)/32);
	temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
	uint8_t g0 = (uint8_t)((temp/64 + temp)/64);
	temp = (color0 & 0x001F) * 255 + 16;
	uint8_t b0 = (uint8_t)((temp/32 + temp)/32);

	temp = (color1 >> 11) * 255 + 16;
	uint8_t r1 = (uint8_t)((temp/32 + temp)/32);
	temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
	uint8_t g1 = (uint8_t)((temp/64 + temp)/64);
	temp = (color1 & 0x001F) * 255 + 16;
	uint8_t b1 = (uint8_t)((temp/32 + temp)/32);

	// Process all 16 pixels in the block
	// VTF appears to store pixels within blocks with columns swapped
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
		{
			int pixelIndex = 4 * j + i;

			// Decode alpha (3-bit index)
			int alphaCodeIndex = 3 * pixelIndex;
			int alphaCode;

			if (alphaCodeIndex <= 12)
			{
				alphaCode = (alphaCode2 >> alphaCodeIndex) & 0x07;
			}
			else if (alphaCodeIndex == 15)
			{
				alphaCode = (alphaCode2 >> 15) | ((alphaCode1 << 1) & 0x06);
			}
			else // alphaCodeIndex >= 18 && alphaCodeIndex <= 45
			{
				alphaCode = (alphaCode1 >> (alphaCodeIndex - 16)) & 0x07;
			}

			uint8_t finalAlpha;
			if (alphaCode == 0)
				finalAlpha = alpha0;
			else if (alphaCode == 1)
				finalAlpha = alpha1;
			else
			{
				if (alpha0 > alpha1)
					finalAlpha = ((8 - alphaCode) * alpha0 + (alphaCode - 1) * alpha1) / 7;
				else
				{
					if (alphaCode == 6)
						finalAlpha = 0;
					else if (alphaCode == 7)
						finalAlpha = 255;
					else
						finalAlpha = ((6 - alphaCode) * alpha0 + (alphaCode - 1) * alpha1) / 5;
				}
			}

			// Decode color (2-bit index) - DXT5 always uses 4-color mode
			int colorCode = (colorBits >> (2 * pixelIndex)) & 0x03;

			uint8_t r, g, b;
			switch (colorCode)
			{
				case 0:
					r = r0; g = g0; b = b0;
					break;
				case 1:
					r = r1; g = g1; b = b1;
					break;
				case 2:
					r = (2*r0+r1)/3; g = (2*g0+g1)/3; b = (2*b0+b1)/3;
					break;
				case 3:
					r = (r0+2*r1)/3; g = (g0+2*g1)/3; b = (b0+2*b1)/3;
					break;
			}

			// Bounds check - don't write pixels outside texture dimensions
			int pixelX = blockX * 4 + i;
			int pixelY = blockY * 4 + j;

			// DXT5 has 1-pixel vertical offset on right half - shift it up
			// Testing: try 4-pixel shift for DXT
			int yAdjust = (pixelX >= width / 2) ? -4 : 0;

			if (pixelY + yAdjust < 0 || pixelY + yAdjust >= height || pixelX >= width)
				continue;

			// Write final pixel (with Y adjustment)
			uint8_t* pixel = output + ((j + yAdjust) * stride + i * 4);
			pixel[0] = r;
			pixel[1] = g;
			pixel[2] = b;
			pixel[3] = finalAlpha;
		}
	}
}

void VTFLoader::DecompressDXT1(const uint8_t* src, uint8_t* dst, int width, int height)
{
	int blockCountX = (width + 3) / 4;
	int blockCountY = (height + 3) / 4;
	int halfBlockX = blockCountX / 2;

	// VTF stores blocks left-to-right, but we need to swap the X coordinate
	// Block at source position bx should be drawn at mirrored X position
	const uint8_t* blockStorage = src;
	for (int by = 0; by < blockCountY; by++)
	{
		for (int bx = 0; bx < blockCountX; bx++)
		{
#if VTF_ENABLE_MIRRORING
			// Mirror the X coordinate across the center
			int mirroredBx = (bx < halfBlockX) ? (bx + halfBlockX) : (bx - halfBlockX);
#else
			int mirroredBx = bx;
#endif
			uint8_t* output = dst + ((by * 4) * width + (mirroredBx * 4)) * 4;
			DecompressDXT1Block(blockStorage, output, width * 4, mirroredBx, by, width, height);
			blockStorage += 8;
		}
	}
}

void VTFLoader::DecompressDXT5(const uint8_t* src, uint8_t* dst, int width, int height)
{
	int blockCountX = (width + 3) / 4;
	int blockCountY = (height + 3) / 4;
	int halfBlockX = blockCountX / 2;

	// Debug: DXT5 decompression info
	// I::CVar->ConsoleColorPrintf({200, 200, 100, 255}, "[VTFLoader] DXT5: %dx%d pixels, %dx%d blocks, halfBlockX=%d\n",
	//	width, height, blockCountX, blockCountY, halfBlockX);

	// VTF stores blocks left-to-right, but we need to swap the X coordinate
	// Block at source position bx should be drawn at mirrored X position
	const uint8_t* blockStorage = src;
	for (int by = 0; by < blockCountY; by++)
	{
		for (int bx = 0; bx < blockCountX; bx++)
		{
#if VTF_ENABLE_MIRRORING
			// Mirror the X coordinate across the center
			int mirroredBx = (bx < halfBlockX) ? (bx + halfBlockX) : (bx - halfBlockX);
#else
			int mirroredBx = bx;
#endif
			uint8_t* output = dst + ((by * 4) * width + (mirroredBx * 4)) * 4;

			// Debug: Block mirroring info
			// if ((by == 0 || by == 1) && bx < 4)
			// {
			//	I::CVar->ConsoleColorPrintf({200, 200, 100, 255}, "[VTFLoader]   Block[%d,%d] -> mirroredBx=%d, outputY=%d\n",
			//		bx, by, mirroredBx, by * 4);
			// }

			DecompressDXT5Block(blockStorage, output, width * 4, mirroredBx, by, width, height);
			blockStorage += 16;
		}
	}
}

void VTFLoader::ConvertBGRAtoRGBA(uint8_t* pImageBuffer, int nSize)
{
	if (!pImageBuffer)
		return;

	uint32_t* pStart = reinterpret_cast<uint32_t*>(pImageBuffer);
	uint32_t* pEnd = reinterpret_cast<uint32_t*>(pImageBuffer + nSize);

	while (pStart < pEnd)
	{
		uint32_t uColor = *pStart;
		*pStart++ = ((uColor & 0xFF00FF00) | ((uColor & 0xFF0000) >> 16) | ((uColor & 0xFF) << 16));
	}
}

bool VTFLoader::CreateTexture(uint8_t* pImageBuffer, int iWidth, int iHeight, IDirect3DTexture9** ppTexture, IDirect3DDevice9* pDevice)
{
	if (!pDevice)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] D3D device is NULL!\n");
		return false;
	}

	if (!pImageBuffer)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Image buffer is NULL!\n");
		return false;
	}

	if (!ppTexture)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Texture pointer is NULL!\n");
		return false;
	}

	// Check device state
	HRESULT deviceState = pDevice->TestCooperativeLevel();
	if (deviceState != D3D_OK)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Device not ready: HRESULT 0x%08X\n", deviceState);
		return false;
	}

	// Use D3DPOOL_DEFAULT with D3DUSAGE_DYNAMIC for compatibility
	HRESULT hr = pDevice->CreateTexture(iWidth, iHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, ppTexture, nullptr);
	if (FAILED(hr))
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] CreateTexture failed with HRESULT 0x%08X\n", hr);
		return false;
	}

	if (!*ppTexture)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Texture creation returned NULL\n");
		return false;
	}

	D3DLOCKED_RECT rect;
	if (SUCCEEDED((*ppTexture)->LockRect(0, &rect, nullptr, 0)))
	{
		// CRITICAL: D3DFMT_A8R8G8B8 expects BGRA byte order, but we have RGBA
		// We must convert RGBA → BGRA and copy row-by-row using the texture's pitch

		uint8_t* src = pImageBuffer;
		uint8_t* dst = static_cast<uint8_t*>(rect.pBits);

		for (int y = 0; y < iHeight; y++)
		{
			for (int x = 0; x < iWidth; x++)
			{
				// Source: RGBA
				uint8_t r = src[(y * iWidth + x) * 4 + 0];
				uint8_t g = src[(y * iWidth + x) * 4 + 1];
				uint8_t b = src[(y * iWidth + x) * 4 + 2];
				uint8_t a = src[(y * iWidth + x) * 4 + 3];

				// Destination: BGRA (D3DFMT_A8R8G8B8 byte order)
				dst[y * rect.Pitch + x * 4 + 0] = b;
				dst[y * rect.Pitch + x * 4 + 1] = g;
				dst[y * rect.Pitch + x * 4 + 2] = r;
				dst[y * rect.Pitch + x * 4 + 3] = a;
			}
		}

		(*ppTexture)->UnlockRect(0);
		// I::CVar->ConsoleColorPrintf({100, 255, 100, 255}, "[VTFLoader] D3D9 texture created successfully (%dx%d, pitch=%d)\n", iWidth, iHeight, rect.Pitch);
		return true;
	}

	I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Failed to lock texture for writing\n");
	(*ppTexture)->Release();
	*ppTexture = nullptr;
	return false;
}

uint8_t* VTFLoader::ReadVTFFromVPK(const char* szVPKPath, int* iWidth, int* iHeight)
{
	if (!szVPKPath || !iWidth || !iHeight)
		return nullptr;

	if (!I::BaseFileSystem)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] BaseFileSystem is NULL!\n");
		return nullptr;
	}

	FileHandle_t fileHandle = nullptr;
	std::string usedPath;

	// Build list of path variations to try
	std::vector<std::string> pathsToTry;
	std::string basePath(szVPKPath);

	// Determine if we have .vtf extension
	bool hasVtfExt = (basePath.length() > 4 && basePath.substr(basePath.length() - 4) == ".vtf");
	std::string noExtPath = hasVtfExt ? basePath.substr(0, basePath.length() - 4) : basePath;

	// Source Engine stores VTF files in VPK WITHOUT the .vtf extension
	// and WITHOUT the materials/ prefix (it's implicit)
	// Try these variations in order of most likely to work:

	// 1. No materials/ prefix, no .vtf extension (most common for VPK)
	pathsToTry.push_back(noExtPath);

	// 2. No materials/ prefix, with .vtf extension
	if (hasVtfExt)
		pathsToTry.push_back(basePath);

	// 3. With materials/ prefix, no .vtf extension
	pathsToTry.push_back("materials/" + noExtPath);

	// 4. With materials/ prefix, with .vtf extension
	if (hasVtfExt)
		pathsToTry.push_back("materials/" + basePath);

	// Try each path variation
	for (const auto& path : pathsToTry)
	{
		fileHandle = I::BaseFileSystem->Open(path.c_str(), "r", "GAME");
		if (fileHandle)
		{
			usedPath = path;
			// I::CVar->ConsoleColorPrintf({100, 255, 100, 255}, "[VTFLoader] Opened: %s\n", path.c_str());
			break;
		}
	}

	if (!fileHandle)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Failed to open: %s\n", szVPKPath);
		I::CVar->ConsoleColorPrintf({255, 200, 100, 255}, "[VTFLoader] Tried:\n");
		for (const auto& path : pathsToTry)
		{
			I::CVar->ConsoleColorPrintf({255, 200, 100, 255}, "  - %s\n", path.c_str());
		}
		return nullptr;
	}

	int fileSize = I::BaseFileSystem->Size(fileHandle);
	if (fileSize < sizeof(VTFHeader))
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] File too small: %d bytes (need %d)\n", fileSize, sizeof(VTFHeader));
		I::BaseFileSystem->Close(fileHandle);
		return nullptr;
	}

	uint8_t* fileData = new uint8_t[fileSize];
	I::BaseFileSystem->Read(fileData, fileSize, fileHandle);
	I::BaseFileSystem->Close(fileHandle);

	VTFHeader* header = reinterpret_cast<VTFHeader*>(fileData);

	if (header->signature[0] != 'V' || header->signature[1] != 'T' || header->signature[2] != 'F')
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Invalid signature: %c%c%c\n",
			header->signature[0], header->signature[1], header->signature[2]);
		delete[] fileData;
		return nullptr;
	}

	if (header->version[0] != 7 || header->version[1] < 0 || header->version[1] > 5)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Unsupported version: %d.%d\n",
			header->version[0], header->version[1]);
		delete[] fileData;
		return nullptr;
	}

	// I::CVar->ConsoleColorPrintf({100, 200, 255, 255}, "[VTFLoader] Header: %dx%d, format=%d, mipmaps=%d\n",
	//	header->width, header->height, header->highResImageFormat, header->mipmapCount);

	*iWidth = header->width;
	*iHeight = header->height;

	int imageWidth = header->width;
	int imageHeight = header->height;
	int imageFormat = header->highResImageFormat;
	int mipmapCount = header->mipmapCount;

	int offset = header->headerSize;
	// I::CVar->ConsoleColorPrintf({100, 200, 255, 255}, "[VTFLoader] Header size: %d\n", header->headerSize);

	if (header->lowResImageFormat != IMAGE_FORMAT_NONE && header->lowResImageWidth > 0 && header->lowResImageHeight > 0)
	{
		int lowResSize = header->lowResImageWidth * header->lowResImageHeight;
		if (header->lowResImageFormat == IMAGE_FORMAT_DXT1)
			lowResSize = std::max(1, (header->lowResImageWidth + 3) / 4) * std::max(1, (header->lowResImageHeight + 3) / 4) * 8;
		else if (header->lowResImageFormat == IMAGE_FORMAT_DXT5)
			lowResSize = std::max(1, (header->lowResImageWidth + 3) / 4) * std::max(1, (header->lowResImageHeight + 3) / 4) * 16;

		// I::CVar->ConsoleColorPrintf({100, 200, 255, 255}, "[VTFLoader] Low-res thumbnail: %dx%d, format=%d, size=%d\n",
		//	header->lowResImageWidth, header->lowResImageHeight, header->lowResImageFormat, lowResSize);
		offset += lowResSize;
	}

	// VTF stores mipmaps from smallest to largest
	// We need to skip all mipmaps except mipmap 0 (full resolution)
	// Calculate size of each mipmap starting from the smallest

	// Start from the smallest mipmap dimensions
	int mipWidth = imageWidth >> (mipmapCount - 1);
	int mipHeight = imageHeight >> (mipmapCount - 1);
	if (mipWidth < 1) mipWidth = 1;
	if (mipHeight < 1) mipHeight = 1;

	// Skip all mipmaps except the last one (mipmap 0 = full resolution)
	for (int i = mipmapCount - 1; i > 0; i--)
	{
		int mipSize = 0;
		if (imageFormat == IMAGE_FORMAT_DXT1 || imageFormat == IMAGE_FORMAT_DXT1_ONEBITALPHA)
			mipSize = std::max(1, (mipWidth + 3) / 4) * std::max(1, (mipHeight + 3) / 4) * 8;
		else if (imageFormat == IMAGE_FORMAT_DXT5)
			mipSize = std::max(1, (mipWidth + 3) / 4) * std::max(1, (mipHeight + 3) / 4) * 16;
		else if (imageFormat == IMAGE_FORMAT_RGBA8888 || imageFormat == IMAGE_FORMAT_ABGR8888 || imageFormat == IMAGE_FORMAT_ARGB8888 || imageFormat == IMAGE_FORMAT_BGRA8888)
			mipSize = mipWidth * mipHeight * 4;
		else if (imageFormat == IMAGE_FORMAT_RGB888 || imageFormat == IMAGE_FORMAT_BGR888)
			mipSize = mipWidth * mipHeight * 3;
		else if (imageFormat == IMAGE_FORMAT_I8)
			mipSize = mipWidth * mipHeight;

		// I::CVar->ConsoleColorPrintf({100, 200, 255, 255}, "[VTFLoader] Mip %d: %dx%d = %d bytes, offset now %d\n",
		//	i, mipWidth, mipHeight, mipSize, offset + mipSize);

		offset += mipSize;

		// Move to next larger mipmap
		mipWidth = std::min(imageWidth, mipWidth * 2);
		mipHeight = std::min(imageHeight, mipHeight * 2);
	}

	// I::CVar->ConsoleColorPrintf({100, 200, 255, 255}, "[VTFLoader] Final offset: %d (full-res mip is %dx%d)\n",
	//	offset, imageWidth, imageHeight);

	uint8_t* rgbaData = new uint8_t[imageWidth * imageHeight * 4];
	memset(rgbaData, 0, imageWidth * imageHeight * 4);

	// Calculate actual compressed size based on format
	int compressedSize = 0;
	if (imageFormat == IMAGE_FORMAT_DXT1 || imageFormat == IMAGE_FORMAT_DXT1_ONEBITALPHA)
		compressedSize = std::max(1, (imageWidth + 3) / 4) * std::max(1, (imageHeight + 3) / 4) * 8;
	else if (imageFormat == IMAGE_FORMAT_DXT5)
		compressedSize = std::max(1, (imageWidth + 3) / 4) * std::max(1, (imageHeight + 3) / 4) * 16;
	else if (imageFormat == IMAGE_FORMAT_RGBA8888 || imageFormat == IMAGE_FORMAT_ABGR8888 || imageFormat == IMAGE_FORMAT_ARGB8888 || imageFormat == IMAGE_FORMAT_BGRA8888)
		compressedSize = imageWidth * imageHeight * 4;
	else if (imageFormat == IMAGE_FORMAT_RGB888 || imageFormat == IMAGE_FORMAT_BGR888)
		compressedSize = imageWidth * imageHeight * 3;
	else if (imageFormat == IMAGE_FORMAT_I8)
		compressedSize = imageWidth * imageHeight;

	if (offset + compressedSize > fileSize)
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Offset out of bounds: offset=%d, compressedSize=%d, fileSize=%d\n",
			offset, compressedSize, fileSize);
		delete[] fileData;
		delete[] rgbaData;
		return nullptr;
	}

	uint8_t* imageData = fileData + offset;

	if (imageFormat == IMAGE_FORMAT_DXT1 || imageFormat == IMAGE_FORMAT_DXT1_ONEBITALPHA)
	{
		DecompressDXT1(imageData, rgbaData, imageWidth, imageHeight);
	}
	else if (imageFormat == IMAGE_FORMAT_DXT5)
	{
		DecompressDXT5(imageData, rgbaData, imageWidth, imageHeight);
	}
	else if (imageFormat == IMAGE_FORMAT_RGBA8888)
	{
		memcpy(rgbaData, imageData, imageWidth * imageHeight * 4);
	}
	else if (imageFormat == IMAGE_FORMAT_BGRA8888)
	{
		// BGRA8888 -> RGBA8888 with swapped-half fix
		int halfWidth = imageWidth / 2;

		// I::CVar->ConsoleColorPrintf({200, 200, 100, 255}, "[VTFLoader] BGRA8888: %dx%d pixels, halfWidth=%d\n",
		//	imageWidth, imageHeight, halfWidth);

		for (int y = 0; y < imageHeight; y++)
		{
			for (int x = 0; x < imageWidth; x++)
			{
#if VTF_ENABLE_MIRRORING
				// Mirror X coordinate
				int mirroredX = (x < halfWidth) ? (x + halfWidth) : (x - halfWidth);
				// BGRA8888 has 1-pixel vertical offset on right half - shift it up
				int yAdjust = (mirroredX >= imageWidth / 2) ? -1 : 0;
#else
				int mirroredX = x;
				int yAdjust = 0;
#endif
				// Skip if Y would go out of bounds
				if (y + yAdjust < 0 || y + yAdjust >= imageHeight)
					continue;

				int srcIndex = y * imageWidth + x;
				int dstIndex = (y + yAdjust) * imageWidth + mirroredX;

				// Debug: Pixel mirroring info
				// if (y == 0 && x < 8)
				// {
				//	I::CVar->ConsoleColorPrintf({200, 200, 100, 255}, "[VTFLoader]   Pixel[%d,%d] -> mirroredX=%d, yAdjust=%d\n",
				//		x, y, mirroredX, yAdjust);
				// }

				// BGRA -> RGBA
				uint8_t b = imageData[srcIndex * 4 + 0];
				uint8_t g = imageData[srcIndex * 4 + 1];
				uint8_t r = imageData[srcIndex * 4 + 2];
				uint8_t a = imageData[srcIndex * 4 + 3];
				rgbaData[dstIndex * 4 + 0] = r;
				rgbaData[dstIndex * 4 + 1] = g;
				rgbaData[dstIndex * 4 + 2] = b;
				rgbaData[dstIndex * 4 + 3] = a;
			}
		}
	}
	else if (imageFormat == IMAGE_FORMAT_ARGB8888)
	{
		// ARGB8888 -> RGBA8888: Rotate bytes
		// Also apply swapped-half fix (left/right halves are swapped in VTF)
		int halfWidth = imageWidth / 2;

		// I::CVar->ConsoleColorPrintf({200, 200, 100, 255}, "[VTFLoader] ARGB8888: %dx%d pixels, halfWidth=%d\n",
		//	imageWidth, imageHeight, halfWidth);

		for (int y = 0; y < imageHeight; y++)
		{
			for (int x = 0; x < imageWidth; x++)
			{
#if VTF_ENABLE_MIRRORING
				// Mirror X coordinate
				int mirroredX = (x < halfWidth) ? (x + halfWidth) : (x - halfWidth);
				// ARGB8888 has 1-pixel vertical offset on right half - shift it up
				int yAdjust = (mirroredX >= imageWidth / 2) ? -1 : 0;
#else
				int mirroredX = x;
				int yAdjust = 0;
#endif
				// Skip if Y would go out of bounds
				if (y + yAdjust < 0 || y + yAdjust >= imageHeight)
					continue;

				int srcIndex = y * imageWidth + x;
				int dstIndex = (y + yAdjust) * imageWidth + mirroredX;

				// Debug: Pixel mirroring info
				// if (y == 0 && x < 8)
				// {
				//	I::CVar->ConsoleColorPrintf({200, 200, 100, 255}, "[VTFLoader]   Pixel[%d,%d] -> mirroredX=%d, yAdjust=%d\n",
				//		x, y, mirroredX, yAdjust);
				// }

				uint8_t a = imageData[srcIndex * 4 + 0];
				uint8_t r = imageData[srcIndex * 4 + 1];
				uint8_t g = imageData[srcIndex * 4 + 2];
				uint8_t b = imageData[srcIndex * 4 + 3];
				rgbaData[dstIndex * 4 + 0] = r;
				rgbaData[dstIndex * 4 + 1] = g;
				rgbaData[dstIndex * 4 + 2] = b;
				rgbaData[dstIndex * 4 + 3] = a;
			}
		}
	}
	else if (imageFormat == IMAGE_FORMAT_ABGR8888)
	{
		// ABGR8888 -> RGBA8888: Swap R and B
		for (int i = 0; i < imageWidth * imageHeight; i++)
		{
			uint8_t a = imageData[i * 4 + 0];
			uint8_t b = imageData[i * 4 + 1];
			uint8_t g = imageData[i * 4 + 2];
			uint8_t r = imageData[i * 4 + 3];
			rgbaData[i * 4 + 0] = r;
			rgbaData[i * 4 + 1] = g;
			rgbaData[i * 4 + 2] = b;
			rgbaData[i * 4 + 3] = a;
		}
	}
	else if (imageFormat == IMAGE_FORMAT_RGB888)
	{
		for (int i = 0; i < imageWidth * imageHeight; i++)
		{
			rgbaData[i * 4 + 0] = imageData[i * 3 + 0];
			rgbaData[i * 4 + 1] = imageData[i * 3 + 1];
			rgbaData[i * 4 + 2] = imageData[i * 3 + 2];
			rgbaData[i * 4 + 3] = 255;
		}
	}
	else if (imageFormat == IMAGE_FORMAT_BGR888)
	{
		for (int i = 0; i < imageWidth * imageHeight; i++)
		{
			rgbaData[i * 4 + 0] = imageData[i * 3 + 2];
			rgbaData[i * 4 + 1] = imageData[i * 3 + 1];
			rgbaData[i * 4 + 2] = imageData[i * 3 + 0];
			rgbaData[i * 4 + 3] = 255;
		}
	}
	else if (imageFormat == IMAGE_FORMAT_I8)
	{
		for (int i = 0; i < imageWidth * imageHeight; i++)
		{
			rgbaData[i * 4 + 0] = imageData[i];
			rgbaData[i * 4 + 1] = imageData[i];
			rgbaData[i * 4 + 2] = imageData[i];
			rgbaData[i * 4 + 3] = 255;
		}
	}
	else
	{
		I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Unsupported image format: %d\n", imageFormat);
		delete[] fileData;
		delete[] rgbaData;
		return nullptr;
	}

	delete[] fileData;
	// I::CVar->ConsoleColorPrintf({100, 255, 100, 255}, "[VTFLoader] Successfully decompressed image data\n");
	return rgbaData;
}

void VTFLoader::FreeImage(uint8_t* pImageBuffer)
{
	delete[] pImageBuffer;
}
