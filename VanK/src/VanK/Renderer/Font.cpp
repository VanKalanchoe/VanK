#include "Font.h"

#undef INFINITE
#include "Vendor/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"
#include "Vendor/msdf-atlas-gen/msdf-atlas-gen/FontGeometry.h"
#include "Vendor/msdf-atlas-gen/msdf-atlas-gen/GlyphGeometry.h"

#include "MSDFData.h"
#include "Renderer2D.h"

namespace VanK
{
    template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
    static Ref<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
        const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height)
    {
        msdf_atlas::GeneratorAttributes attributes;
        attributes.config.overlapSupport = true;
        attributes.scanlinePass = true;
        
        msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
        generator.setAttributes(attributes);
        generator.setThreadCount(8);
        generator.generate(glyphs.data(), (int)glyphs.size());

        msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

        TextureSpecification spec;
        spec.Width = bitmap.width;
        spec.Height = bitmap.height;
        spec.Format = ImageFormat::RGB8;
        spec.Data = (void*)bitmap.pixels; // he had a function SetData in texture class idk which better
        spec.GenerateMips = false;
        
        Ref<Texture2D> texture = Texture2D::Create(spec, Renderer2D::m_sampler);

        return texture;
    }
    
    Font::Font(const std::filesystem::path& filepath) : m_Data(new MSDFData())
    {
        msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
        VK_CORE_ASSERT(ft, "Freetype initialization failed!");
        
        if (ft) {
            std::string fileString = filepath.string();
            // todo msdfgen::loadFontData loads from memory buffer which we ll need
            msdfgen::FontHandle* font = msdfgen::loadFont(ft, fileString.c_str());
            if (!font)
            {
                VK_CORE_ASSERT("Failed to load font: {}", fileString);
                return;
            }

            struct CharsetRange
            {
                uint32_t Begin, End;
            };

            // From imgui_draw.cpp
            static const CharsetRange charsetRanges[] =
            {
                {0x0020, 0x00FF}, // Basic Latin + Latin Supplement
            };

            msdf_atlas::Charset charset;
            for (CharsetRange range : charsetRanges)
            {
                for (uint32_t c = range.Begin; c <= range.End; c++)
                    charset.add(c);
            }
            
            double fontScale = 1.0;
            m_Data->FontGeometry = msdf_atlas::FontGeometry(&m_Data->Glyphs);
            int glyphsLoaded = m_Data->FontGeometry.loadCharset(font, fontScale, charset);
            VK_CORE_INFO("Loaded {} glyphs from font: (out of {})", glyphsLoaded, charset.size());

            double emSize = 40.0;
            
            msdf_atlas::TightAtlasPacker atlasPacker;
            //atlasPacker.setDimensionsConstraint();
            atlasPacker.setPixelRange(2.0);
            atlasPacker.setMiterLimit(1.0);
            //atlasPacker.setPadding(0); not anymore need to look up
            atlasPacker.setScale(emSize);
            int remaining = atlasPacker.pack(m_Data->Glyphs.data(), (int)m_Data->Glyphs.size());
            VK_CORE_ASSERT(remaining == 0, "Failed to pack glyphs into atlas!");

            int width, height;
            atlasPacker.getDimensions(width, height);
            emSize = atlasPacker.getScale();

#define DEFAULT_ANGLE_THRESHOLD 3.0f
#define LCG_MULTIPLIER 6364136223846793005ULL
#define LCG_INCREMENT 1442695040888963407ULL
#define THREAD_COUNT 8
            // if MSDF || MTSDF

            uint64_t coloringSeed = 0;
            bool expensiveColoring = false;
            if (expensiveColoring)
            {
                msdf_atlas::Workload([&glyphs = m_Data->Glyphs, &coloringSeed](int i, int threadNo) -> bool
                {
                    unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
                    glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
                    return true;
                }, m_Data->Glyphs.size()).finish(THREAD_COUNT);
            } else
            {
                unsigned long long glyphSeed = coloringSeed;
                for (msdf_atlas::GlyphGeometry& glyph : m_Data->Glyphs)
                {
                    glyphSeed *= LCG_MULTIPLIER;
                    glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
                };
            }
            
            m_AtlasTexture = CreateAndCacheAtlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>("Test", (float)emSize, m_Data->Glyphs, m_Data->FontGeometry, width, height);
            
#if 0            
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
#endif            
            msdfgen::destroyFont(font);
            
            msdfgen::deinitializeFreetype(ft);
        }
    }

    Font::~Font()
    {
        delete m_Data;
    }

    Ref<Font> Font::GetDefault()
    {
        static Ref<Font> DefaultFont;
        if (!DefaultFont)
            DefaultFont = CreateRef<Font>("E:/dev/VanK/VanK-Editor/assets/Content/fonts/opensans/static/OpenSans-Regular.ttf");
        
        return DefaultFont;   
    }
}
