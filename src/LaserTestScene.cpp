// Copyright (c) 2023 - Barton Dring
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include <string>
#include "Scene.h"
#include "ConfirmScene.h"
#include "e4math.h"

class LaserTestScene : public Scene {
private:
    int  selection   = 0;
    long oldPosition = 0;

    // Saved to NVS
    // e4_t _offset  = e4_from_int(0);
    uint16_t power_max     = 1000;
    uint32_t test_duration = 50;
    uint8_t  power_percent = 20;
    uint16_t feedrate      = 500;

    bool testStarted = false;

public:
    LaserTestScene() : Scene("Laser") {}

    void onDialButtonPress() { pop_scene(); }

    void onGreenButtonPress() {
        switch (state) {
            case Idle:
                confirm_cutting_test();
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
                ackBeep();
                testStarted = true;
                reDisplay();

                send_line("G1 F1");
                send_linef("M3 S%d", power_max / 100 * power_percent);
                send_linef("G4 P%d.%03d", test_duration / 1000, test_duration % 1000);
                send_line("M5 S0");

                testStarted = false;
                reDisplay();
                // ackBeep();
                break;
            case Hold:
            case DoorClosed:
                fnc_realtime(Reset);
                break;
        };
    }

    void onTouchClick() {
        // Rotate through the items to be adjusted.
        rotateNumberLoop(selection, 1, 0, 3);
        reDisplay();
        ackBeep();
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
                power_percent += delta;
                if (power_percent > 100) {
                    power_percent = 100;
                } else if (power_percent < 0) {
                    power_percent = 0;
                }
                break;
            case 2:
                test_duration += delta * 20;
                if (test_duration > 2000) {
                    test_duration = 2000;
                } else if (test_duration < 50) {
                    test_duration = 50;
                }
                break;
            case 3:
                feedrate += delta * 20;
                if (feedrate > 2000) {
                    feedrate = 2000;
                } else if (feedrate < 40) {
                    feedrate = 40;
                }
                break;
            default:
                break;
        }

        reDisplay();
    }

    void onEntry(void* arg) override {
        if (arg && strcmp((const char*)arg, "Confirmed") == 0) {
            // send_line("$Macros/Run=1");

            send_line("G21");
            send_line("G91");
            send_linef("M3 S%d", power_max / 100 * power_percent);
            send_linef("G1 X10 F%d", feedrate);
            send_line("G1 Y10");
            send_line("G1 X-10");
            send_line("G1 Y-10");
            send_line("M5");
            send_line("G90");
        }

        // if (initPrefs()) {
        //     getPref("PowerMax", &power_max);
        //     getPref("TestDuration", &test_duration);
        //     getPref("PowerPercent", &power_percent);
        // }

        switch (state) {
            case Idle:
                testStarted = false;
                reDisplay();
                break;
        }
    }

    void reDisplay() {
        background();
        drawMenuTitle(current_scene->name());
        drawStatus();

        const char* grnLabel = "";
        const char* redLabel = "";

        if (state == Idle) {
            int x      = 16;
            int y      = 62;
            int width  = display_short_side() - (x * 2);
            int height = 28;

            Stripe button(x, y, width, height, TINY);

            button.draw("Power Max", intToCStr(power_max), selection == 0);
            button.draw("Power, %", intToCStr(power_percent), selection == 1);
            button.draw("Duration, ms", intToCStr(test_duration), selection == 2);
            button.draw("Feedrate", intToCStr(feedrate), selection == 3);

            LED led(x + width / 2, y + height * 5 - 5, 13, button.gap());
            led.draw(testStarted, RED);

            grnLabel = "Cut";
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

    void confirm_cutting_test() {
        ackBeep();
        std::string confirmMsg("Test cutting?");
        dbg_println(confirmMsg.c_str());
        push_scene(&confirmScene, (void*)confirmMsg.c_str());
    }
};
LaserTestScene laserTestScene;
