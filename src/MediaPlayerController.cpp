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
		mediaPanel->setClipChangedHandler([this]() {
			subtitleOverride_.clear();
			syncSubtitleText();
		});
	}

	if (subtitlesOverlay) {
		subtitlesOverlay->setEnabled(true);
	}
	syncSubtitleText();
}

void MediaPlayerController::syncSubtitleText() {
	if (!subtitlesOverlay || !mediaPanel) {
		return;
	}

	if (!subtitleOverride_.empty()) {
		subtitlesOverlay->setText(subtitleOverride_);
		return;
	}

	if (!mediaPanel->isLoaded()) {
		subtitlesOverlay->setText({});
		return;
	}

	const std::string clipName = mediaPanel->getLoadedPath();
	const std::string summary = mediaPanel->getCorpus().subtitleSummary(clipName);
	if (!summary.empty()) {
		subtitlesOverlay->setText(summary);
		return;
	}

	const std::string preview = mediaPanel->getCorpus().subtitlePreview(clipName);
	subtitlesOverlay->setText(preview.empty() ? clipName : preview);
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

void MediaPlayerController::randomClip() {
	if (!mediaPanel) {
		return;
	}
	mediaPanel->cycleRandom();
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

bool MediaPlayerController::setSubtitleText(const std::string& text) {
	if (!subtitlesOverlay) {
		return false;
	}
	subtitleOverride_ = text;
	subtitlesOverlay->setText(text);
	return true;
}

void MediaPlayerController::clearSubtitleOverride() {
	subtitleOverride_.clear();
	syncSubtitleText();
}

bool MediaPlayerController::isSubtitlesEnabled() const {
	return subtitlesOverlay && subtitlesOverlay->isEnabled();
}

bool MediaPlayerController::isCurrentClipImage() const {
	return mediaPanel && mediaPanel->isCurrentClipImage();
}

bool MediaPlayerController::setShowRegionBBox(bool enabled) {
	if (!mediaPanel) {
		return false;
	}
	mediaPanel->setShowRegionBBox(enabled);
	return true;
}

bool MediaPlayerController::showRegionBBox() const {
	return mediaPanel && mediaPanel->showRegionBBox();
}

bool MediaPlayerController::setRegionFocusEnabled(bool enabled) {
	if (!mediaPanel) {
		return false;
	}
	mediaPanel->setRegionFocusEnabled(enabled);
	return true;
}

bool MediaPlayerController::regionFocusEnabled() const {
	return mediaPanel && mediaPanel->regionFocusEnabled();
}

bool MediaPlayerController::setRegionPanEnabled(bool enabled) {
	if (!mediaPanel) {
		return false;
	}
	mediaPanel->setRegionPanEnabled(enabled);
	return true;
}

bool MediaPlayerController::regionPanEnabled() const {
	return mediaPanel && mediaPanel->regionPanEnabled();
}

bool MediaPlayerController::setAnimationsEnabled(bool enabled) {
	if (!mediaPanel) {
		return false;
	}
	mediaPanel->setAnimationsEnabled(enabled);
	return true;
}

bool MediaPlayerController::animationsEnabled() const {
	return mediaPanel && mediaPanel->animationsEnabled();
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
	status.subtitleText = getSubtitleText();

	return status;
}

std::string MediaPlayerController::getSubtitleText() const {
	if (subtitlesOverlay) {
		return subtitlesOverlay->getText();
	}
	return {};
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
