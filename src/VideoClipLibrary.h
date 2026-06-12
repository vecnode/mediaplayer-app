#pragma once

#include "IClipSource.h"
#include "ofMain.h"
#include <string>
#include <vector>

/// Discovers and indexes video files from disk (default IClipSource implementation).
class VideoClipLibrary : public IClipSource {
public:
	void scan();

	bool empty() const override { return clips.empty(); }
	std::size_t size() const override { return clips.size(); }

	const VideoClip& clipAt(std::size_t index) const override;
	std::size_t nextIndex(std::size_t currentIndex) const override;
	std::size_t previousIndex(std::size_t currentIndex) const override;

	const std::vector<VideoClip>& allClips() const { return clips; }
	const std::string& getSearchLog() const { return searchLog; }

private:
	std::vector<VideoClip> clips;
	std::string searchLog;
};
