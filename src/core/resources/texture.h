#pragma once

#include "fs.h"

// FIXME: Images <> Textures <> Resources

struct Image {
    // FIXME: debug info
    const bgfx::Memory *pData = nullptr;
    /*
    u8 *pData = nullptr;
    i32 iWidth = 0;
    i32 iHeight = 0;
    i32 iPixelSize = 0;
    */
    char mpName[24];
};

typedef bgfx::TextureHandle Texture;
static Image *pMissingTexture = nullptr;

static Image *createImageInternal(const char *filename) {
    auto *img = new Image;
    //img->pData = stbi_load(filename, &img->iWidth, &img->iHeight, &img->iPixelSize, 0);
    img->pData = loadFile(filename);
    // FIXME: use texturec to compile to dds when loading
    // or implement a bgfx CS compressor based on 
    // https://github.com/walbourn/directx-sdk-samples/tree/master/BC6HBC7EncoderCS
    if (img->pData) {
        memset(img->mpName, 0, sizeof(img->mpName));
        memcpy(img->mpName, filename, std::min(sizeof(img->mpName) - 1, strlen(filename)));
    } else {
        fprintf(stdout, "Could not load material texture: %s.\n", filename);
        delete img;
        img = nullptr;
    }

    return img;
}

// FIXME: create a base default dds in a header file to use for streaming in (grey) and another for not-found (magenta-yellow-squares)
static Image *getMissingImage() {
    if (!pMissingTexture) {
        pMissingTexture = createImageInternal("cracked_c.dds");
    }
    return pMissingTexture;
}

static Image *createImage(const char *filename) {
    if (!strstr(filename, ".dds"))
    {
        auto search = std::string(filename);
        search.replace(search.size() - 3, 3, "dds");
        auto found = createImageInternal(search.c_str());
        return found ? found : getMissingImage();
    }
    auto img = createImageInternal(filename);
    return img ? img : getMissingImage();
}


static void destroyImage(Image *img) {
    assert(img && "Trying to destroy null image");

    if (img == pMissingTexture)
        return;

    //stbi_image_free(img->pData);
    memset(img, 0, sizeof(Image));
    delete img;
}

static void cleanupSystemImages() {
    if (pMissingTexture) {
        memset(pMissingTexture, 0, sizeof(Image));
        delete pMissingTexture;
    }
}