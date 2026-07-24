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
    void mouseDragged(int x, int y, int button) override;
    void mouseScrolled(ofMouseEventArgs& mouse) override;

private:
    ofxImGui::Gui  m_gui;
    bool           m_dockLayoutBuilt = false;

    ImFont*        m_uiFont   = nullptr;
    bool           m_uiFontDirty = true;
    bool           m_useFontForUi  = false;
    float          m_uiFontSize    = 15.f;

    varfont::VarFontFace m_face;
    bool           m_fontLoaded = false;
    char           m_fontPathBuf[512] = "fonts/Sixtyfour[BLED,SCAN].ttf";
    char           m_loadError[256]   = "";
    char           m_textBuf[4096]    = "The quick brown fox\njumps over the lazy dog";

    float          m_emPx            = 140.f;
    float          m_lineHeightMult  = 1.f;
    float          m_letterSpacingEm = 0.f;
    float          m_thickness       = 1.5f;
    bool           m_fill            = true;
    bool           m_outline         = false;
    bool           m_extrapolate     = false;
    bool           m_morph           = false;
    bool           m_forceCpuFill    = false;
    bool           m_showImGuiPreview = false;

    ofColor        m_textColor { 255, 255, 255, 255 };
    ofColor        m_bgColor   { 10, 10, 18, 255 };

    glm::vec2      m_previewPan  { 0.f, 0.f };
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
    void drawImGuiPreviewWindow();
    void drawMetadataWindow();
    void drawKernWindow();
    void drawOfCanvasText();

    void calcTextBlock(ImVarFont::Face* face, float emPx, float lineHeightPx,
                       float letterSpacingPx, const char* text,
                       float* outW, float* outH) const;
};
