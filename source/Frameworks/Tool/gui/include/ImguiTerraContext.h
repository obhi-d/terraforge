#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Timeline.h>
#include <Magnum/Math/Vector.h>
#include <Magnum/Math/Vector2.h>

#include <string>
#include <memory>
#include <unordered_map>

#include "Common.h"

namespace terra
{
    class ImguiTerraWindow;
    class ImguiTerraContext
    {
    public:
        
      std::shared_ptr<ImguiTerraWindow> createWindow(std::string name, Magnum::Vector2i size, Magnum::Vector2i pos);

      bool pollEvents();

    private:
      // 0th is the main window
        std::unordered_map<uint32, uint32>             windowMap;
        std::vector<std::shared_ptr<ImguiTerraWindow>> windows;
    };
} // namespace terra
