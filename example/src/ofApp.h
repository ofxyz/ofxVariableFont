#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxVariableFont.h"
#include "ImThemeRegistry.h"
#include "ImFonts.h"

#include <string>
#include <vector>

class ofApp : public ofBaseApp {
public:
    void setup()  override;
    void update() override;
    void draw()   override;
    void exit()   override;
    void dragEvent(ofDragInfo dragInfo) override;
    void keyPressed(int key) override;

private:
    ofxImGui::Gui  m_gui;
    bool           m_dockLayoutBuilt = false;

    ImFont*        m_uiFont   = nullptr;
    bool           m_uiFontDirty = true;
    bool           m_useFontForUi  = false;
    float          m_uiFontSize    = 15.f;

    varfont::VarFontFace m_face;
    bool           m_fontLoaded = false;
    char           m_fontPathBuf[512] = "fonts/Sixtyfour.otf";
    char           m_loadError[256]   = "";
    char           m_textBuf[4096]    = "The quick brown fox\njumps over the lazy dog";

    float          m_emPx            = 140.f;
    float          m_lineHeightMult  = 1.f;
    float          m_letterSpacingEm = 0.f;
    float          m_thickness       = 1.5f;
    bool           m_fill            = true;
    bool           m_outline         = false;
    bool           m_extrapolate     = false;

    ImVec4         m_textColor { 1.f, 1.f, 1.f, 1.f };
    ImVec4         m_bgColor   { 0.04f, 0.04f, 0.07f, 1.f };

    ImVec2         m_previewPan  { 0.f, 0.f };
    float          m_previewZoom = 1.f;

    int            m_renderModeIdx = 0;
    int            m_hintingIdx    = 0;
    bool           m_rasterDirty   = true;
    ofTexture      m_rasterTex;
    std::vector<uint8_t> m_rasterPixels;

    void tryLoadFont(const std::string& path);
    void syncRenderSettings();
    void rebuildUiFont();
    void applyUiFontSize();

    void setupDockLayout(ImGuiID dockspaceId);
    void drawDockSpace();
    void drawControlsWindow();
    void drawPreviewWindow();
    void drawMetadataWindow();
    void drawKernWindow();

    void calcTextBlock(ImVarFont::Face* face, float emPx, float lineHeightPx,
                       float letterSpacingPx, const char* text,
                       float* outW, float* outH) const;
};
