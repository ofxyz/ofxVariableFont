#include "VarFontFace.h"

#include "ofMain.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "ofLog.h"
#include <algorithm>
#include <cstdio>

namespace varfont {

static inline FT_Face ftFace(ImVarFont::Face* face) {
    return static_cast<FT_Face>(ImVarFont::GetFtFace(face));
}

struct OutlineCtx {
    ofPath*   path   = nullptr;
    float     scaleX = 1.f;
    float     scaleY = 1.f;
    float     penX   = 0.f;
    float     penY   = 0.f;
    glm::vec3 cur    = {};

    glm::vec3 toMM(const FT_Vector& v) const {
        return { penX + v.x * scaleX,
                 penY - v.y * scaleY,
                 0.f };
    }
};

static int cb_moveTo(const FT_Vector* to, void* user) {
    auto& c = *static_cast<OutlineCtx*>(user);
    c.cur = c.toMM(*to);
    c.path->moveTo(c.cur);
    return 0;
}

static int cb_lineTo(const FT_Vector* to, void* user) {
    auto& c = *static_cast<OutlineCtx*>(user);
    c.cur = c.toMM(*to);
    c.path->lineTo(c.cur);
    return 0;
}

static int cb_conicTo(const FT_Vector* ctrl, const FT_Vector* to, void* user) {
    auto& c = *static_cast<OutlineCtx*>(user);
    const glm::vec3 p1  = c.toMM(*ctrl);
    const glm::vec3 p2  = c.toMM(*to);
    const glm::vec3 cp1 = c.cur + (2.f / 3.f) * (p1 - c.cur);
    const glm::vec3 cp2 = p2    + (2.f / 3.f) * (p1 - p2);
    c.path->bezierTo(cp1, cp2, p2);
    c.cur = p2;
    return 0;
}

static int cb_cubicTo(const FT_Vector* c1, const FT_Vector* c2,
                      const FT_Vector* to, void* user) {
    auto& c = *static_cast<OutlineCtx*>(user);
    const glm::vec3 p2 = c.toMM(*to);
    c.path->bezierTo(c.toMM(*c1), c.toMM(*c2), p2);
    c.cur = p2;
    return 0;
}

static const FT_Outline_Funcs kOutlineFuncs = {
    cb_moveTo, cb_lineTo, cb_conicTo, cb_cubicTo,
    0, 0
};

VarFontFace::VarFontFace() = default;

VarFontFace::~VarFontFace() { unload(); }

VarFontFace::VarFontFace(VarFontFace&& o) noexcept
    : m_path(std::move(o.m_path))
    , m_familyName(std::move(o.m_familyName))
    , m_styleName(std::move(o.m_styleName))
    , m_face(o.m_face)
    , m_axes(std::move(o.m_axes))
    , m_allowExtrapolation(o.m_allowExtrapolation)
{
    o.m_face = nullptr;
}

VarFontFace& VarFontFace::operator=(VarFontFace&& o) noexcept {
    if (this != &o) {
        unload();
        m_path               = std::move(o.m_path);
        m_familyName         = std::move(o.m_familyName);
        m_styleName          = std::move(o.m_styleName);
        m_face               = o.m_face;
        m_axes               = std::move(o.m_axes);
        m_allowExtrapolation = o.m_allowExtrapolation;
        o.m_face             = nullptr;
    }
    return *this;
}

static std::string nativeFontPath(const of::filesystem::path& path) {
    if (path.is_absolute()) {
        std::string pathStr = path.string();
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
        return pathStr;
    }
    return ofPathToString(ofToDataPathFS(path));
}

bool VarFontFace::load(const of::filesystem::path& path) {
    unload();

    const std::string pathStr = nativeFontPath(path);
    if (FILE* f = fopen(pathStr.c_str(), "rb")) {
        fclose(f);
    } else {
        ofLogError("VarFontFace") << "File not found: " << pathStr;
        return false;
    }

    char err[512] = {};
    m_face = ImVarFont::LoadFace(pathStr.c_str(), err, sizeof(err));
    if (!m_face) {
        ofLogError("VarFontFace") << (err[0] ? err : "LoadFace failed") << " — " << pathStr;
        return false;
    }

    m_path       = path;
    m_familyName = ImVarFont::GetFamilyName(m_face);
    m_styleName  = ImVarFont::GetStyleName(m_face);
    syncAxesFromImVar();

    ofLogNotice("VarFontFace") << "Loaded " << m_familyName << " " << m_styleName
                               << (isVariable() ? " (variable)" : " (static)")
                               << " — axes: " << m_axes.size()
                               << ", kerning: " << ImVarFont::GetKerningEngineLabel(m_face);
    return true;
}

void VarFontFace::unload() {
    ImVarFont::FreeFace(m_face);
    m_face = nullptr;
    m_axes.clear();
    m_familyName.clear();
    m_styleName.clear();
    m_path.clear();
}

void VarFontFace::syncAxesFromImVar() {
    m_axes.clear();
    if (!m_face) return;

    ImVarFont::Axis* src = ImVarFont::GetAxes(m_face);
    const int n = ImVarFont::GetAxisCount(m_face);
    if (!src || n <= 0) return;

    m_axes.resize(n);
    for (int i = 0; i < n; ++i) {
        m_axes[i].tag      = src[i].Tag;
        m_axes[i].name     = src[i].Name;
        m_axes[i].maxValue = src[i].Max;
        m_axes[i].minValue = src[i].Min;
        m_axes[i].defValue = src[i].Default;
        m_axes[i].value    = src[i].Value;
    }
}

void VarFontFace::syncAxesToImVar() {
    if (!m_face) return;
    ImVarFont::Axis* dst = ImVarFont::GetAxes(m_face);
    if (!dst) return;
    for (size_t i = 0; i < m_axes.size(); ++i)
        dst[i].Value = m_axes[i].value;
}

void VarFontFace::resetAxes() {
    if (!m_face) return;
    ImVarFont::ResetAxes(m_face);
    syncAxesFromImVar();
}

void VarFontFace::applyAxes(bool allow_extrapolation) {
    if (!m_face) return;
    m_allowExtrapolation = allow_extrapolation;
    syncAxesToImVar();
    ImVarFont::ApplyAxes(m_face, m_allowExtrapolation);
}

void VarFontFace::pullAxesFromImVar() {
    syncAxesFromImVar();
}

float VarFontFace::designToMM(float emMM) const {
    FT_Face face = ftFace(m_face);
    if (!face || face->units_per_EM == 0) return 1.f;
    return emMM / static_cast<float>(face->units_per_EM);
}

ofPath VarFontFace::getGlyphPathByIndex(uint32_t glyphIndex,
                                        float penX, float penY,
                                        float emMM) const
{
    ofPath result;
    result.setFilled(true);
    result.setUseShapeColor(false);
    result.setPolyWindingMode(OF_POLY_WINDING_ODD);

    FT_Face face = ftFace(m_face);
    if (!face || glyphIndex == 0) return result;

    if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING) != 0)
        return result;
    if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
        return result;

    const float scale = designToMM(emMM);
    OutlineCtx ctx;
    ctx.path   = &result;
    ctx.scaleX = scale;
    ctx.scaleY = scale;
    ctx.penX   = penX;
    ctx.penY   = penY;

    FT_Outline_Decompose(&face->glyph->outline, &kOutlineFuncs, &ctx);
    result.close();
    return result;
}

ofPath VarFontFace::getGlyphPath(uint32_t codepoint,
                                 float penX, float penY,
                                 float emMM) const
{
    FT_Face face = ftFace(m_face);
    if (!face) return {};
    const FT_UInt gi = FT_Get_Char_Index(face, codepoint);
    return getGlyphPathByIndex(gi, penX, penY, emMM);
}

float VarFontFace::getGlyphAdvanceMM(uint32_t codepoint, float emMM) const {
    FT_Face face = ftFace(m_face);
    if (!face) return 0.f;
    const FT_UInt gi = FT_Get_Char_Index(face, codepoint);
    if (gi == 0) return 0.f;
    if (FT_Load_Glyph(face, gi, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING) != 0)
        return 0.f;
    return static_cast<float>(face->glyph->advance.x) * designToMM(emMM);
}

bool VarFontFace::hasKerning() const {
    return m_face && ImVarFont::HasKerning(m_face);
}

bool VarFontFace::hasKernTable() const {
    return m_face && ImVarFont::HasKernTable(m_face);
}

bool VarFontFace::hasGpos() const {
    return m_face && ImVarFont::HasGpos(m_face);
}

bool VarFontFace::usesHarfBuzz() const {
    return m_face && ImVarFont::UsesHarfBuzz(m_face);
}

bool VarFontFace::useKerning() const {
    return m_face && ImVarFont::GetUseKerning(m_face);
}

void VarFontFace::setUseKerning(bool enabled) {
    if (m_face) ImVarFont::SetUseKerning(m_face, enabled);
}

bool VarFontFace::useHarfBuzz() const {
    return m_face && ImVarFont::GetUseHarfBuzz(m_face);
}

void VarFontFace::setUseHarfBuzz(bool enabled) {
    if (m_face) ImVarFont::SetUseHarfBuzz(m_face, enabled);
}

bool VarFontFace::useKernTable() const {
    return m_face && ImVarFont::GetUseKernTable(m_face);
}

void VarFontFace::setUseKernTable(bool enabled) {
    if (m_face) ImVarFont::SetUseKernTable(m_face, enabled);
}

const char* VarFontFace::kerningEngineLabel() const {
    return m_face ? ImVarFont::GetKerningEngineLabel(m_face) : "none";
}

float VarFontFace::getKerningMM(uint32_t left, uint32_t right, float emMM) const {
    if (!m_face) return 0.f;
    return ImVarFont::GetKernTablePairPx(m_face, left, right, emMM);
}

std::vector<ofPath> VarFontFace::getStringPaths(const std::string& utf8,
                                                float x, float y,
                                                float emMM,
                                                const StringLayoutOptions& opts) const
{
    std::vector<ofPath> result;
    if (!m_face || utf8.empty()) return result;

    const float defaultLineH = ImVarFont::CalcLineHeightPx(m_face, emMM);
    const float lineH = defaultLineH * opts.lineHeightMult;
    const float letterSp = opts.letterSpacingEm * emMM;
    const float asc = ImVarFont::CalcAscenderPx(m_face, emMM);

    std::vector<ImVarFont::PlacedGlyph> placed;
    ImVarFont::LayoutGlyphs(m_face, utf8.c_str(), emMM, lineH, letterSp, placed);

    result.reserve(placed.size());
    for (const auto& g : placed) {
        if (g.glyph_index == 0) continue;
        ofPath p = getGlyphPathByIndex(g.glyph_index, x + g.x, y + g.y - asc, emMM);
        result.push_back(std::move(p));
    }
    return result;
}

ofRectangle VarFontFace::getStringBoundsMM(const std::string& utf8, float emMM,
                                           const StringLayoutOptions& opts) const
{
    if (!m_face || utf8.empty()) return {};

    const float defaultLineH = ImVarFont::CalcLineHeightPx(m_face, emMM);
    const float lineH = defaultLineH * opts.lineHeightMult;
    const float letterSp = opts.letterSpacingEm * emMM;
    const float asc = ImVarFont::CalcAscenderPx(m_face, emMM);
    const float desc = ImVarFont::CalcDescenderPx(m_face, emMM);

    const float w = ImVarFont::CalcTextWidth(m_face, emMM, utf8.c_str(), letterSp);

    int lines = 1;
    for (char c : utf8)
        if (c == '\n') ++lines;

    const float h = asc + desc + (lines - 1) * lineH;
    return ofRectangle(0.f, -asc, w, h);
}

void VarFontFace::drawString(const std::string& utf8, float x, float y, float emSize,
                             const StringLayoutOptions& opts) const
{
    for (const auto& p : getStringPaths(utf8, x, y, emSize, opts)) {
        if (p.getOutline().empty()) continue;
        p.draw();
    }
}

} // namespace varfont
