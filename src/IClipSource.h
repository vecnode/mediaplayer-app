#pragma once

#include <cstddef>
#include <string>

enum class ClipMediaType {
	Video,
	Image
};

/// Metadata for one playable file in the playlist.
struct VideoClip {
	std::string absolutePath;
	std::string displayName;
	ClipMediaType mediaType = ClipMediaType::Video;
};

/// Playlist source abstraction — swap implementations (disk scan, JSON, M3U) without
/// touching the playback engine.
class IClipSource {
public:
	virtual ~IClipSource() = default;

	virtual bool empty() const = 0;
	virtual std::size_t size() const = 0;
	virtual const VideoClip& clipAt(std::size_t index) const = 0;
	virtual std::size_t nextIndex(std::size_t currentIndex) const = 0;
	virtual std::size_t previousIndex(std::size_t currentIndex) const = 0;
};
