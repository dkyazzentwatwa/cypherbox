// terminal.h - Serial Terminal/CLI Interface for Cypherbox V2
// Command-line control via serial port

#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include "../types.h"
#include "../config.h"

struct CommandMapping {
    const char* command;
    MenuItem menuItem;
    const char* description;
};

class Terminal {
public:
    static void init();
    static void processInput();
    static bool hasCommand();
    static MenuItem getCommand();
    static void clearCommand();
    static bool isCommandFromSerial() { return commandFromSerial; }
    static String getCommandLine() { return currentCommandLine; }
    static String getCommandName();
    static String getArg(uint8_t index);
    static String getArgsAfter(uint8_t index);
    static bool stopRequested();
    static void clearStopFlag();
    static void echoToSerial(const String& line1, const String& line2,
                             const String& line3, const String& line4);
    static void setEchoEnabled(bool enabled) { echoEnabled = enabled; }
    static bool isEchoEnabled() { return echoEnabled; }
    static void setVerbose(bool enabled) { verboseLogging = enabled; }
    static bool isVerbose() { return verboseLogging; }

private:
    static void parseCommand(const String& cmd);
    static MenuItem commandToMenuItem(const String& cmd);
    static void printHelp();
    static void printStatus();
    static void printMenu();

    static char lineBuffer[BUF_LENGTH];
    static uint8_t bufferIndex;
    static bool commandReady;
    static bool commandFromSerial;
    static String currentCommandLine;
    static MenuItem pendingCommand;
    static bool stopFlag;
    static bool echoEnabled;
    static bool verboseLogging;
    static bool initialized;
};

#endif // TERMINAL_H
