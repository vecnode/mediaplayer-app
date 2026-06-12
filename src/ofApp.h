#pragma once

#include "VideoPanel.h"
#include "ofMain.h"
#include "ofxGui.h"

/// Application entry point for the media player.
///
/// Owns a full-window VideoPanel and an ofxGui control panel.
/// The main loop runs uncapped (frame rate 0, vsync off) so Media Foundation
/// decode events are pumped as quickly as possible; MF handles A/V clocking.
class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;
	void windowResized(int w, int h) override;

private:
	void onPlayPressed();
	void onCyclePressed();
	void onStopPressed();
	void refreshStatusLabel();

	VideoPanel videoPanel;
	ofRectangle panelBounds;

	ofxPanel gui;
	ofxButton playButton;
	ofxButton cycleButton;
	ofxButton stopButton;
	ofxLabel statusLabel;
};
