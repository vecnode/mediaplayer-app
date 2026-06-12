/*
 * media-player-cpp — MediaPlayerController
 * Copyright (c) vecnode 2026
 */

#include "MediaPlayerController.h"

void MediaPlayerController::setup(VideoPanel* panel, SubtitlesOverlay* subtitles) {
	videoPanel = panel;
	subtitlesOverlay = subtitles;
}

void MediaPlayerController::syncSubtitleText() {
	if (!subtitlesOverlay || !videoPanel) {
		return;
	}

	if (videoPanel->isLoaded()) {
		subtitlesOverlay->setText(videoPanel->getLoadedPath());
	} else {
		subtitlesOverlay->setText("Sample subtitle");
	}
}

void MediaPlayerController::play() {
	if (!videoPanel) {
		return;
	}
	videoPanel->play();
}

void MediaPlayerController::stop() {
	if (!videoPanel) {
		return;
	}
	videoPanel->stop();
}

void MediaPlayerController::nextClip() {
	if (!videoPanel) {
		return;
	}
	videoPanel->cycleNext();
	syncSubtitleText();
}

bool MediaPlayerController::openClipAtIndex(std::size_t index) {
	if (!videoPanel) {
		return false;
	}
	if (!videoPanel->openClipAtIndex(index)) {
		return false;
	}
	syncSubtitleText();
	return true;
}

bool MediaPlayerController::setSubtitlesEnabled(bool enabled) {
	if (!subtitlesOverlay) {
		return false;
	}
	subtitlesOverlay->setEnabled(enabled);
	return true;
}

bool MediaPlayerController::isSubtitlesEnabled() const {
	return subtitlesOverlay && subtitlesOverlay->isEnabled();
}

MediaPlayerStatus MediaPlayerController::getStatus() const {
	MediaPlayerStatus status;

	if (!videoPanel) {
		return status;
	}

	status.loaded = videoPanel->isLoaded();
	status.playing = videoPanel->isPlaying();
	status.clipIndex = videoPanel->getCurrentIndex();
	status.clipCount = videoPanel->getClipCount();
	status.clipName = videoPanel->getLoadedPath();
	status.subtitlesEnabled = isSubtitlesEnabled();

	return status;
}

std::vector<MediaPlayerClipInfo> MediaPlayerController::getClips() const {
	std::vector<MediaPlayerClipInfo> clips;

	if (!videoPanel) {
		return clips;
	}

	const auto& library = videoPanel->getLibrary();
	clips.reserve(library.size());

	for (std::size_t i = 0; i < library.size(); ++i) {
		const VideoClip& clip = library.clipAt(i);
		MediaPlayerClipInfo info;
		info.index = i;
		info.name = clip.displayName;
		info.path = clip.absolutePath;
		clips.push_back(std::move(info));
	}

	return clips;
}
