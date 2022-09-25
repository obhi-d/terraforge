#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Timeline.h>

#include <memory>
#include <unordered_map>

#include "../Common.h"

namespace terra
{
    class ImguiTerraWindow;
    class ImguiTerraContext
    {
    public:
        bool pollEvents();

    private:
        std::unordered_map<uint32, uint32>             windowMap;
        std::vector<std::shared_ptr<ImguiTerraWindow>> windows;
    };
} // namespace terra
