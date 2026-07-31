#include "magritte/io/file.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

#include <algorithm>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "magritte/utils/logging.h"

namespace fs = std::filesystem;

namespace {
    using EncodedData = std::vector<char>;

    template<typename Pointer>
    using ScopedPointer = std::unique_ptr<
        std::remove_pointer_t<Pointer>,
        void (*)(Pointer)
    >;

    EncodedData read_encoded_file(const fs::path &input) {
        std::ifstream file(input, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("could not open input file: " + input.string());
        }

        const std::streampos end = file.tellg();
        if (end < 0) {
            throw std::runtime_error(
                "could not determine input file size: " + input.string()
            );
        }

        EncodedData data(static_cast<std::size_t>(end));
        file.seekg(0, std::ios::beg);
        if (!data.empty()) {
            file.read(data.data(), static_cast<std::streamsize>(data.size()));
        }
        if (!file) {
            throw std::runtime_error("could not read input file: " + input.string());
        }
        return data;
    }

    FileData decode_jpeg(const EncodedData &encoded) {
        if (encoded.size() >
            static_cast<std::size_t>(std::numeric_limits<CFIndex>::max())) {
            throw std::runtime_error("input image is too large to decode");
        }

        ScopedPointer<CFDataRef> source_data(
            CFDataCreate(
                kCFAllocatorDefault,
                reinterpret_cast<const UInt8 *>(encoded.data()),
                static_cast<CFIndex>(encoded.size())
            ),
            [](CFDataRef value) {
                if (value != nullptr) {
                    CFRelease(value);
                }
            }
        );
        if (!source_data) {
            throw std::runtime_error("could not allocate JPEG source data");
        }

        ScopedPointer<CGImageSourceRef> source(
            CGImageSourceCreateWithData(source_data.get(), nullptr),
            [](CGImageSourceRef value) {
                if (value != nullptr) {
                    CFRelease(value);
                }
            }
        );
        if (!source) {
            throw std::runtime_error("could not create JPEG decoder");
        }

        ScopedPointer<CGImageRef> source_image(
            CGImageSourceCreateImageAtIndex(source.get(), 0, nullptr),
            CGImageRelease
        );
        if (!source_image) {
            throw std::runtime_error("could not decode JPEG image");
        }

        const std::int64_t max_dimension = static_cast<std::int64_t>(
            std::max(
                CGImageGetWidth(source_image.get()),
                CGImageGetHeight(source_image.get())
            )
        );
        ScopedPointer<CFNumberRef> max_pixel_size(
            CFNumberCreate(
                kCFAllocatorDefault,
                kCFNumberSInt64Type,
                &max_dimension
            ),
            [](CFNumberRef value) {
                if (value != nullptr) {
                    CFRelease(value);
                }
            }
        );
        if (!max_pixel_size) {
            throw std::runtime_error("could not create JPEG orientation options");
        }

        const void *option_keys[] = {
            kCGImageSourceCreateThumbnailFromImageAlways,
            kCGImageSourceCreateThumbnailWithTransform,
            kCGImageSourceThumbnailMaxPixelSize,
        };
        const void *option_values[] = {
            kCFBooleanTrue,
            kCFBooleanTrue,
            max_pixel_size.get(),
        };
        ScopedPointer<CFDictionaryRef> options(
            CFDictionaryCreate(
                kCFAllocatorDefault,
                option_keys,
                option_values,
                3,
                &kCFTypeDictionaryKeyCallBacks,
                &kCFTypeDictionaryValueCallBacks
            ),
            [](CFDictionaryRef value) {
                if (value != nullptr) {
                    CFRelease(value);
                }
            }
        );
        if (!options) {
            throw std::runtime_error("could not create JPEG orientation options");
        }

        ScopedPointer<CGImageRef> image(
            CGImageSourceCreateThumbnailAtIndex(source.get(), 0, options.get()),
            CGImageRelease
        );
        if (!image) {
            throw std::runtime_error("could not normalize JPEG orientation");
        }

        const std::size_t width = CGImageGetWidth(image.get());
        const std::size_t height = CGImageGetHeight(image.get());
        if (width == 0 || height == 0 ||
            width > std::numeric_limits<std::size_t>::max() / height) {
            throw std::runtime_error("JPEG has invalid dimensions");
        }

        FileData decoded{
            .width = width,
            .height = height,
            .pixels = std::vector<Pixel>(width * height),
        };

        ScopedPointer<CGColorSpaceRef> color_space(
            CGColorSpaceCreateDeviceRGB(),
            CGColorSpaceRelease
        );
        if (!color_space) {
            throw std::runtime_error("could not create RGB color space");
        }

        constexpr CGBitmapInfo bitmap_info = static_cast<CGBitmapInfo>(
            static_cast<std::uint32_t>(kCGImageAlphaPremultipliedLast) |
            static_cast<std::uint32_t>(kCGBitmapByteOrder32Big)
        );
        ScopedPointer<CGContextRef> context(
            CGBitmapContextCreate(
                decoded.pixels.data(),
                decoded.width,
                decoded.height,
                8,
                decoded.width * sizeof(Pixel),
                color_space.get(),
                bitmap_info
            ),
            CGContextRelease
        );
        if (!context) {
            throw std::runtime_error("could not create JPEG pixel buffer");
        }

        CGContextSetBlendMode(context.get(), kCGBlendModeCopy);
        CGContextTranslateCTM(context.get(), 0.0, static_cast<CGFloat>(decoded.height));
        CGContextScaleCTM(context.get(), 1.0, -1.0);
        CGContextDrawImage(
            context.get(),
            CGRectMake(
                0.0,
                0.0,
                static_cast<CGFloat>(decoded.width),
                static_cast<CGFloat>(decoded.height)
            ),
            image.get()
        );

        return decoded;
    }
} // namespace

void validate_file_data(const FileData &data) {
    if (data.width == 0 || data.height == 0 ||
        data.width > std::numeric_limits<std::size_t>::max() / data.height ||
        data.pixels.size() != data.width * data.height) {
        throw std::runtime_error("step returned invalid pixel data");
    }
}

FileData read_file(const fs::path &input) {
    log(LogLevel::info, "Reading file: " + input.string());
    FileData data = decode_jpeg(read_encoded_file(input));
    log(
        LogLevel::info,
        "Decoded " + std::to_string(data.width) + "x" +
        std::to_string(data.height) + " pixels"
    );
    return data;
}

void save_file(const fs::path &output, const FileData &data) {
    validate_file_data(data);

    if (!output.parent_path().empty()) {
        std::error_code error;
        fs::create_directories(output.parent_path(), error);
        if (error) {
            throw std::runtime_error(
                "could not create output directory: " + error.message()
            );
        }
    }

    log(LogLevel::info, "Encoding JPEG: " + output.string());

    std::vector<Pixel> encoded_pixels(data.pixels.size());
    for (std::size_t y = 0; y < data.height; ++y) {
        const auto source = data.pixels.begin() +
                            static_cast<std::ptrdiff_t>(y * data.width);
        const auto destination = encoded_pixels.begin() +
                                 static_cast<std::ptrdiff_t>((data.height - 1 - y) * data.width);
        std::copy_n(source, data.width, destination);
    }

    ScopedPointer<CGColorSpaceRef> color_space(
        CGColorSpaceCreateDeviceRGB(),
        CGColorSpaceRelease
    );
    if (!color_space) {
        throw std::runtime_error("could not create RGB color space");
    }

    constexpr CGBitmapInfo bitmap_info = static_cast<CGBitmapInfo>(
        static_cast<std::uint32_t>(kCGImageAlphaPremultipliedLast) |
        static_cast<std::uint32_t>(kCGBitmapByteOrder32Big)
    );
    ScopedPointer<CGContextRef> context(
        CGBitmapContextCreate(
            encoded_pixels.data(),
            data.width,
            data.height,
            8,
            data.width * sizeof(Pixel),
            color_space.get(),
            bitmap_info
        ),
        CGContextRelease
    );
    if (!context) {
        throw std::runtime_error("could not create output pixel buffer");
    }

    ScopedPointer<CGImageRef> image(
        CGBitmapContextCreateImage(context.get()),
        CGImageRelease
    );
    if (!image) {
        throw std::runtime_error("could not create output image");
    }

    ScopedPointer<CFMutableDataRef> encoded(
        CFDataCreateMutable(kCFAllocatorDefault, 0),
        [](CFMutableDataRef value) {
            if (value != nullptr) {
                CFRelease(value);
            }
        }
    );
    if (!encoded) {
        throw std::runtime_error("could not allocate encoded JPEG data");
    }

    ScopedPointer<CGImageDestinationRef> destination(
        CGImageDestinationCreateWithData(
            encoded.get(),
            CFSTR("public.jpeg"),
            1,
            nullptr
        ),
        [](CGImageDestinationRef value) {
            if (value != nullptr) {
                CFRelease(value);
            }
        }
    );
    if (!destination) {
        throw std::runtime_error("could not create JPEG encoder");
    }

    CGImageDestinationAddImage(destination.get(), image.get(), nullptr);
    if (!CGImageDestinationFinalize(destination.get())) {
        throw std::runtime_error("could not encode output JPEG");
    }

    std::ofstream file(output, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error(
            "could not open output file: " + output.string()
        );
    }

    const CFIndex encoded_size = CFDataGetLength(encoded.get());
    file.write(
        reinterpret_cast<const char *>(CFDataGetBytePtr(encoded.get())),
        static_cast<std::streamsize>(encoded_size)
    );
    if (!file) {
        throw std::runtime_error(
            "could not write output file: " + output.string()
        );
    }
}
