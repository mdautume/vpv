#pragma once

#include <string>
#include <vector>
#include <map>

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include "Texture.hpp"

struct View;
struct Player;
struct Colormap;
struct Image;

struct Sequence {
    std::string ID;
    std::string glob;
    std::string glob_;
    std::vector<std::string> filenames;
    bool valid;
    bool force_reupload;

    int loadedFrame;
    ImRect loadedRect;

    Texture texture;
    View* view;
    Player* player;
    Colormap* colormap;
    const Image* image;

    Sequence();

    void loadFilenames();

    void loadTextureIfNeeded();

    void autoScaleAndBias();

    const Image* getCurrentImage();
};

