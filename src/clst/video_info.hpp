#ifndef CLST_VIDEO_INFO_HPP
#define CLST_VIDEO_INFO_HPP

namespace clst {

struct VideoInfo {
    double duration;         // duration of the video track, in seconds.
    double fps;
    long   nb_frames;
    int    width, height;
    int    display_rotation; // 0, 90, 180, 270 or -1 for others. Clockwise!
    bool   is_cfr;           // is constant framerate?
};

} /* namespace clst */

#endif /* CLST_VIDEO_INFO_HPP */
