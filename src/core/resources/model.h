#pragma once

#include "texture.h"
#include "../math/bbox.h"

struct Material {
    // should we have these?
    glm::vec3 vAmbient;
    glm::vec3 vDiffuse;
    glm::vec3 vSpecular;
    float fShininess = 0.f; // use this ???

    // load texture resources here
    Texture hDiffuseMap;
    Texture hNormalMap;
    Texture hRoughMap;

    // load image resources somewhere not here
    Image *pDiffuseMap = nullptr;
    Image *pNormalMap = nullptr;
    Image *pRoughness = nullptr;

    // debug only
    char mpName[24];
};

struct Mesh {
    Vertex *pVertexBuffer = nullptr;
    Index *pIndexBuffer = nullptr;
    u32 uiVertexCount = 0;
    u32 uiIndexCount = 0;

    // debug only
    char mpName[24];
};

struct Model {
    Mesh *pMeshes;
    u32 uiMeshCount;

    BoundingBox mBBox;
};