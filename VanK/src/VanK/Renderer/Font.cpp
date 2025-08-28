#include "Font.h"

#undef INFINITE
#include "Vendor/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"
#include "Vendor/msdf-atlas-gen/msdf-atlas-gen/FontGeometry.h"
#include "Vendor/msdf-atlas-gen/msdf-atlas-gen/GlyphGeometry.h"

namespace VanK
{
    Font::Font(const std::filesystem::path& filepath)
    {
        msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
        if (ft) {
            std::string fileString = filepath.string();
            msdfgen::FontHandle* font = msdfgen::loadFont(ft, fileString.c_str());
            if (font) {
                msdfgen::Shape shape;
                if (msdfgen::loadGlyph(shape, font, 'A', msdfgen::FONT_SCALING_EM_NORMALIZED)) {
                    shape.normalize();
                    //                      max. angle
                    msdfgen::edgeColoringSimple(shape, 3.0);
                    //          output width, height
                    msdfgen::Bitmap<float, 3> msdf(32, 32);
                    //                            scale, translation (in em's)
                    msdfgen::SDFTransformation t(msdfgen::Projection(32.0, msdfgen::Vector2(0.125, 0.125)), msdfgen::Range(0.125));
                    msdfgen::generateMSDF(msdf, shape, t);
                    msdfgen::savePng(msdf, "output.png");
                }
                msdfgen::destroyFont(font);
            }
            msdfgen::deinitializeFreetype(ft);
        }
    }
}
