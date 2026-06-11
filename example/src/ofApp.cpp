#include "ofApp.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstring>

namespace {

static const char* kSampleTexts[] = {
    "The quick brown fox jumps over the lazy dog",
    "Sphinx of black quartz, judge my vow",
    "Pack my box with five dozen liquor jugs",
    "How vexingly quick daft zebras jump!",
    "The quick brown fox\njumps over the lazy dog",
    "Variable Fonts\nWeight  Width  Slant  Custom axes",
    "ABCDEFGHIJKLM\nNOPQRSTUVWXYZ\nabcdefghijklm\nnopqrstuvwxyz",
    "0123456789\n!@#$%^&*()_+-=[]{}",
    "Hello World",
    nullptr
};

static float calcLineWidth(ImVarFont::Face* face, float emPx, float letterSpacingPx,
                           const char* start, const char* end) {
    if (start >= end) return 0.f;
    const std::string line(start, (size_t)(end - start));
    return ImVarFont::CalcTextWidth(face, emPx, line.c_str(), letterSpacingPx);
}

static void normalisePath(char* path) {
    for (char* c = path; *c; ++c)
        if (*c == '\\') *c = '/';
}

} // namespace

// ============================================================================
// setup / exit
// ============================================================================

void ofApp::setup() {
    ofDisableArbTex(); // ImGui needs GL_TEXTURE_2D; ARB rects break raster preview
    ofBackground(10, 10, 18);
    ofSetFrameRate(60);
    ofSetWindowTitle("ImVarFont  –  Variable Font Viewer");

    m_gui.setup(nullptr, /*autoDraw=*/false,
                ImGuiConfigFlags_DockingEnable |
                ImGuiConfigFlags_NavEnableKeyboard);

    auto* atlas = ImGui::GetIO().Fonts;
    m_uiFont = ImFonts::LoadDefaultFonts(atlas, m_uiFontSize);
    ImGui::GetIO().FontDefault = m_uiFont;
    m_gui.rebuildFontsTexture();

    ImTheme::Setup(ImTheme::Theme_SoDark_AccentBlue);
    applyUiFontSize();

    tryLoadFont(m_fontPathBuf);
}

void ofApp::exit() {
    m_rasterTex.clear();
    m_gui.exit();
}

void ofApp::update() {}

// ============================================================================
// draw
// ============================================================================

void ofApp::draw() {
    ofBackground(10, 10, 18);

    if (m_uiFontDirty)
        rebuildUiFont();

    m_gui.begin();

    drawDockSpace();
    drawControlsWindow();
    drawPreviewWindow();
    drawMetadataWindow();
    drawKernWindow();

    m_gui.end();
    m_gui.draw();
}

// ============================================================================
// Docking
// ============================================================================

void ofApp::drawDockSpace() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::Begin("DockSpaceHost", nullptr, flags);
    ImGui::PopStyleVar(2);

    const ImGuiID dockspaceId = ImGui::GetID("ImVarFontDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    if (!m_dockLayoutBuilt) {
        m_dockLayoutBuilt = true;
        ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
        if (!node || node->IsEmpty())
            setupDockLayout(dockspaceId);
    }

    ImGui::End();
}

void ofApp::setupDockLayout(ImGuiID dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    ImGuiID dockMain = dockspaceId;
    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.22f, nullptr, &dockMain);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.26f, nullptr, &dockMain);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.28f, nullptr, &dockMain);

    ImGui::DockBuilderDockWindow("Controls",   dockLeft);
    ImGui::DockBuilderDockWindow("Preview",    dockMain);
    ImGui::DockBuilderDockWindow("Metadata",   dockRight);
    ImGui::DockBuilderDockWindow("Kern table", dockBottom);
    ImGui::DockBuilderFinish(dockspaceId);
}

// ============================================================================
// Controls
// ============================================================================

void ofApp::drawControlsWindow() {
    ImGui::Begin("Controls");

    ImGui::SeparatorText("Font");

    if (ImGui::Button("Browse...", ImVec2(-1.f, 0.f))) {
        ofFileDialogResult r = ofSystemLoadDialog("Load font", false, "");
        if (r.bSuccess)
            tryLoadFont(r.getPath());
    }

    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::InputText("##path", m_fontPathBuf, sizeof(m_fontPathBuf),
                         ImGuiInputTextFlags_EnterReturnsTrue))
        tryLoadFont(m_fontPathBuf);

    if (m_loadError[0])
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", m_loadError);
    else if (m_fontLoaded)
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.f), "%s  %s",
                           m_face.familyName().c_str(),
                           m_face.styleName().c_str());

    ImGui::Spacing();
#ifdef IMGUI_ENABLE_FREETYPE
    if (ImGui::Checkbox("Use loaded font for UI", &m_useFontForUi))
        m_uiFontDirty = true;
    if (m_useFontForUi && !m_fontLoaded)
        ImGui::TextDisabled("Load a font to style the UI");
    else if (m_useFontForUi && m_fontLoaded && m_face.isVariable())
        ImGui::TextDisabled("Axis changes update UI text live");
#else
    {
        bool disabled = false;
        ImGui::BeginDisabled();
        ImGui::Checkbox("Use loaded font for UI", &disabled);
        ImGui::EndDisabled();
        ImGui::TextDisabled("(requires IMGUI_ENABLE_FREETYPE)");
    }
#endif

    ImGui::Text("UI size (px)");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("##uiPx", &m_uiFontSize, 10.f, 28.f, "%.0f px");
    if (ImGui::IsItemEdited())
        applyUiFontSize();

    ImGui::SeparatorText("Text");

    if (ImGui::BeginCombo("Sample", "Choose specimen…")) {
        for (int i = 0; kSampleTexts[i]; ++i) {
            const bool selected = (strcmp(m_textBuf, kSampleTexts[i]) == 0);
            if (ImGui::Selectable(kSampleTexts[i], selected)) {
                strncpy(m_textBuf, kSampleTexts[i], sizeof(m_textBuf) - 1);
                m_textBuf[sizeof(m_textBuf) - 1] = '\0';
                m_rasterDirty = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::InputTextMultiline("##text", m_textBuf, sizeof(m_textBuf),
                                  ImVec2(-1.f, 90.f)))
        m_rasterDirty = true;

    ImGui::Spacing();
    ImGui::Text("Size (px)");
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderFloat("##emPx", &m_emPx, 12.f, 600.f, "%.0f px"))
        m_rasterDirty = true;

    ImGui::Text("Line height");
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderFloat("##lineH", &m_lineHeightMult, 0.5f, 3.f, "%.2f×"))
        m_rasterDirty = true;

    ImGui::Text("Letter spacing");
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderFloat("##letterSp", &m_letterSpacingEm, -0.2f, 0.5f, "%.3f em"))
        m_rasterDirty = true;

    ImGui::Text("Render mode");
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::Combo("##renderMode", &m_renderModeIdx,
                     "Vector\0Hinted vector\0Raster\0")) {
        syncRenderSettings();
        m_rasterDirty = true;
    }

    if (m_renderModeIdx != 0) {
        ImGui::Text("Hinting");
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::Combo("##hinting", &m_hintingIdx,
                         "Native\0Light\0Auto-hint\0")) {
            syncRenderSettings();
            m_rasterDirty = true;
        }
    }

    if (m_renderModeIdx == 2) {
        ImGui::TextDisabled("Fill / outline apply to vector modes only");
    } else {
        ImGui::Checkbox("Fill", &m_fill);
        ImGui::SameLine();
        ImGui::Checkbox("Outline", &m_outline);
        if (m_outline) {
            ImGui::Text("Outline thickness");
            ImGui::SetNextItemWidth(-1.f);
            ImGui::SliderFloat("##thick", &m_thickness, 0.5f, 8.f, "%.1f px");
        }
    }

    ImVarFont::Face* ivf = m_face.imVarFace();
    if (ivf) {
        ImGui::SeparatorText("Kerning");

        bool kern = ImVarFont::GetUseKerning(ivf);
        if (ImGui::Checkbox("Kerning", &kern))
            ImVarFont::SetUseKerning(ivf, kern);

        ImGui::TextDisabled("Engine: %s", ImVarFont::GetKerningEngineLabel(ivf));

        if (ImVarFont::UsesHarfBuzz(ivf)) {
            bool useHb = ImVarFont::GetUseHarfBuzz(ivf);
            if (ImGui::Checkbox("Use HarfBuzz", &useHb))
                ImVarFont::SetUseHarfBuzz(ivf, useHb);
            if (!ImVarFont::HasGpos(ivf))
                ImGui::TextDisabled("(font has no GPOS table)");
        } else {
            bool useHb = false;
            ImGui::BeginDisabled();
            ImGui::Checkbox("Use HarfBuzz", &useHb);
            ImGui::EndDisabled();
            ImGui::TextDisabled("(not built with HarfBuzz)");
        }

        if (ImVarFont::HasKernTable(ivf)) {
            bool useKern = ImVarFont::GetUseKernTable(ivf);
            if (ImGui::Checkbox("Use kern table", &useKern))
                ImVarFont::SetUseKernTable(ivf, useKern);
        } else {
            bool useKern = false;
            ImGui::BeginDisabled();
            ImGui::Checkbox("Use kern table", &useKern);
            ImGui::EndDisabled();
            ImGui::TextDisabled("(font has no kern table)");
        }
    }

    ImGui::Spacing();
    if (ImGui::ColorEdit3("Text", (float*)&m_textColor, ImGuiColorEditFlags_NoInputs))
        m_rasterDirty = true;
    ImGui::SameLine();
    ImGui::ColorEdit3("BG", (float*)&m_bgColor, ImGuiColorEditFlags_NoInputs);

    if (ivf && m_face.isVariable()) {
        ImGui::SeparatorText("Axes");
        if (ImVarFont::AxisSliders(ivf, "##axes", m_extrapolate)) {
            ImVarFont::ApplyAxes(ivf, m_extrapolate);
#ifdef IMGUI_ENABLE_FREETYPE
            if (m_useFontForUi) m_uiFontDirty = true;
#endif
            m_rasterDirty = true;
        }
        if (ImGui::Checkbox("Extrapolate beyond limits", &m_extrapolate)) {
            ImVarFont::ApplyAxes(ivf, m_extrapolate);
            m_rasterDirty = true;
        }
    }

    ImGui::End();
}

// ============================================================================
// Preview
// ============================================================================

void ofApp::calcTextBlock(ImVarFont::Face* face, float emPx, float lineHeightPx,
                          float letterSpacingPx, const char* text,
                          float* outW, float* outH) const {
    const float asc  = ImVarFont::CalcAscenderPx(face, emPx);
    const float desc = ImVarFont::CalcDescenderPx(face, emPx);
    const float lineH = lineHeightPx;

    float maxW = 0.f;
    int lines = 1;
    const char* lineStart = text;
    for (const char* p = text; ; ++p) {
        if (*p == '\n' || *p == '\0') {
            maxW = std::max(maxW, calcLineWidth(face, emPx, letterSpacingPx, lineStart, p));
            if (*p == '\0') break;
            ++lines;
            lineStart = p + 1;
        }
    }

    *outW = maxW;
    *outH = asc + desc + (lines - 1) * lineH;
}

void ofApp::drawPreviewWindow() {
    ImGui::PushStyleColor(ImGuiCol_WindowBg,
                          ImGui::ColorConvertFloat4ToU32(m_bgColor));
    ImGui::Begin("Preview", nullptr, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();

    const ImVec2 canvasPos  = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    ImGui::InvisibleButton("##preview_canvas", canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft);
    const bool canvasHovered = ImGui::IsItemHovered();
    const bool canvasActive  = ImGui::IsItemActive();

    ImGuiIO& io = ImGui::GetIO();
    if (canvasHovered && io.MouseWheel != 0.f) {
        const ImVec2 center = {
            canvasPos.x + canvasSize.x * 0.5f,
            canvasPos.y + canvasSize.y * 0.5f
        };
        const ImVec2 world = {
            (io.MousePos.x - center.x - m_previewPan.x) / m_previewZoom,
            (io.MousePos.y - center.y - m_previewPan.y) / m_previewZoom
        };
        m_previewZoom *= (1.f + io.MouseWheel * 0.12f);
        m_previewZoom = std::clamp(m_previewZoom, 0.05f, 20.f);
        m_previewPan.x = io.MousePos.x - center.x - world.x * m_previewZoom;
        m_previewPan.y = io.MousePos.y - center.y - world.y * m_previewZoom;
        m_rasterDirty = true;
    }
    if (canvasActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        m_previewPan = { m_previewPan.x + io.MouseDelta.x,
                         m_previewPan.y + io.MouseDelta.y };
    if (canvasHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        m_previewPan  = { 0.f, 0.f };
        m_previewZoom = 1.f;
        m_rasterDirty = true;
    }

    ImVarFont::Face* ivf = m_face.imVarFace();
    if (ivf) {
        syncRenderSettings();
        const float emPx = m_emPx * m_previewZoom;
        const float lineH = ImVarFont::CalcLineHeightPx(ivf, emPx) * m_lineHeightMult;
        const float letterSp = m_letterSpacingEm * emPx;
        float textW = 0.f, textH = 0.f;
        calcTextBlock(ivf, emPx, lineH, letterSp, m_textBuf, &textW, &textH);

        const ImVec2 center = {
            canvasPos.x + canvasSize.x * 0.5f,
            canvasPos.y + canvasSize.y * 0.5f
        };
        const float posX = center.x - textW * 0.5f + m_previewPan.x;
        const float posY = center.y - textH * 0.5f + m_previewPan.y;

        const ImU32 col = ImGui::ColorConvertFloat4ToU32(m_textColor);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        if (m_renderModeIdx == (int)ImVarFont::RenderMode::Raster) {
            if (m_rasterDirty) {
                int rw = 0, rh = 0;
                if (ImVarFont::RasterizeText(ivf, emPx, m_textBuf, col, lineH, letterSp,
                                             m_rasterPixels, rw, rh) && rw > 0 && rh > 0) {
                    ofPixels px;
                    px.setFromPixels(m_rasterPixels.data(), rw, rh, OF_PIXELS_RGBA);
                    if (!m_rasterTex.isAllocated()
                        || m_rasterTex.getWidth() != rw
                        || m_rasterTex.getHeight() != rh) {
                        m_rasterTex.allocate(px, /*bUseARBExtension=*/false);
                    } else {
                        m_rasterTex.loadData(px);
                    }
                }
                m_rasterDirty = false;
            }
            if (m_rasterTex.isAllocated()) {
                const float drawX = center.x - m_rasterTex.getWidth() * 0.5f + m_previewPan.x;
                const float drawY = center.y - m_rasterTex.getHeight() * 0.5f + m_previewPan.y;
                const float texW = (float)m_rasterTex.getWidth();
                const float texH = (float)m_rasterTex.getHeight();
                dl->AddImage(
                    (ImTextureID)(intptr_t)m_rasterTex.getTextureData().textureID,
                    ImVec2(drawX, drawY),
                    ImVec2(drawX + texW, drawY + texH));
            }
        } else {
            ImVarFont::AddText(dl, ivf, emPx, ImVec2(posX, posY),
                               col, m_textBuf, m_fill, m_outline,
                               m_thickness * m_previewZoom, lineH, letterSp);
        }
    } else {
        const char* hint = "Drop a .ttf / .otf file here,  or enter its path and press Enter";
        const ImVec2 ts = ImGui::CalcTextSize(hint);
        ImGui::SetCursorScreenPos({
            canvasPos.x + (canvasSize.x - ts.x) * 0.5f,
            canvasPos.y + (canvasSize.y - ts.y) * 0.5f
        });
        ImGui::TextDisabled("%s", hint);
    }

    ImGui::End();
}

// ============================================================================
// Metadata / Kern table
// ============================================================================

void ofApp::drawMetadataWindow() {
    ImGui::Begin("Metadata");
    ImVarFont::MetadataTable(m_face.imVarFace());
    ImGui::End();
}

void ofApp::drawKernWindow() {
    ImGui::Begin("Kern table");
    ImVarFont::KernTableUi(m_face.imVarFace(), m_emPx);
    ImGui::End();
}

// ============================================================================
// Helpers
// ============================================================================

void ofApp::tryLoadFont(const std::string& path) {
    m_loadError[0] = '\0';
    m_face.unload();
    m_fontLoaded = false;
    if (path.empty()) return;

    strncpy(m_fontPathBuf, path.c_str(), sizeof(m_fontPathBuf) - 1);
    m_fontPathBuf[sizeof(m_fontPathBuf) - 1] = '\0';
    normalisePath(m_fontPathBuf);

    if (m_face.load(m_fontPathBuf)) {
        m_fontLoaded = true;
        m_face.applyAxes(m_extrapolate);
        syncRenderSettings();
        m_uiFontDirty = true;
        m_rasterDirty = true;
        ofSetWindowTitle("ImVarFont  –  Variable Font Viewer  —  " + m_face.familyName());
    } else {
        snprintf(m_loadError, sizeof(m_loadError), "Failed to load: %s", m_fontPathBuf);
    }
}

void ofApp::syncRenderSettings() {
    ImVarFont::Face* ivf = m_face.imVarFace();
    if (!ivf) return;
    ImVarFont::SetRenderMode(ivf, (ImVarFont::RenderMode)m_renderModeIdx);
    ImVarFont::SetHintingFlags(ivf, (ImVarFont::HintingFlags)m_hintingIdx);
}

void ofApp::applyUiFontSize() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.FontSizeBase = m_uiFontSize;
    style._NextFrameFontSizeBase = m_uiFontSize;
    if (ImGui::GetCurrentContext())
        ImGui::UpdateCurrentFontSize(0.0f);
}

void ofApp::rebuildUiFont() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->ClearFonts();

#ifdef IMGUI_ENABLE_FREETYPE
    if (m_useFontForUi && m_face.imVarFace()) {
        m_uiFont = ImVarFont::SetImGuiFont(io.Fonts, m_face.imVarFace(), m_uiFontSize);
        if (!m_uiFont)
            m_uiFont = ImFonts::LoadDefaultFonts(io.Fonts, m_uiFontSize);
    } else {
        m_uiFont = ImFonts::LoadDefaultFonts(io.Fonts, m_uiFontSize);
    }
#else
    m_uiFont = ImFonts::LoadDefaultFonts(io.Fonts, m_uiFontSize);
#endif

    if (m_uiFont)
        io.FontDefault = m_uiFont;

    applyUiFontSize();
    m_gui.rebuildFontsTexture();
    m_uiFontDirty = false;
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.empty()) return;
    tryLoadFont(dragInfo.files[0].string());
}

void ofApp::keyPressed(int key) {
    if ((key == 'r' || key == 'R') && m_face.imVarFace()) {
        ImVarFont::ResetAxes(m_face.imVarFace());
        ImVarFont::ApplyAxes(m_face.imVarFace(), m_extrapolate);
        m_rasterDirty = true;
#ifdef IMGUI_ENABLE_FREETYPE
        if (m_useFontForUi) m_uiFontDirty = true;
#endif
    }
}
