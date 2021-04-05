#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>

#include "doctest.h"
#include "imgui.h"

#include "Player.hpp"
#include "Sequence.hpp"
#include "ImageCollection.hpp"
#include "globals.hpp"
#include "events.hpp"
#include "strutils.hpp"

Player::Player() {
    static int id = 0;
    id++;
    ID = "Player " + std::to_string(id);

    frame = 1;
    minFrame = 1;
    maxFrame = std::numeric_limits<int>::max();
    currentMinFrame = 1;
    currentMaxFrame = maxFrame;
    opened = false;

    fps = gDefaultFramerate;
    frameClock = 0;
    frameAccumulator = 0.;
}

void Player::update()
{
    frameAccumulator += letTimeFlow(&frameClock);

    if (!bouncy) {
        direction = 1;
    }

    if (playing) {
        while (frameAccumulator > 1000. / std::abs(fps)) {
            int d = (fps >= 0 ? 1 : -1) * direction;
            frame += d;
            frameAccumulator -= 1000. / std::abs(fps);
            if (bouncy) {
                if (frame < currentMinFrame) {
                    frame = currentMinFrame + 1;
                    direction *= -1;
                }
                if (frame > currentMaxFrame) {
                    frame = currentMaxFrame - 1;
                    direction *= -1;
                }
            }
            checkBounds();
        }
    } else {
        frameAccumulator = 0.;
    }

    int index = std::find(gPlayers.begin(), gPlayers.end(), this) - gPlayers.begin();
    char d[2] = {static_cast<char>('1' + index), 0};
    bool isKeyFocused = index <= 9 && isKeyPressed(d) && isKeyDown("alt");

    if (isKeyFocused)
        opened = true;

    if (!opened) {
        return;
    }

    if (!ImGui::Begin(ID.c_str(), &opened, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }
    ImGui::BringFront();

    if (isKeyFocused) {
        ImGui::SetWindowFocus();
    }

    if (ImGui::IsWindowFocused()) {
        checkShortcuts();
    }

    displaySettings();

    checkBounds();

    ImGui::End();
}

void Player::displaySettings()
{
    if (ImGui::Button("<")) {
        frame--;
        playing = false;
    }
    ImGui::SameLine(); ImGui::ShowHelpMarker("Previous frame (left)");
    ImGui::SameLine();
    ImGui::Checkbox("Play", &playing);
    ImGui::SameLine(); ImGui::ShowHelpMarker("Toggle playback (p)");
    ImGui::SameLine();
    if (ImGui::Button(">")) {
        frame++;
        playing = false;
    }
    ImGui::SameLine(); ImGui::ShowHelpMarker("Next frame (right)");
    ImGui::Checkbox("Looping", &looping);
    ImGui::SameLine(); ImGui::ShowHelpMarker("Loops when at the end of the sequence");
    ImGui::SameLine(); ImGui::Checkbox("Bouncy", &bouncy);
    ImGui::SameLine(); ImGui::ShowHelpMarker("Bounce back and forth instead of circular playback");
    if (ImGui::SliderInt("Frame", &frame, currentMinFrame, currentMaxFrame)) {
        playing = false;
    }
    ImGui::SliderFloat("FPS", &fps, -100.f, 100.f, "%.2f frames/s");
    ImGui::SameLine(); ImGui::ShowHelpMarker("Change the Frame Per Second rate");
    ImGui::DragIntRange2("Bounds", &currentMinFrame, &currentMaxFrame, 1.f, minFrame, maxFrame);
    ImGui::SameLine(); ImGui::ShowHelpMarker("Change the bounds of the playback");
}

void Player::checkShortcuts()
{
    if (ImGui::GetIO().WantCaptureKeyboard
        || isKeyDown("control") || isKeyDown("alt") || isKeyDown("shift")) {
        return;
    }
    if (isKeyPressed("p", false)) {
        playing = !playing;
    }
    if (isKeyPressed("left")) {
        frame--;
        checkBounds();
    }
    if (isKeyPressed("right")) {
        frame++;
        checkBounds();
    }
    if (isKeyPressed("F8")) {
        fps -= 1;
    }
    if (isKeyPressed("F9")) {
        fps += 1;
    }
}

void Player::checkBounds()
{
    currentMaxFrame = std::min(currentMaxFrame, maxFrame);
    currentMinFrame = std::max(currentMinFrame, minFrame);
    currentMaxFrame = std::max(currentMaxFrame, currentMinFrame);
    currentMinFrame = std::min(currentMinFrame, currentMaxFrame);

    if (frame > currentMaxFrame) {
        if (looping) {
            frame = currentMinFrame;
        } else {
            frame = currentMaxFrame;
        }
    }
    if (frame < currentMinFrame) {
        if (looping) {
            frame = currentMaxFrame;
        } else {
            frame = currentMinFrame;
        }
    }
}

void Player::reconfigureBounds()
{
    minFrame = 1;
    maxFrame = 0;
    currentMinFrame = minFrame;
    currentMaxFrame = std::numeric_limits<int>::max();

    for (auto seq : gSequences) {
        if (seq->player == this && seq->collection) {
            int len = seq->collection->getLength();
            maxFrame = std::max(maxFrame, len);
        }
    }

    checkBounds();
}

bool Player::parseArg(const std::string& arg)
{
    if (startswith(arg, "p:fps:")) {
        float new_fps = gDefaultFramerate;
        if (sscanf(arg.c_str(), "p:fps:%f", &new_fps) == 1) {
            fps = new_fps;
            return true;
        }
    } else if (arg == "p:play") {
        playing = true;
        return true;
    } else if (startswith(arg, "p:looping")) {
        int new_looping;
        if (sscanf(arg.c_str(), "p:looping:%d", &new_looping) == 1) {
            looping = new_looping;
            return true;
        }
    }
    return false;
}

TEST_CASE("Player::parseArg") {
    Player p;

    SUBCASE("p:fps:") {
        CHECK(p.fps == gDefaultFramerate);
        CHECK(p.parseArg("p:fps:120.5"));
        CHECK(p.fps == doctest::Approx(120.5f));
        CHECK(p.parseArg("p:fps:-10"));
        CHECK(p.fps == doctest::Approx(-10.f));
    }

    SUBCASE("p:play") {
        CHECK(!p.playing);
        CHECK(p.parseArg("p:play"));
        CHECK(p.playing);
    }

    SUBCASE("p:looping") {
        CHECK(p.looping);
        CHECK(p.parseArg("p:looping:0"));
        CHECK(!p.looping);
        CHECK(p.parseArg("p:looping:1"));
        CHECK(p.looping);
    }
}


