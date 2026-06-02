// ASCII C/C++ TAB4 CRLF
// Docutitle: Picture
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/


#ifndef _INC_PICTURE
#define _INC_PICTURE

#include "trait/StorageTrait.hpp"
#include "trait/MallocTrait.hpp"
#include "nnode"
#include "../c/graphic.h"

namespace uni {
	enum class ImageResult
	{
		OK,
		FAILED,
		UNSUPPORTED,
		INVALID_FORMAT,
		INVALID_ARGUMENT,
		OUT_OF_MEMORY,
		IO_ERROR,
		NOT_FOUND,
		ACCESS_DENIED,
		BUFFER_TOO_SMALL,
		END_OF_STREAM,
	};

	enum class ImageFormat
	{
		UNKNOWN,
		BMP,
		PNG,
		JPEG,
		WEBP,
		GIF,
		TGA,
		ICO,
		TIFF,
	};

	// enum class PixelFormat in inc/c/graphic/color.h

	enum class ColorSpace
	{
		UNKNOWN,
		SRGB,
		LINEAR,
		GRAY,
		CMYK,
	};

	enum class ImageAccessMode {
		READ_ONLY,
		READ_WRITE,
	};

	enum class ImageAlphaMode
	{
		NONE,
		STRAIGHT,
		PREMULTIPLIED,
	};

	struct ImageInfo
	{
		uint32_t		width;
		uint32_t		height;

		PixelFormat		format;
		ColorSpace		colorSpace;
		ImageAlphaMode	alphaMode;
		ImageFormat		fileFormat;

		uint32_t		bitsPerPixel;
		uint32_t		frameCount;
		bool			hasAlpha;
		bool			hasAnimation;
	};

	// pixels == nullptr:
	//	empty buffer
	//
	// pixels != nullptr && allocator != nullptr:
	//	buffer owns pixels, must be released by allocator
	//
	// pixels != nullptr && allocator == nullptr:
	//	non-owning borrowed pixels
	struct ImageBuffer
	{
		uint32_t		width;
		uint32_t		height;
		uint32_t		stride;

		PixelFormat		format;
		ColorSpace		colorSpace;
		ImageAlphaMode	alphaMode;

		void* pixels;
		size_t			size;
		trait::Malloc*	allocator;
	};

	class IImageObject
	{
	public:

		// Release this object.
		// Caller must not use this object after Release().
		virtual void Release() = 0;

	protected:
		virtual ~IImageObject() = default;
	};
}

// Stream : StorageTrait
namespace uni {}

// Metadata
namespace uni {
	enum class ImageMetadataType
	{
		UNKNOWN,
		TEXT,
		EXIF,
		ICC_PROFILE,
		GAMMA,
		ORIENTATION,
		ANIMATION,
	};

	struct ImageMetadataItem
	{
		ImageMetadataType	type;
		const char* key;
		const void* data;
		size_t				size;
	};

	class IImageMetadata : public IImageObject {
	public:
		// Get metadata item count.
		virtual uint32_t GetCount() const = 0;

		// Get metadata item by index.
		virtual ImageResult GetItem(uint32_t index, ImageMetadataItem& outItem) const = 0;

	protected:
		virtual ~IImageMetadata() = default;
	};
}

// Lazy Surface
namespace uni {

	enum class ImageSurfaceCapability : uint32_t
	{
		NONE = 0,
		FULL_READ = 1 << 0,
		REGION_READ = 1 << 1,
		SCANLINE_READ = 1 << 2,
		TILE_READ = 1 << 3,
		REGION_WRITE = 1 << 4,
		FLUSH = 1 << 5,
		RANDOM_STORAGE	= 1 << 6,
	};

	inline ImageSurfaceCapability operator | (ImageSurfaceCapability a, ImageSurfaceCapability b) {
		return static_cast<ImageSurfaceCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}
	inline ImageSurfaceCapability operator & (ImageSurfaceCapability a, ImageSurfaceCapability b) {
		return static_cast<ImageSurfaceCapability>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}
	inline bool ImageHasCapability(ImageSurfaceCapability flags, ImageSurfaceCapability cap) {
		return(static_cast<uint32_t>(flags) & static_cast<uint32_t>(cap)) != 0;
	}

	class IImageSurface : public IImageObject {
	public:

		// Get image information without forcing full decode.
		virtual ImageResult GetInfo(ImageInfo& outInfo) const = 0;

		// Get surface capabilities.
		virtual ImageSurfaceCapability GetCapabilities() const = 0;

		// Get image metadata.
		// On success, outMetadata receives a borrowed pointer valid
		// while this surface remains alive. Caller must not Release() it.
		// If metadata was not requested during open/decode, this may return NOT_FOUND.
		// Codecs that do not support metadata may return UNSUPPORTED.
		virtual ImageResult GetMetadata(IImageMetadata*& outMetadata) = 0;

		// Read pixels from region.
		// If outBuffer already owns memory, caller should free it first.
		// On success, outBuffer.allocator must be non-null if outBuffer owns pixels.
		virtual ImageResult ReadPixels(
			const Rectangle& rect,
			ImageBuffer& outBuffer,
			trait::Malloc& allocator
		) = 0;

		// Write pixels into region.
		virtual ImageResult WritePixels(
			const Rectangle& rect,
			const ImageBuffer& srcBuffer
		) = 0;

		// Flush dirty data.
		virtual ImageResult	Flush() = 0;

	protected:
		virtual ~IImageSurface() = default;
	};
}

// Codec
namespace uni {
	enum class ImageDecodeFlags : uint32_t
	{
		NONE = 0,
		FORCE_RGBA = 1 << 0,
		APPLY_ORIENTATION = 1 << 1,
		// Try to load metadata for later GetMetadata() access.
		// If this flag is not set, GetMetadata() may return NOT_FOUND.
		LOAD_METADATA = 1 << 2,
		IGNORE_GAMMA = 1 << 3,
	};

	inline ImageDecodeFlags operator | (ImageDecodeFlags a, ImageDecodeFlags b) {
		return static_cast<ImageDecodeFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}
	inline ImageDecodeFlags operator & (ImageDecodeFlags a, ImageDecodeFlags b) {
		return static_cast<ImageDecodeFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}
	inline bool ImageHasFlag(ImageDecodeFlags flags, ImageDecodeFlags flag) {
		return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
	}

	enum class ImageEncodeFlags : uint32_t
	{
		NONE = 0,
		PRESERVE_ALPHA = 1 << 0,
		WRITE_METADATA = 1 << 1,
		PROGRESSIVE = 1 << 2,
	};

	inline ImageEncodeFlags operator | (ImageEncodeFlags a, ImageEncodeFlags b) {
		return static_cast<ImageEncodeFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}
	inline ImageEncodeFlags operator & (ImageEncodeFlags a, ImageEncodeFlags b) {
		return static_cast<ImageEncodeFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}
	inline bool ImageHasFlag(ImageEncodeFlags flags, ImageEncodeFlags flag) {
		return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
	}

	struct ImageDecodeOptions
	{
		uint32_t	structSize;
		uint32_t	version;
		ImageDecodeFlags	flags;

		PixelFormat	preferredFormat;
		ColorSpace	preferredColorSpace;

		uint32_t	targetFrame;
		uint32_t	reserved[8];
	};

	struct ImageEncodeOptions
	{
		uint32_t	structSize;
		uint32_t	version;
		ImageEncodeFlags	flags;

		uint32_t	quality;
		uint32_t	compression;

		uint32_t	reserved[8];
	};

	class IImageCodec
	{
	public:

		virtual ~IImageCodec() = default;

		// Get codec name.
		virtual const char* GetName() const = 0;

		// Get image file format.
		virtual ImageFormat GetFormat() const = 0;

		// Get supported extensions, null-terminated.
		virtual const char* const* GetExtensions() const = 0;

		// Probe image format from storage.
		// This function must read by absolute byte offset.
		// This function must not modify storage state.
		virtual ImageResult Probe(StorageTrait& storage, bool& matched) const = 0;

		// Read image header and basic information.
		// This function must read by absolute byte offset.
		// This function must not modify storage state.
		virtual ImageResult ReadInfo(StorageTrait& storage, ImageInfo& outInfo) const = 0;

		// Open lazy image surface.
		virtual ImageResult OpenSurface(
			StorageTrait& storage,
			IImageSurface*& outSurface,
			trait::Malloc& allocator,
			const ImageDecodeOptions& options,
			ImageAccessMode access
		) const = 0;

		// Decode full image into memory.
		virtual ImageResult Decode(
			StorageTrait& storage,
			ImageBuffer& outBuffer,
			trait::Malloc& allocator,
			const ImageDecodeOptions& options
		) const = 0;

		// Encode full image into storage.
		virtual ImageResult Encode(
			const ImageBuffer& image,
			StorageTrait& storage,
			trait::Malloc& allocator,
			const ImageEncodeOptions& options
		) const = 0;

		// Check whether encoder can write this pixel format.
		virtual bool CanEncode(PixelFormat format) const = 0;
	};
}

namespace uni {
	class IImageCodecManager
	{
	public:

		virtual ~IImageCodecManager() = default;

		// Register image codec.
		virtual ImageResult RegisterCodec(IImageCodec* codec) = 0;

		// Unregister image codec.
		virtual ImageResult UnregisterCodec(IImageCodec* codec) = 0;

		// Find decoder by storage probing.
		virtual ImageResult FindDecoder(StorageTrait& storage, const IImageCodec*& outCodec) const = 0;

		// Find encoder by format.
		virtual ImageResult FindEncoder(ImageFormat format, const IImageCodec*& outCodec) const = 0;

		// Get registered codec count.
		virtual uint32_t GetCodecCount() const = 0;

		// Get registered codec by index.
		virtual const IImageCodec* GetCodec(uint32_t index) const = 0;
	};
}

// Filter
namespace uni {
	struct ImageFilterContext {
		trait::Malloc* allocator;
	};

	class IImageFilter
	{
	public:

		virtual ~IImageFilter() = default;

		// Get filter name.
		virtual const char* GetName() const = 0;

		// Apply filter.
		virtual ImageResult Apply(
			const ImageBuffer& src,
			ImageBuffer& dst,
			ImageFilterContext& context
		) = 0;
	};
}

// Layer / Document
namespace uni {
	enum class ImageBlendMode
	{
		NORMAL,
		MULTIPLY,
		SCREEN,
		OVERLAY,
		ADD,
		SUBTRACT,
	};

	class IImageLayer : public IImageObject {
	public:
		// Get layer name.
		virtual const char* GetName() const = 0;

		// Set layer name.
		virtual ImageResult SetName(const char* name) = 0;

		// Get layer surface.
		// Returned pointer is borrowed. Caller must not Release() it.
		virtual IImageSurface* GetSurface() = 0;

		// Get layer opacity.
		virtual float GetOpacity() const = 0;

		// Set layer opacity.
		virtual void SetOpacity(float opacity) = 0;

		// Get blend mode.
		virtual ImageBlendMode GetBlendMode() const = 0;

		// Set blend mode.
		virtual void SetBlendMode(ImageBlendMode mode) = 0;

		// Check whether layer is visible.
		virtual bool IsVisible() const = 0;

		// Set layer visibility.
		virtual void SetVisible(bool visible) = 0;

	protected:
		virtual ~IImageLayer() = default;
	};
	class IImageDocument : public IImageObject {
	public:
		// Get canvas size.
		virtual Size2 GetCanvasSize() const = 0;

		// Resize canvas.
		virtual ImageResult ResizeCanvas(uint32_t width, uint32_t height) = 0;

		// Get layer count.
		virtual uint32_t GetLayerCount() const = 0;

		// Get borrowed layer pointer.
		// Caller must not Release() the returned layer.
		virtual IImageLayer* GetLayer(uint32_t index) = 0;

		// Add layer.
		// Document takes ownership of layer on success.
		virtual ImageResult AddLayer(IImageLayer* layer) = 0;

		// Remove layer.
		// Document releases the removed layer.
		virtual ImageResult RemoveLayer(uint32_t index) = 0;

		//{FUTURE} DetachLayer

		// Flatten document into one image.
		virtual ImageResult Flatten(
			ImageBuffer& outBuffer,
			trait::Malloc& allocator
		) = 0;

	protected:
		virtual ~IImageDocument() = default;
	};
}

// System
namespace uni {
	class IImageSystem
	{
	public:

		virtual ~IImageSystem() = default;

		// Get allocator.
		virtual trait::Malloc* GetAllocator() = 0;

		// Get codec manager.
		virtual IImageCodecManager* GetCodecManager() = 0;

		// Get codec manager.
		virtual const IImageCodecManager* GetCodecManager() const = 0;

		// Load full image.
		virtual ImageResult
			LoadImage(
				StorageTrait& storage,
				ImageBuffer& outBuffer,
				const ImageDecodeOptions& options
			) = 0;

		// Open lazy image surface.
		virtual ImageResult OpenImage(
			StorageTrait& storage,
			IImageSurface*& outSurface,
			const ImageDecodeOptions& options,
			ImageAccessMode access
		) = 0;

		// Save full image.
		virtual ImageResult SaveImage(
			const ImageBuffer& image,
			StorageTrait& storage,
			ImageFormat format,
			const ImageEncodeOptions& options
		) = 0;

		// Free image buffer.
		// Equivalent to ImageBufferFree(image).
		virtual void FreeImage(ImageBuffer& image) = 0;
	};


	inline void ImageBufferClear(ImageBuffer& buffer) {
		buffer.width = 0;
		buffer.height = 0;
		buffer.stride = 0;
		buffer.format = PixelFormat::UNKNOWN;
		buffer.colorSpace = ColorSpace::UNKNOWN;
		buffer.alphaMode = ImageAlphaMode::NONE;
		buffer.pixels = nullptr;
		buffer.size = 0;
		buffer.allocator = nullptr;
	}

	inline void ImageBufferFree(ImageBuffer& buffer) {
		if (buffer.pixels && buffer.allocator) {
			buffer.allocator->deallocate(buffer.pixels, buffer.size);
		}
		ImageBufferClear(buffer);
	}

	inline void ImageDecodeOptionsInit(ImageDecodeOptions& options) {
		options.structSize = sizeof(ImageDecodeOptions);
		options.version = 1;
		options.flags =
			ImageDecodeFlags::FORCE_RGBA |
			ImageDecodeFlags::APPLY_ORIENTATION;

		options.preferredFormat = PixelFormat::RGBA8888;
		options.preferredColorSpace = ColorSpace::SRGB;
		options.targetFrame = 0;

		for (uint32_t i = 0; i < 8; ++i)
		{
			options.reserved[i] = 0;
		}
	}
	inline void ImageEncodeOptionsInit(ImageEncodeOptions& options) {
		options.structSize = sizeof(ImageEncodeOptions);
		options.version = 1;
		options.flags = ImageEncodeFlags::PRESERVE_ALPHA;

		options.quality = 90;
		options.compression = 6;

		for (uint32_t i = 0; i < 8; ++i)
		{
			options.reserved[i] = 0;
		}
	}
}

#endif // _INC_PICTURE
