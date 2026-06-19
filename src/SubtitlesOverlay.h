#pragma once

#include "ofMain.h"
#include <string>
#include <vector>

/// Renders wrapped subtitle paragraphs with a semi-transparent backing plane.
class SubtitlesOverlay {
public:
	void setup();

	void draw(const ofRectangle& windowBounds) const;

	void setEnabled(bool enabled);
	bool isEnabled() const { return enabled; }
	void toggle();

	void setText(const std::string& text);
	const std::string& getText() const { return text; }

private:
	bool loadFont();
	std::vector<std::string> wrapText(float maxWidth) const;

	ofTrueTypeFont font;
	bool fontLoaded = false;
	bool enabled = true;
	std::string text;

	static constexpr float kWindowWidthFraction = 0.86f;
	static constexpr float kBottomMargin = 28.f;
	static constexpr float kPadding = 16.f;
	static constexpr float kLineSpacing = 6.f;
	static constexpr int kFontSize = 26;
};
