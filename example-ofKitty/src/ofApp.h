#pragma once

#include "ofMain.h"
#include "ofxKit.h"
#include "ofxVariableFont.h"
#include "VarFontModGraph.h"

#include <memory>
#include <string>

class ofApp : public ofBaseApp {
public:
    void setup()  override;
    void update() override;
    void draw()   override;
    void exit()   override;
    void dragEvent(ofDragInfo dragInfo) override;

private:
    ofkitty::MainView2D* m_view { nullptr };
    std::unique_ptr<VarFontModGraph> m_modGraph;

    varfont::VarFontFace m_face;
    bool                 m_fontLoaded = false;
    std::string          m_fontPath   = "fonts/Sixtyfour[BLED,SCAN].ttf";
    char                 m_textBuf[4096] = "Variable Font Modulators";
    float                m_emPx          = 120.f;
    float                m_lineHeightMult = 1.f;
    float                m_letterSpacingEm = 0.f;
    bool                 m_extrapolate   = false;

    /// true = NodeGraph on MainView2D overlay; false = docked Modulators window.
    bool m_graphOverlay = true;

    void initVarFontGpuRenderer();
    void tryLoadFont(const std::string& path);
    void applyGraphHostMode();
    void drawNodeOverlay(ofkitty::MainView2D& view);
    void registerControlsWindow();
    void registerModulatorsWindow();
    void registerFontMenu();
    void registerGraphPrefs();
    void drawAddNodeMenuItems();
};
