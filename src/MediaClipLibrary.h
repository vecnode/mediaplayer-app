#pragma once

#include "IClipSource.h"
#include "ofMain.h"
#include <string>
#include <vector>

/// Discovers images and videos from disk (default IClipSource implementation).
///
/// Images are listed before videos in the playlist. Runtime assets live in bin/data/
/// and are not modified by make Release or make RunRelease.
class MediaClipLibrary : public IClipSource {
public:
	void scan();

	bool empty() const override { return clips.empty(); }
	std::size_t size() const override { return clips.size(); }

	const MediaClip& clipAt(std::size_t index) const override;
	std::size_t nextIndex(std::size_t currentIndex) const override;
	std::size_t previousIndex(std::size_t currentIndex) const override;

	const std::vector<MediaClip>& allClips() const { return clips; }
	const std::string& getSearchLog() const { return searchLog; }

private:
	std::vector<MediaClip> clips;
	std::string searchLog;
};
