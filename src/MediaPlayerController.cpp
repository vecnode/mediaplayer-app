/*
 * media-player-cpp — MediaPlayerController
 * Copyright (c) vecnode 2026
 */

#include "MediaPlayerController.h"

namespace {

const char* mediaTypeLabel(ClipMediaType type) {
	return type == ClipMediaType::Image ? "image" : "video";
}

} // namespace

void MediaPlayerController::setup(MediaPanel* panel, SubtitlesOverlay* subtitles) {
	mediaPanel = panel;
	subtitlesOverlay = subtitles;

	if (mediaPanel) {
		mediaPanel->setClipChangedHandler([this]() { syncSubtitleText(); });
	}
}

void MediaPlayerController::syncSubtitleText() {
	if (!subtitlesOverlay || !mediaPanel) {
		return;
	}

	if (mediaPanel->isLoaded()) {
		subtitlesOverlay->setText(mediaPanel->getLoadedPath());
	} else {
		subtitlesOverlay->setText("Sample subtitle");
	}
}

void MediaPlayerController::play() {
	if (!mediaPanel || mediaPanel->isCurrentClipImage()) {
		return;
	}
	mediaPanel->play();
}

void MediaPlayerController::stop() {
	if (!mediaPanel || mediaPanel->isCurrentClipImage()) {
		return;
	}
	mediaPanel->stop();
}

void MediaPlayerController::nextClip() {
	if (!mediaPanel) {
		return;
	}
	mediaPanel->cycleNext();
}

void MediaPlayerController::previousClip() {
	if (!mediaPanel) {
		return;
	}
	mediaPanel->cyclePrevious();
}

bool MediaPlayerController::openClipAtIndex(std::size_t index) {
	if (!mediaPanel) {
		return false;
	}
	return mediaPanel->openClipAtIndex(index);
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

bool MediaPlayerController::isCurrentClipImage() const {
	return mediaPanel && mediaPanel->isCurrentClipImage();
}

MediaPlayerStatus MediaPlayerController::getStatus() const {
	MediaPlayerStatus status;

	if (!mediaPanel) {
		return status;
	}

	status.loaded = mediaPanel->isLoaded();
	status.playing = mediaPanel->isPlaying();
	status.isImage = mediaPanel->isCurrentClipImage();
	status.clipIndex = mediaPanel->getCurrentIndex();
	status.clipCount = mediaPanel->getClipCount();
	status.clipName = mediaPanel->getLoadedPath();
	status.subtitlesEnabled = isSubtitlesEnabled();

	return status;
}

std::vector<MediaPlayerClipInfo> MediaPlayerController::getClips() const {
	std::vector<MediaPlayerClipInfo> clips;

	if (!mediaPanel) {
		return clips;
	}

	const auto& library = mediaPanel->getLibrary();
	clips.reserve(library.size());

	for (std::size_t i = 0; i < library.size(); ++i) {
		const MediaClip& clip = library.clipAt(i);
		MediaPlayerClipInfo info;
		info.index = i;
		info.name = clip.displayName;
		info.path = clip.absolutePath;
		info.mediaType = mediaTypeLabel(clip.mediaType);
		clips.push_back(std::move(info));
	}

	return clips;
}
