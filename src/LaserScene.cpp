// Copyright (c) 2023 - Barton Dring
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include <string>
#include "Scene.h"
#include "e4math.h"

class LaserScene : public Scene {
private:
    int  selection   = 0;
    long oldPosition = 0;

    // Saved to NVS
    // e4_t _offset  = e4_from_int(0);
    uint16_t power_max     = 1000;
    uint32_t test_duration = 50;
    uint8_t  power_percent = 20;

public:
    LaserScene() : Scene("Laser") {}

    void onDialButtonPress() { pop_scene(); }

    void onGreenButtonPress() {
        // G38.2 G91 F80 Z-20 P8.00
        switch (state) {
            case Idle:
                // TODO :: Make a GCode command for laser testing and use it here.  Maybe something like:
                // G38.2 G91 F1000 S20 P50 for a 20% power test for 50ms.  We can use the existing feedrate and dwell parameters for the laser test.

                // send_linef("G38.2G91F%d%c%dP%s", test_duration, axisNumToChar(_axis), power_max, e4_to_cstr(_offset, 2));
                send_linef("M5S0");
                break;
            case Cycle:
                fnc_realtime(FeedHold);
                break;
            case Hold:
            case DoorClosed:
                fnc_realtime(CycleStart);
                break;
            case Alarm:
                send_line("$X");  // unlock
                break;
        }
    }

    void onRedButtonPress() {
        // G38.2 G91 F80 Z-20 P8.00
        switch (state) {
            case Cycle:
            case Alarm:
                fnc_realtime(Reset);
                break;
            case Idle:
                send_linef("M5S0");
                break;
            case Hold:
            case DoorClosed:
                fnc_realtime(Reset);
                break;
        };
    }

    void onTouchClick() {
        // Rotate through the items to be adjusted.
        rotateNumberLoop(selection, 1, 0, 2);
        reDisplay();
        // ackBeep();
    }

    void onDROChange() { reDisplay(); }

    void onEncoder(int delta) {
        if (!abs(delta)) {
            return;
        }

        switch (selection) {
            case 0:
                power_max += delta * 10;  // Increment by 10
                if (power_max > 1000) {
                    power_max = 1000;
                } else if (power_max < 0) {
                    power_max = 0;
                }
                break;
            case 1:
                test_duration += delta * 50;
                if (test_duration > 2000) {
                    test_duration = 2000;
                } else if (test_duration < 50) {
                    test_duration = 50;
                }
                break;
            case 2:
                power_percent += delta * 2;
                if (power_percent > 100) {
                    power_percent = 100;
                } else if (power_percent < 0) {
                    power_percent = 0;
                }
                break;
            default:
                break;
        }

        reDisplay();
    }
    void onEntry(void* arg) override {
        // if (initPrefs()) {
        //     getPref("PowerMax", &power_max);
        //     getPref("TestDuration", &test_duration);
        //     getPref("PowerPercent", &power_percent);
        // }
    }

    void reDisplay() {
        background();
        drawMenuTitle(current_scene->name());
        drawStatus();

        const char* grnLabel = "";
        const char* redLabel = "";

        if (state == Idle) {
            int    x      = 40;
            int    y      = 62;
            int    width  = display_short_side() - (x * 2);
            int    height = 25;
            int    pitch  = 27;  // for spacing of buttons
            Stripe button(x, y, width, height, TINY);
            button.draw("Power Max", intToCStr(power_max), selection == 0);
            y = button.y();  // For LED
            button.draw("Duration", intToCStr(test_duration), selection == 1);
            button.draw("Power %", intToCStr(power_percent), selection == 2);

            //LED led(x - 20, y + height / 2, 10, button.gap());
            //led.draw(myProbeSwitch);

            grnLabel = "*";
            redLabel = "Test";
        } else {
            if (state == Jog || state == Alarm) {  // there is no Probing state, so Cycle is a valid state on this
                //centered_text("Invalid State", 105, WHITE, MEDIUM);
                //centered_text("For Probing", 145, WHITE, MEDIUM);
                redLabel = "Stop";
                grnLabel = "Unlock";
            } else {
                // int x      = 14;
                // int height = 35;
                // int y      = 82 - height / 2;

                // LED led(120, 190, 10, 5);
                // led.draw(myProbeSwitch);

                // int width = display_short_side() - x * 2;
                // DRO dro(x, y, width, height);
                // dro.draw(0, _axis == 0);
                // dro.draw(1, _axis == 1);
                // dro.draw(2, _axis == 2);

                switch (state) {
                    case Cycle:
                        redLabel = "E-Stop";
                        grnLabel = "Hold";
                        break;
                    case Hold:
                    case DoorClosed:
                        redLabel = "Reset";
                        grnLabel = "Resume";
                        break;
                }
            }
        }

        drawButtonLegends(redLabel, grnLabel, "Back");
        drawError();  // only if one just happened
        refreshDisplay();
    }
};
LaserScene laserScene;
