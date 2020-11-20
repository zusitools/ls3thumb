#pragma once

#include "macros.hpp"

namespace {

struct DDS_PIXELFORMAT {
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwFourCC;
  uint32_t dwRGBBitCount;
  uint32_t dwRBitMask;
  uint32_t dwGBitMask;
  uint32_t dwBBitMask;
  uint32_t dwABitMask;
};

struct DDS_HEADER {
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwHeight;
  uint32_t dwWidth;
  uint32_t dwPitchOrLinearSize;
  uint32_t dwDepth;
  uint32_t dwMipMapCount;
  uint32_t dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  uint32_t dwCaps;
  uint32_t dwCaps2;
  uint32_t dwCaps3;
  uint32_t dwCaps4;
  uint32_t dwReserved2;
};

} // namespace

class Texture {
public:
  bool readDDS(const std::string &filename) {
#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

    DDS_HEADER header{};

    FILE *fp;

    // Try to open file
    fp = fopen(filename.c_str(), "rb");
    if (fp == NULL) {
      printf("ERROR::TEXTURE::FILE_NOT_FOUND::%s\n", filename.c_str());
      return false;
    }

    fseek(fp, 0, SEEK_END);
    unsigned int fileSize = ftell(fp);
    rewind(fp);

    // check magic
    char filecode[4];
    fread(&filecode, 1, 4, fp);
    if (strncmp(filecode, "DDS ", 4) != 0) {
      printf("ERROR::TEXTURE::NOT_VALID_DDS_FORMAT(\n");
      return false;
    }

    unsigned char *buffer;
    unsigned int bufSize;

    fread(&header, 1, 124, fp);
    // los file parametres
    assert(header.dwSize == 124);
    assert((header.ddspf.dwFlags & 0x4) == 0x4); // fourCC available

    bufSize = fileSize - 124 - 4;
    assert(bufSize != 0);

    buffer = new unsigned char[bufSize *
                               sizeof(unsigned char)]; // on allocate la ram
    fread(buffer, 1, bufSize, fp);                     // On lis le tout

    fclose(fp);

    // NOW we check what format that is... And make it compatible to OpenGL
    unsigned int format;
    switch (header.ddspf.dwFourCC) {
    case FOURCC_DXT1:
      format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      break;
    case FOURCC_DXT3:
      format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
      break;
    case FOURCC_DXT5:
      format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      break;
    default:
      delete[] buffer;
      printf("ERROR::TEXTURE::NOT_A_VALID_DXT_COMPRESSED_FILE\n");
      return false;
    }

    this->buffer = buffer;
    this->bufSize = bufSize;
    this->format = format;
    this->height = header.dwHeight;
    this->width = header.dwWidth;
    this->mipMapCount =
        std::max(1u, header.dwMipMapCount); // 0: non-mipmapped texture

    return true;
  }

  bool load_DDS(const std::string &ddsFile) {
    if (!readDDS(ddsFile)) {
      return false;
    }

    // Time  to load it in opengl
    TRY(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    // Fill  mipmaps
    unsigned int blockSize =
        (this->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    for (unsigned int level = 0;
         level < this->mipMapCount && (this->width || this->height); ++level) {
      unsigned int size =
          ((this->width + 3) / 4) * ((this->height + 3) / 4) * blockSize;
      TRY(glCompressedTexImage2D(GL_TEXTURE_2D, level, this->format,
                                 this->width, this->height, 0, size,
                                 this->buffer + offset));

      offset += size;
      this->width /= 2;
      this->height /= 2;
    }

    TRY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    TRY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    TRY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    TRY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        this->mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR
                                              : GL_LINEAR));

    TRY(glBindTexture(GL_TEXTURE_2D, 0));

    delete[] this->buffer;

    return true;
  }

private:
  unsigned int height;
  unsigned int width;
  unsigned int mipMapCount;
  unsigned int format;
  unsigned char *buffer;
  unsigned int bufSize;
};
