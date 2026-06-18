#pragma once

#include "HttpControlServer.h"
#include "MediaPlayerController.h"
#include "MediaPanel.h"
#include "SubtitlesOverlay.h"
#include "ofMain.h"
#include "ofxGui.h"

/// Application entry point for the media player.
///
/// Owns playback (MediaPanel), subtitles, GUI, and the HTTP control API.
class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;
	void windowResized(int w, int h) override;

private:
	static ofRectangle mediaBoundsForWindow(int w, int h);

	void onPlayPressed();
	void onCyclePressed();
	void onStopPressed();
	void onSubtitlesToggled(bool& value);
	void refreshGuiFromController();

	MediaPlayerController controller;
	HttpControlServer httpServer;

	MediaPanel mediaPanel;
	SubtitlesOverlay subtitles;
	ofRectangle panelBounds;

	ofxPanel gui;
	ofxButton playButton;
	ofxButton cycleButton;
	ofxButton stopButton;
	ofxToggle subtitlesToggle;
	ofxLabel statusLabel;
};
