#ifndef SLATE_VIDEO_INFO_HPP
#define SLATE_VIDEO_INFO_HPP

namespace slate {

struct VideoInfo {
    double duration;         // duration of the video track, in seconds.
    double fps;
    long   nb_frames;
    int    width, height;
    int    display_rotation; // 0, 90, 180, 270 or -1 for others. Clockwise!
    bool   is_cfr;           // is constant framerate?
};

} /* namespace slate */

#endif /* SLATE_VIDEO_INFO_HPP */
