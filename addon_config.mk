meta:
	ADDON_NAME = ofxVariableFont
	ADDON_DESCRIPTION = Variable font loading, axis control, kerning, and outline rendering for openFrameworks.
	ADDON_AUTHOR = ofKitty
	ADDON_TAGS = "font" "typography" "variable font"
	ADDON_URL = https://www.github.com/ofxyz/ofxVariableFont

common:
	ADDON_DEPENDENCIES = ofxImGui
	ADDON_SOURCES += src/VarFontNodes.cpp

msys2:
	ADDON_CPPFLAGS += -DIMVARFONT_USE_HARFBUZZ
	ADDON_PKG_CONFIG_LIBRARIES = harfbuzz

linux64:
	ADDON_CPPFLAGS += -DIMVARFONT_USE_HARFBUZZ
	ADDON_PKG_CONFIG_LIBRARIES = harfbuzz

linuxarmv6l:
	ADDON_CPPFLAGS += -DIMVARFONT_USE_HARFBUZZ
	ADDON_PKG_CONFIG_LIBRARIES = harfbuzz

linuxarmv7l:
	ADDON_CPPFLAGS += -DIMVARFONT_USE_HARFBUZZ
	ADDON_PKG_CONFIG_LIBRARIES = harfbuzz

linuxaarch64:
	ADDON_CPPFLAGS += -DIMVARFONT_USE_HARFBUZZ
	ADDON_PKG_CONFIG_LIBRARIES = harfbuzz

osx:
	ADDON_CPPFLAGS += -DIMVARFONT_USE_HARFBUZZ
	ADDON_PKG_CONFIG_LIBRARIES = harfbuzz

vs:
	# To enable GPOS on Visual Studio, install HarfBuzz (e.g. vcpkg) and uncomment:
	# ADDON_INCLUDES += path/to/harfbuzz/include
	# ADDON_LIBS += path/to/harfbuzz.lib
	# ADDON_CPPFLAGS += -DIMVARFONT_USE_HARFBUZZ
