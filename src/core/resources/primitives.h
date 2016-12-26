#pragma once

typedef uint16_t Index;

struct Vertex {
    glm::vec3 vPos;
    glm::vec2 vUV;
    //u32 uiARGB;

    static void init() {
        scDecl
            .begin()
            .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float, true, true)
            .end();
    }

    static bgfx::VertexDecl scDecl;
};

bgfx::VertexDecl Vertex::scDecl;