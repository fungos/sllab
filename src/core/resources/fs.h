#pragma once

const bgfx::Memory *loadFile(const char *path) {
    auto reader = entry::getFileReader();
    if (bx::open(reader, path))
    {
        uint32_t size = (uint32_t)bx::getSize(reader);
        const bgfx::Memory* mem = bgfx::alloc(size + 1);
        bx::read(reader, mem->data, size);
        bx::close(reader);
        mem->data[mem->size - 1] = '\0';
        return mem;
    }
    return nullptr;
}
