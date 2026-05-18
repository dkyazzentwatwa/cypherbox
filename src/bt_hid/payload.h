// payload.h - DuckyScript-subset payload runner.
// Vendored from ESP32_BT_HID/payload.h

#pragma once

#include <string>

namespace payload {

// Run a DuckyScript-subset payload synchronously. Returns true on success.
//
// Supported instructions (one per line):
//   REM <comment>
//   STRING <text>          - types text with KEY_INTERCHAR_MS spacing
//   STRINGLN <text>        - same + ENTER
//   ENTER / TAB / SPACE / ESC / ESCAPE / BACKSPACE / DELETE / INSERT
//   HOME / END / PAGEUP / PAGEDOWN
//   UP / DOWN / LEFT / RIGHT
//   F1..F12
//   DELAY <ms>
//   GUI/WIN/WINDOWS/CMD [key]
//   CTRL/CONTROL [key]
//   ALT [key]
//   SHIFT [key]
//   <combo>                - e.g. "CTRL ALT DELETE", "GUI r"
//   REPEAT <n>             - repeat the previous executed line n more times
bool run(const std::string& script);

}  // namespace payload
