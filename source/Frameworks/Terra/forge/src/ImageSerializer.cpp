
#include "Logger.h"
#include "ImageSerializer.h"
#include <png.h>

namespace terra
{

void handleErrPNG(png_structp, png_const_charp txt) 
{
  logError("PNG error: {}", txt ? txt : "");
}

void writePNG(png_structp str, png_bytep data, size_t size) 
{
  std::ofstream& ss = *(std::ofstream*) png_get_io_ptr(str);
  ss.write((const char*)data, size);
}

void flushPNG(png_structp str) 
{
  std::ofstream& ss = *(std::ofstream*)png_get_io_ptr(str);
  ss.flush();
}

void readPNG(png_structp str, png_bytep data, size_t size)
{
  std::ifstream& ss = *(std::ifstream*)png_get_io_ptr(str);
  ss.read((char*)data, size);
}

void ImageSerializer::saveImage(ImageData const& image, std::filesystem::path path) 
{
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    return;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    return;
  }

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return;
  }

  std::ofstream file(path, std::ios::binary);
  if (!file.is_open())
  {
    logError("Failed to open file : {}", path.string());
    return;
  }
  png_set_write_fn(png_ptr, &file, writePNG, flushPNG);
  int color = PNG_COLOR_TYPE_GRAY;
  int bitd  = 16;
  uint pix   = 2;
  if (image.format == ImageFormatEnum::eRgba8 || image.format == ImageFormatEnum::eSrgb8Alpha8)
  {
    color = PNG_COLOR_TYPE_RGBA;
    bitd  = 8;
    pix   = 4;
  }
  png_set_IHDR(png_ptr, info_ptr, (uint32_t)image.width, (uint32_t)image.height, bitd,
               color, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);
  png_set_swap(png_ptr);
  auto row_pointers = std::vector<png_bytep>(image.height);
  auto start        = image.data.get();
  for (uint i = 0; i <= image.height; ++i)
  {
    row_pointers[i] = (png_bytep)start;
    start += (image.width) * pix;
  }
  png_write_image(png_ptr, row_pointers.data());
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

bool ImageSerializer::loadImage(ImageData& data, std::filesystem::path path)
{
  int           bitDepth        = 0;
  int           colorType       = 0;
  int           interlaceType   = 0;
  int           compressionType = 0;
  int           filterMethod    = 0;
  size_t          rowbytes        = 0; 
  char            channels        = 0;
  auto            row_pointers    = std::vector<png_bytep>();
  bool            ok              = true;
  std::ifstream file(path, std::ios::binary);
  if (!file)
  {
    logError("Failed to open file : {}", path.string());
    return false;
  }
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    return false;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    goto error;
  }

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    goto error;
  }

  png_set_read_fn(png_ptr, &file, readPNG);
  png_read_info(png_ptr, info_ptr);
  
  png_get_IHDR(png_ptr, info_ptr, &data.width, &data.height, &bitDepth, &colorType, &interlaceType, &compressionType,
               &filterMethod);
  if (colorType == PNG_COLOR_TYPE_GRAY)
  {
    if (bitDepth == 8)
      data.format = ImageFormatEnum::eUnorm8;
    else if (bitDepth == 16)
    {
      png_set_swap(png_ptr);
      data.format = ImageFormatEnum::eSnorm16;
    }
    else
    {
      logError("PNG bit depth not supported! : {}", bitDepth);
      goto error;
    }
  }
  else
  {

    if (colorType == PNG_COLOR_TYPE_RGBA)
    {
      if (bitDepth == 8)
        data.format = ImageFormatEnum::eRgba8;
      else
      {
        logError("PNG bit depth not supported for color texture! : {}", bitDepth);
        goto error;
      }
    }
    else if (colorType == PNG_COLOR_TYPE_RGB)
    {
      png_set_add_alpha(png_ptr, 255, PNG_FILLER_AFTER);
      png_read_update_info(png_ptr, info_ptr);
      colorType = png_get_color_type(png_ptr, info_ptr);
    }
    else
    {
      logError("Color format not supported.");
      goto error;
    }
    if (bitDepth == 8)
      data.format = ImageFormatEnum::eRgba8;
    else
    {
      logError("PNG bit depth not supported for color texture! : {}", bitDepth);
      goto error;
    }
  }

  rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  channels = png_get_channels(png_ptr, info_ptr);
  data.data     = std::make_unique<ubyte_t[]>(rowbytes * data.height);
  if (!data.data.get())
    goto error;
  row_pointers.resize(data.height);
  for (uint32_t i = 0; i < data.height; ++i)
    row_pointers[i] = (png_bytep)(data.data.get() + i * rowbytes);
  png_read_image(png_ptr, row_pointers.data());
  png_read_end(png_ptr, NULL);
  goto done;
error:
  ok = false;
done:
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
  return ok;
}

void ImageSerializer::loadImageRgba(std::span<ubyte_t*> rows, uint32_t xwidth, uint32_t xheight,
                                    std::filesystem::path path)
{
  int           bitDepth        = 0;
  int           colorType       = 0;
  int           interlaceType   = 0;
  int           compressionType = 0;
  int           filterMethod    = 0;
  uint32_t      width           = 0;
  uint32_t      height          = 0;
  size_t        rowbytes        = 0;
  char          channels        = 0;
  auto          row_pointers    = std::vector<png_bytep>();
  std::ifstream file(path, std::ios::binary);
  if (!file)
  {
    logError("Failed to open file : {}", path.string());
    return;
  }
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    return;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    return;
  }

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    return;
  }

  png_set_read_fn(png_ptr, &file, readPNG);
  png_read_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth, &colorType, &interlaceType, &compressionType,
               &filterMethod);
  if (width != xwidth || height != xheight)
  {
    logError("Cannot read this file of different dimensions");
    return;
  }
  if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
  {
    png_set_gray_to_rgb(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    colorType = png_get_color_type(png_ptr, info_ptr);
  }
  if (colorType == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_palette_to_rgb(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    colorType = png_get_color_type(png_ptr, info_ptr);
  }
  if (colorType == PNG_COLOR_TYPE_RGB)
  {
    png_set_add_alpha(png_ptr, 255, PNG_FILLER_AFTER);
    png_read_update_info(png_ptr, info_ptr);
    colorType = png_get_color_type(png_ptr, info_ptr);
  }
  if (colorType != PNG_COLOR_TYPE_RGBA)
  {
    logError("PNG format not supported! : {}", colorType);
    return;
  }
  if (bitDepth != 8)
  {
    if (bitDepth == 16)
    {
      png_set_strip_16(png_ptr);
    }
  }
  
  rowbytes  = png_get_rowbytes(png_ptr, info_ptr);
  assert(rowbytes == width * 4);
  channels  = png_get_channels(png_ptr, info_ptr);
  assert(channels == 4);
  png_read_image(png_ptr, (png_bytepp)rows.data());
  png_read_end(png_ptr, NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
  return;
}

void ImageSerializer::loadImageGray(std::span<ubyte_t*> rows, uint32_t xwidth, uint32_t xheight,
                                    std::filesystem::path path)
{
  int           bitDepth        = 0;
  int           colorType       = 0;
  int           interlaceType   = 0;
  int           compressionType = 0;
  int           filterMethod    = 0;
  uint32_t      width           = 0;
  uint32_t      height          = 0;
  size_t        rowbytes        = 0;
  char          channels        = 0;
  auto          row_pointers    = std::vector<png_bytep>();
  std::ifstream file(path, std::ios::binary);
  if (!file)
  {
    logError("Failed to open file : {}", path.string());
    return;
  }
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    return;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    return;
  }

 
  png_set_read_fn(png_ptr, &file, readPNG);
  png_read_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth, &colorType, &interlaceType, &compressionType,
               &filterMethod);
  if (width != xwidth || height != xheight)
  {
    logError("Cannot read this file of different dimensions");
    return;
  }
  
  png_set_alpha_mode(png_ptr, PNG_ALPHA_PREMULTIPLIED, PNG_GAMMA_LINEAR);
  png_set_strip_alpha(png_ptr);

  if (colorType == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_palette_to_rgb(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    colorType = png_get_color_type(png_ptr, info_ptr);
  }
  if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGBA)
  {
    png_set_rgb_to_gray(png_ptr, 1, 0.0f, 0.0f);
    png_read_update_info(png_ptr, info_ptr);
    colorType = png_get_color_type(png_ptr, info_ptr);
  }
  if (colorType != PNG_COLOR_TYPE_GRAY)
  {
    logError("PNG format not supported! : {}", colorType);
    return;
  }

  if (bitDepth != 8)
  {
    if (bitDepth == 16)
    {
      png_set_strip_16(png_ptr);
    }
  }

  rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  assert(rowbytes == width);
  channels = png_get_channels(png_ptr, info_ptr);
  assert(channels == 1);
  png_read_image(png_ptr, (png_bytepp)rows.data());
  png_read_end(png_ptr, NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
  return;
}

}