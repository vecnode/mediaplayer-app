/*
 * media-player-cpp — SubtitlesOverlay
 * Copyright (c) vecnode 2026
 */

#include "SubtitlesOverlay.h"

#include <algorithm>
#include <sstream>

namespace {

bool tryLoadFont(ofTrueTypeFont& font, const std::string& path, int size) {
	if (!ofFile::doesFileExist(path)) {
		return false;
	}
	return font.load(path, size);
}

} // namespace

bool SubtitlesOverlay::loadFont() {
	static const char* kCandidates[] = {
		"fonts/verdana.ttf",
		"verdana.ttf",
#ifdef TARGET_WIN32
		"C:/Windows/Fonts/segoeui.ttf",
		"C:/Windows/Fonts/arial.ttf",
#endif
	};

	for (const char* path : kCandidates) {
		if (tryLoadFont(font, path, kFontSize)) {
			ofLogNotice("SubtitlesOverlay") << "Font: " << path;
			return true;
		}
	}

	if (font.load(OF_TTF_SANS, kFontSize)) {
		ofLogNotice("SubtitlesOverlay") << "Font: " << OF_TTF_SANS;
		return true;
	}

	ofLogWarning("SubtitlesOverlay") << "No TTF found; using bitmap fallback";
	return false;
}

void SubtitlesOverlay::setup() {
	fontLoaded = loadFont();
}

void SubtitlesOverlay::setEnabled(bool value) {
	enabled = value;
}

void SubtitlesOverlay::toggle() {
	enabled = !enabled;
}

void SubtitlesOverlay::setText(const std::string& value) {
	text = value;
}

std::vector<std::string> SubtitlesOverlay::wrapText(float maxWidth) const {
	std::vector<std::string> lines;
	if (text.empty()) {
		return lines;
	}

	if (!fontLoaded || maxWidth <= 0.f) {
		lines.push_back(text);
		return lines;
	}

	std::istringstream stream(text);
	std::string word;
	std::string current;
	while (stream >> word) {
		const std::string trial = current.empty() ? word : current + " " + word;
		if (font.getStringBoundingBox(trial, 0, 0).width <= maxWidth) {
			current = trial;
		} else {
			if (!current.empty()) {
				lines.push_back(current);
			}
			current = word;
		}
	}
	if (!current.empty()) {
		lines.push_back(current);
	}
	return lines;
}

void SubtitlesOverlay::draw(const ofRectangle& windowBounds) const {
	if (!enabled || text.empty() || windowBounds.width <= 0.f || windowBounds.height <= 0.f) {
		return;
	}

	const float maxTextWidth = windowBounds.width * kWindowWidthFraction;

	if (fontLoaded) {
		const std::vector<std::string> lines = wrapText(maxTextWidth);
		if (lines.empty()) {
			return;
		}

		float totalHeight = 0.f;
		float maxLineWidth = 0.f;
		std::vector<ofRectangle> lineBounds;
		lineBounds.reserve(lines.size());

		for (const std::string& line : lines) {
			const ofRectangle bounds = font.getStringBoundingBox(line, 0, 0);
			lineBounds.push_back(bounds);
			maxLineWidth = std::max(maxLineWidth, bounds.width);
			totalHeight += bounds.height;
			if (!lineBounds.empty() && &line != &lines.back()) {
				totalHeight += kLineSpacing;
			}
		}

		const float boxW = maxLineWidth + kPadding * 2.f;
		const float boxH = totalHeight + kPadding * 2.f;
		const float boxX = windowBounds.x + (windowBounds.width - boxW) * 0.5f;
		const float boxY = windowBounds.y + windowBounds.height - kBottomMargin - boxH;

		ofSetColor(0, 200);
		ofDrawRectangle(boxX, boxY, boxW, boxH);

		ofSetColor(255);
		float cursorY = boxY + kPadding;
		for (std::size_t i = 0; i < lines.size(); ++i) {
			const ofRectangle& bounds = lineBounds[i];
			const float lineX = boxX + kPadding + (maxLineWidth - bounds.width) * 0.5f - bounds.x;
			font.drawString(lines[i], lineX, cursorY - bounds.y);
			cursorY += bounds.height + kLineSpacing;
		}
		return;
	}

	// Bitmap fallback: single truncated line.
	const float lineH = 16.f;
	const std::string line = text.size() > 120 ? text.substr(0, 117) + "..." : text;
	const float textW = static_cast<float>(line.size()) * 8.f;
	const float boxW = std::min(textW + kPadding * 2.f, maxTextWidth + kPadding * 2.f);
	const float boxH = lineH + kPadding * 2.f;
	const float boxX = windowBounds.x + (windowBounds.width - boxW) * 0.5f;
	const float boxY = windowBounds.y + windowBounds.height - kBottomMargin - boxH;

	ofSetColor(0, 200);
	ofDrawRectangle(boxX, boxY, boxW, boxH);

	ofSetColor(255);
	ofDrawBitmapString(line, boxX + kPadding, boxY + kPadding + lineH - 4.f);
}
