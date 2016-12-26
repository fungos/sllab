#pragma once

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "model.h"

struct ModelLoader {

    ~ModelLoader() {
        if (scene) {
            unload();
        }

        shutdown();
    }

    void init() {
        stream  = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, nullptr);
        aiAttachLogStream(&stream);
    }

    void shutdown() {
        aiDetachAllLogStreams();
    }

    bool load(const char *path) {
        scene = aiImportFile(path, aiProcessPreset_TargetRealtime_MaxQuality);
        if (scene) {
            fprintf(stdout, "Loaded file '%s'.\n", path);

            releaseData();
            assert(prepareData2() == true && "Failed to load data");
            aiReleaseImport(scene); // dirty pointer as flag, fixme

            return true;
        }

        return false;
    }

    void unload() {
        releaseData();
        scene = nullptr;
    }

    void releaseData() {
        if (pModels) {
            for (auto i = u32{0}; i < uiModelCount; ++i) {
                auto *model = pModels + i;

                for (auto j = u32{0}; j < model->uiMeshCount; ++j) {
                    auto *mesh = model->pMeshes + j;
                    if (mesh->pVertexBuffer) {
                        free(mesh->pVertexBuffer);
                    }

                    if (mesh->pIndexBuffer) {
                        free(mesh->pIndexBuffer);
                    }

                    mesh->pVertexBuffer = nullptr;
                    mesh->pIndexBuffer = nullptr;
                    mesh->uiVertexCount = 0;
                    mesh->uiIndexCount = 0;
                }

                delete model->pMeshes;
                model->pMeshes = nullptr;
                model->uiMeshCount = 0;
            }

            delete pModels;
            pModels = nullptr;
            uiModelCount = 0;
        }

        if (pMaterials) {
            for (auto i = u32{0}; i < uiMaterialCount; ++i) {
                auto *mat = pMaterials + i;

                if (mat->pDiffuseMap) {
                    bgfx::destroyTexture(mat->hDiffuseMap);
                    destroyImage(mat->pDiffuseMap);
                }

                if (mat->pNormalMap) {
                    bgfx::destroyTexture(mat->hNormalMap);
                    destroyImage(mat->pNormalMap);
                }
                memset(mat, 0, sizeof(Material));
            }
            delete [] pMaterials;
            pMaterials = nullptr;
            uiMaterialCount = 0;
        }
    }

    u32 loadMaterials(aiMaterial **materials, Material *materialList, u32 len) {
        auto i = u32{0};
        for (; i < len; ++i) {
            auto *m = materialList + i;
            const auto *material = materials[i];

            auto str = aiString{};
            auto ret = material->Get(AI_MATKEY_NAME, str);
            if (ret == AI_SUCCESS) {
                fprintf(stdout, "Loading material: %s\n", str.C_Str());
            }

            auto model = int{0};
            ret = material->Get(AI_MATKEY_SHADING_MODEL, model);
            if (ret == AI_SUCCESS) {
                assert((model == aiShadingMode_Gouraud || model == aiShadingMode_Phong) && "Invalid shading model");
            }

            memset(m->mpName, 0, sizeof(m->mpName));
            memcpy(m->mpName, str.C_Str(), sizeof(m->mpName) - 1);

            auto dif = aiColor3D{ 0.f, 0.f, 0.f };
            auto amb = aiColor3D{ 0.f, 0.f, 0.f };
            auto spec = aiColor3D{ 0.f, 0.f, 0.f };
            auto shine = 0.0f;

            material->Get(AI_MATKEY_COLOR_AMBIENT, amb);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, dif);
            material->Get(AI_MATKEY_COLOR_SPECULAR, spec);
            material->Get(AI_MATKEY_SHININESS, shine);

            m->vAmbient   = glm::vec3{amb.r, amb.g, amb.b};
            m->vDiffuse   = glm::vec3{dif.r, dif.g, dif.b};
            m->vSpecular  = glm::vec3{spec.r, spec.g, spec.b};
            m->fShininess = shine;

            if (!material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), str)) {
                m->pDiffuseMap = createImage(str.C_Str());
                if (m->pDiffuseMap) {
                    m->hDiffuseMap = bgfx::createTexture(m->pDiffuseMap->pData);
                }
            }

            if (!material->Get(AI_MATKEY_TEXTURE_NORMALS(0), str)) {
                m->pNormalMap = createImage(str.C_Str());
                if (m->pNormalMap) {
                    m->hNormalMap = bgfx::createTexture(m->pNormalMap->pData);
                }
            }
            //m->Ambient *= .2;
            //if (m->fShininess == 0.0)
            //    m->fShininess = 30;
        }

        return i;
    }

    void loadMeshData(const aiMesh *meshInput, Vertex *vertices, u32 *vertexRead, Index *indices, u32 *indexRead, BoundingBox &bbox) {
        assert(meshInput);
        assert(vertices);
        assert(vertexRead);
        assert(indices);
        assert(indexRead);

        const auto indexOffset = int(*vertexRead);
        assert(indexOffset < std::numeric_limits<Index>::max() && "Index offset overflow");

        const auto numVertices = meshInput->mNumVertices;
        const auto numFaces = meshInput->mNumFaces;
        const auto numIndices = numFaces * 3;

        if (meshInput->HasTextureCoords(0)) {
            assert(meshInput->GetNumUVChannels() == 1 && "Only diffuse supported");
            assert(meshInput->mNumUVComponents[0] == 2 && "Only 2-component UV supported");

            const auto uvs = meshInput->mTextureCoords[0]; // diffuse
            for (auto v = u32{0}; v < numVertices; ++v) {
                vertices[v].vUV = glm::vec2{uvs[v].x, uvs[v].y};
            }
        }

        for (auto v = u32{0}; v < numVertices; ++v) {
            const auto pos = meshInput->mVertices[v];
            const auto vert = glm::vec3{pos[0], pos[1], pos[2]};
            vertices[v].vPos = vert;
            bbox.grow(vert);
        }

        for (auto f = u32{0}, idx = u32{ 0 }; f < numFaces; ++f, idx += 3) {
            const auto face = meshInput->mFaces[f];
            assert(face.mNumIndices == 3 && "Face must have 3 vertices");
            indices[idx + 0] = face.mIndices[0] + indexOffset;
            indices[idx + 1] = face.mIndices[1] + indexOffset;
            indices[idx + 2] = face.mIndices[2] + indexOffset;
        }
        *vertexRead = numVertices;
        *indexRead = numIndices;
    }

    Model *loadMergedModels(Model *model, aiNode **modelsInput, u32 len) {
        auto *mesh = model->pMeshes;
        auto *vertexPtr = mesh->pVertexBuffer;
        auto *indexPtr = mesh->pIndexBuffer;
        auto vertexCount = u32{0};
        auto indexCount = u32{0};

        for (auto i = u32{0}; i < len; ++i) {
            const auto *child = modelsInput[i];
            assert(child && "Invalid child");

            for (auto i = u32{0}; i < child->mNumMeshes; ++i) {
                const auto *amesh = scene->mMeshes[child->mMeshes[i]];

                // only support triangle primitives for now
                assert(amesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE && "Only trianlge primitives supported");

                BoundingBox bbox;
                loadMeshData(amesh, vertexPtr, &vertexCount, indexPtr, &indexCount, bbox);
                indexPtr += indexCount;
                vertexPtr += vertexCount;
                model->mBBox.grow(bbox);
            }
        }

        return model;
    }

    void processChildren(const aiNode *node, u32 *meshCount, u32 *vertexCount, u32 *indexCount) {
        const auto numModels = node->mNumChildren;
        for (auto i = u32{0}; i < numModels; ++i) {
            const auto *child = node->mChildren[i];
            assert(child && "Invalid child");

            const auto numMeshes = child->mNumMeshes;
            for (auto i = u32{0}; i < numMeshes; ++i) {
                const auto *amesh = scene->mMeshes[child->mMeshes[i]];

                // only support triangle primitives for now
                assert(amesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE && "Only triangle primitives supported");

                *indexCount += amesh->mNumFaces * 3;
                *vertexCount += amesh->mNumVertices;
            }

            *meshCount += numMeshes;
            processChildren(child, meshCount, vertexCount, indexCount);
        }
    }

    bool prepareData2() {
        assert(scene && "Scene not loaded");
        if (scene->HasMeshes()) {
            auto meshCount = u32{0};
            auto vertexCount = u32{0};
            auto indexCount = u32{0};

            const auto *node = scene->mRootNode;
            assert(node && "Could not find root node");

            processChildren(node, &meshCount, &vertexCount, &indexCount);
            assert(meshCount && "Must have at least one mesh");

            const auto vbSize = sizeof(Vertex) * vertexCount; // == Vertex::scDecl.getSize(vertexCount);
            const auto ibSize = sizeof(Index) * indexCount;

            auto *model = new Model;
            auto *mesh = new Mesh;
            mesh->pVertexBuffer = (Vertex *)malloc(vbSize);
            mesh->pIndexBuffer = (Index *)malloc(ibSize);
            mesh->uiVertexCount = vertexCount;
            mesh->uiIndexCount = indexCount;
            model->pMeshes = mesh;
            model->uiMeshCount = 1;
            memset(mesh->mpName, 0, sizeof(mesh->mpName));

            loadMergedModels(model, node->mChildren, node->mNumChildren);
            loadMaterials();

            pModels = model;
            uiModelCount = 1;

            return true;
        }

        return false;
    }

    void loadMaterials() {
        assert(scene && "Invalid scene");
        if (scene->HasMaterials()) {
            pMaterials = new Material[scene->mNumMaterials];
            uiMaterialCount = loadMaterials(&scene->mMaterials[0], &pMaterials[0], scene->mNumMaterials);
            assert(uiMaterialCount == scene->mNumMaterials && "Failed to load all materials");

            fprintf(stdout, "Materials:\n");
            for (auto i = u32{0}; i < uiMaterialCount; ++i) {
                const auto *mat = &pMaterials[i];
                fprintf(stdout, " %d: %s\n", i, mat->mpName);

                if (mat->pDiffuseMap) {
                    fprintf(stdout, "  diffuse: %s\n", mat->pDiffuseMap->mpName);
                }
                if (mat->pNormalMap) {
                    fprintf(stdout, "  normal: %s\n", mat->pNormalMap->mpName);
                }
            }
        }
    }

#if 0 // FIXME: why these functions? can't remember the reason...
    u32 loadMeshes(const aiNode *node, aiMesh **meshesInput, Mesh *meshListOut, u32 len, BoundingBox &bbox) {
        auto i = u32{0};
        for (; i < len; ++i) {
            auto *mesh = meshListOut + i;
            const auto *amesh = meshesInput[node->mMeshes[i]];

            // only support triangle primitives for now
            assert(amesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE && "Primitives must be triangles");

            const auto faceCount = amesh->mNumFaces;
            const auto indexCount = faceCount * 3; // only triangles
            const auto vertexCount = amesh->mNumVertices;
            const auto vbSize = sizeof(Vertex) * vertexCount; // == Vertex::scDecl.getSize(vertexCount);
            const auto ibSize = sizeof(Index) * indexCount;
            mesh->pVertexBuffer = (Vertex *)malloc(vbSize);
            mesh->pIndexBuffer = (Index *)malloc(ibSize);
            mesh->uiVertexCount = vertexCount;
            mesh->uiIndexCount = indexCount;

            memset(mesh->mpName, 0, sizeof(mesh->mpName));
            memcpy(mesh->mpName, node->mName.C_Str(), sizeof(mesh->mpName) - 1);

            if (amesh->HasTextureCoords(0)) {
                assert(amesh->GetNumUVChannels() == 1 && "Only diffuse supported");
                assert(amesh->mNumUVComponents[0] == 2 && "Only 2-component UV supported");

                const auto uvs = amesh->mTextureCoords[0]; // diffuse
                for (auto v = u32{0}; v < vertexCount; ++v) {
                    //fprintf(stdout, "UV %d: %f, %f\n", v, uvs[v].x, uvs[v].y);
                    mesh->pVertexBuffer[v].vUV = glm::vec2{uvs[v].x, uvs[v].y};
                }
            }

            for (auto v = u32{0}; v < vertexCount; ++v) {
                const auto pos = amesh->mVertices[v];
                const auto vert = glm::vec3{pos[0], pos[1], pos[2]};
                mesh->pVertexBuffer[v].vPos = vert;
                bbox.grow(vert);
                //mesh->pVertexBuffer[v + vertexPos].uiARGB = 0xffffffff; //((v % 255) + 0x000000ff) | ((v) << 24); //s_cubeVertices[v].uiARGB;
                //const auto nor = amesh->mNormals[v];
            }

            for (auto f = u32{0}, idx = u32{0}; f < faceCount; ++f, idx += 3) {
                const auto face = amesh->mFaces[f];
                assert(face.mNumIndices == 3);
                mesh->pIndexBuffer[idx + 0] = face.mIndices[0];
                mesh->pIndexBuffer[idx + 1] = face.mIndices[1];
                mesh->pIndexBuffer[idx + 2] = face.mIndices[2];
                //memcpy(pIndexBuffer + idx, face.mIndices, face.mNumIndices * sizeof(Index));
            }
        }

        return i;
    }

    u32 loadModels(const aiNode *node, Model *models, u32 len) {
        fprintf(stdout, "Models: %d\n", len);
        auto i = u32{0};
        for (; i < len; ++i) {
            const auto *child = node->mChildren[i];
            assert(child && "Could not find any child");

            auto *m = models + i;
            const auto meshCount = child->mNumMeshes;

            m->pMeshes = new Mesh[meshCount];
            m->uiMeshCount = loadMeshes(child, scene->mMeshes, m->pMeshes, meshCount, m->mBBox);
            assert(meshCount == m->uiMeshCount && "Mesh count mismatch");
        }

        return i;
    }

    bool prepareData() {
        if (scene->HasMeshes()) {
            const auto *node = scene->mRootNode;
            assert(node && "Invalid node");

            const auto objCount = node->mNumChildren;
            pModels = new Model[objCount];
            uiModelCount = loadModels(node, pModels, objCount);
            assert(uiModelCount == objCount && "Could not load all models");

            loadMaterials();

            return true;
        }

        return false;
    }
#endif

    void dumpModel(Model *model, u32 verbosity) {
        auto bb = model->mBBox;
        fprintf(stdout, " Bounding Box: (%+2.4f, %+2.4f, %+2.4f) (%+2.4f, %+2.4f, %+2.4f)\n", bb.min.x, bb.min.y, bb.min.z, bb.max.x, bb.max.y, bb.max.z);
        fprintf(stdout, "Meshes:\n");
        for (auto i = u32{0}; i < model->uiMeshCount; ++i) {
            const auto *mesh = model->pMeshes + i;
            fprintf(stdout, " %d: %s\n", i, mesh->mpName);

            fprintf(stdout, " Vertices: %d\n", mesh->uiVertexCount);
            if (verbosity == 1) {
                for (auto v = u32{0}; v < mesh->uiVertexCount; ++v) {
                    const auto vtx = &mesh->pVertexBuffer[v];
                    fprintf(stdout, "\t%3d Pos(%+2.4f, %+2.4f, %+2.4f) UV(%+1.2f,%+1.2f)\n", v, vtx->vPos.x, vtx->vPos.y, vtx->vPos.z, vtx->vUV.x, vtx->vUV.y);
                }
            }

            fprintf(stdout, " Indices: %d\n\t", mesh->uiIndexCount);
            if (verbosity == 2) {
                for (auto v = u32{0}; v < mesh->uiIndexCount; ++v) {
                    fprintf(stdout, " %d,", mesh->pIndexBuffer[v]);
                }
            }

            if (verbosity == 3) {
                for (auto idx = u32{0}; idx < mesh->uiIndexCount / 3; idx += 3) {
                    const auto v1 = mesh->pIndexBuffer[idx + 0];
                    const auto v2 = mesh->pIndexBuffer[idx + 1];
                    const auto v3 = mesh->pIndexBuffer[idx + 2];
                    const auto vtx1 = &mesh->pVertexBuffer[v1];
                    const auto vtx2 = &mesh->pVertexBuffer[v2];
                    const auto vtx3 = &mesh->pVertexBuffer[v3];
                    fprintf(stdout, "\t%3d(%3d) Pos(%+2.4f, %+2.4f, %+2.4f) UV(%+1.2f,%+1.2f)\n", idx + 0, v1, vtx1->vPos.x, vtx1->vPos.y, vtx1->vPos.z, vtx1->vUV.x, vtx1->vUV.y);
                    fprintf(stdout, "\t%3d(%3d) Pos(%+2.4f, %+2.4f, %+2.4f) UV(%+1.2f,%+1.2f)\n", idx + 1, v2, vtx2->vPos.x, vtx2->vPos.y, vtx2->vPos.z, vtx2->vUV.x, vtx2->vUV.y);
                    fprintf(stdout, "\t%3d(%3d) Pos(%+2.4f, %+2.4f, %+2.4f) UV(%+1.2f,%+1.2f)\n", idx + 2, v3, vtx3->vPos.x, vtx3->vPos.y, vtx3->vPos.z, vtx3->vUV.x, vtx3->vUV.y);
                }
            }
        }
        fflush(stdout);
    }

    Model *pModels = nullptr;
    Material *pMaterials = nullptr;

    u32 uiModelCount = 0;
    u32 uiMaterialCount = 0;

    // temporary work, fixme
    const struct aiScene *scene = nullptr;
    struct aiLogStream stream;
};
