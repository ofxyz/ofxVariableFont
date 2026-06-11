#pragma once

//
// VarFontFace
// -----------
// openFrameworks wrapper around ImVarFont for variable-font outline extraction.
//
// Loads a .ttf/.otf file, exposes design-variation axes, HarfBuzz/GPOS kerning,
// line height / letter spacing, and extracts glyph outlines as ofPath objects
// Layout/em size uses your coordinate units (pixels, mm, etc.) consistently.
//

#include "ofMain.h"
#include "imgui_var_font.h"
#include <string>
#include <vector>

namespace varfont {

struct Axis {
    uint32_t    tag      = 0;
    std::string name;
    float       minValue = 0.f;
    float       maxValue = 1.f;
    float       defValue = 0.f;
    float       value    = 0.f;

    std::string tagString() const {
        char s[5] = { char((tag >> 24) & 0xFF),
                      char((tag >> 16) & 0xFF),
                      char((tag >>  8) & 0xFF),
                      char( tag        & 0xFF), 0 };
        return s;
    }
};

struct StringLayoutOptions {
    float lineHeightMult  = 1.f;   ///< 1 = font default line height
    float letterSpacingEm = 0.f;   ///< extra spacing as fraction of em
    bool  allowExtrapolation = false;
};

class VarFontFace {
public:
    VarFontFace();
    ~VarFontFace();

    VarFontFace(const VarFontFace&)            = delete;
    VarFontFace& operator=(const VarFontFace&) = delete;
    VarFontFace(VarFontFace&&) noexcept;
    VarFontFace& operator=(VarFontFace&&) noexcept;

    bool load(const of::filesystem::path& path);

    void unload();

    bool isLoaded()   const { return m_face != nullptr; }

    bool isVariable() const { return m_face && ImVarFont::IsVariable(m_face); }

    const of::filesystem::path& fontPath()   const { return m_path; }
    const std::string&          familyName() const { return m_familyName; }
    const std::string&          styleName()  const { return m_styleName; }

    ImVarFont::Face* imVarFace()       { return m_face; }

    const ImVarFont::Face* imVarFace() const { return m_face; }
    const std::vector<Axis>& axes() const { return m_axes; }

    std::vector<Axis>&       axes()       { return m_axes; }

    void resetAxes();
    void applyAxes(bool allow_extrapolation = false);

    /// Copy current axis values from the loaded ImVarFont face into axes().
    void pullAxesFromImVar();

    std::vector<ofPath> getStringPaths(const std::string& utf8,
                                       float x, float y,
                                       float emMM,
                                       const StringLayoutOptions& opts = {}) const;

    ofPath getGlyphPath(uint32_t codepoint, float penX, float penY, float emMM) const;
    ofPath getGlyphPathByIndex(uint32_t glyphIndex, float penX, float penY, float emMM) const;

    float getGlyphAdvanceMM(uint32_t codepoint, float emMM) const;
    float getKerningMM(uint32_t left, uint32_t right, float emMM) const;

    bool hasKerning() const;
    bool hasKernTable() const;
    bool hasGpos() const;
    bool usesHarfBuzz() const;
    bool useKerning() const;

    void setUseKerning(bool enabled);

    bool useHarfBuzz() const;
    void setUseHarfBuzz(bool enabled);

    bool useKernTable() const;

    void setUseKernTable(bool enabled);

    const char* kerningEngineLabel() const;

    ofRectangle getStringBoundsMM(const std::string& utf8, float emMM,
                                  const StringLayoutOptions& opts = {}) const;
    
    void drawString(const std::string& utf8, float x, float y, float emSize,
                    const StringLayoutOptions& opts = {}) const;

private:
    of::filesystem::path m_path;
    std::string          m_familyName;
    std::string          m_styleName;

    ImVarFont::Face*     m_face = nullptr;
    std::vector<Axis>    m_axes;

    bool                 m_allowExtrapolation = false;

    void syncAxesFromImVar();
    void syncAxesToImVar();

    float designToMM(float emMM) const;

};

} // namespace varfont
