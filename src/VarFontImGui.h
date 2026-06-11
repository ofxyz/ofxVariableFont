#pragma once
//
// Optional ImGui rendering helpers (requires ofxImGui in your app).
//

#include "VarFontFace.h"
#include "imgui_var_font.h"
#include "imgui.h"

namespace varfont {

struct ImGuiDrawOptions {
    float lineHeightMult   = 1.f;
    float letterSpacingEm  = 0.f;
    bool  fill             = true;
    bool  outline          = false;
    float outlineThickness = 1.5f;
};

/// Draw text directly to an ImDrawList using ImVarFont vector rendering.
inline float drawToImGui(ImDrawList* dl, VarFontFace& face,
                         float emPx, ImVec2 pos, ImU32 col, const char* text,
                         const ImGuiDrawOptions& opts = {})
{
    ImVarFont::Face* ivf = face.imVarFace();
    if (!dl || !ivf || !text || !*text) return 0.f;

    const float lineH = ImVarFont::CalcLineHeightPx(ivf, emPx) * opts.lineHeightMult;
    const float letterSp = opts.letterSpacingEm * emPx;
    return ImVarFont::AddText(dl, ivf, emPx, pos, col, text,
                              opts.fill, opts.outline, opts.outlineThickness,
                              lineH, letterSp);
}

} // namespace varfont
