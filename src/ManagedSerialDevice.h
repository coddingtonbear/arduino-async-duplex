#pragma once

#include <functional>

#include <Arduino.h>
#undef min
#undef max
#include <Regexp.h>

#define COMMAND_QUEUE_SIZE 5
#define INPUT_BUFFER_LENGTH 256
#define MAX_COMMAND_LENGTH 64
#define MAX_EXPECTATION_LENGTH 128
#define COMMAND_TIMEOUT 2500
#define MAX_HOOK_COUNT 10

//#define MANAGED_SERIAL_DEVICE_DEBUG
//#define MANAGED_SERIAL_DEVICE_DEBUG_VERBOSE
//#define MANAGED_SERIAL_DEVICE_DEBUG_COUT
//#define MANAGED_SERIAL_DEVICE_DEBUG_STREAM

class ManagedSerialDevice: public Stream {
    public:
        enum Timing{
            NEXT,
            ANY
        };
        struct Command {
            char command[MAX_COMMAND_LENGTH];
            char expectation[MAX_EXPECTATION_LENGTH];
            std::function<void(MatchState)> success;
            std::function<void(Command*)> failure;
            uint16_t timeout;
            uint32_t delay;

            Command();
            Command(
                const char* _cmd,
                const char* _expect,
                std::function<void(MatchState)> _success = NULL,
                std::function<void(Command*)> _failure = NULL,
                uint16_t _timeout = COMMAND_TIMEOUT,
                uint32_t _delay = 0
            );
        };
        struct Hook {
            char expectation[MAX_EXPECTATION_LENGTH];
            std::function<void(MatchState)> success;

            Hook();
            Hook(
                const char* _expect,
                std::function<void(MatchState)> _success
            );
        };

        ManagedSerialDevice();

        bool begin(Stream*, Stream* _errorStream=NULL);
        bool wait(uint32_t timeout, std::function<void()> _feed_watchdog=NULL);
        bool abort();
        bool execute(
            const char *_command,
            const char *_expectation,
            Timing _timing,
            std::function<void(MatchState)> _success = NULL,
            std::function<void(Command*)> _failure = NULL,
            uint16_t _timeout = COMMAND_TIMEOUT,
            uint32_t _delay = 0
        );
        bool execute(
            const char *_command,
            const char *_expectation = "",
            std::function<void(MatchState)> _success = NULL,
            std::function<void(Command*)> _failure = NULL,
            uint16_t _timeout = COMMAND_TIMEOUT,
            uint32_t _delay = 0
        );
        bool execute(
            const Command*,
            Timing _timing = Timing::ANY
        );
        bool executeChain(
            const Command*,
            uint16_t count,
            Timing _timing,
            std::function<void(MatchState)> _success = NULL,
            std::function<void(Command*)> _failure = NULL
        );
        bool executeChain(
            const Command*,
            uint16_t count,
            std::function<void(MatchState)> _success = NULL,
            std::function<void(Command*)> _failure = NULL
        );

        bool registerHook(
            const char *_expectation,
            std::function<void(MatchState)> _success
        );

        void loop();

        uint8_t getQueueLength();
        void getResponse(char*, uint16_t);

        // Helper functions
        std::function<void(Command*)> printFailure(Stream*);
        void stripMatchFromInputBuffer(MatchState ms);

        // Stream
        int available();
        size_t write(uint8_t);
        int read();
        int peek();
        void flush();
    protected:
        Command commandQueue[COMMAND_QUEUE_SIZE];
        uint8_t queueLength = 0;

        void shiftRight();
        void shiftLeft();

        void getLatestLine(char*, uint16_t length);
        virtual void newLineReceived();
        virtual void commandSent(char*);

        void clearInputBuffer();
        void copyCommand(Command*, const Command*);
        void createChain(Command*, const Command*);
        void prependCallback(
            Command*,
            std::function<void(MatchState)> _success = NULL,
            std::function<void(Command*)> _failure = NULL
        );

        uint16_t nextLogLineStart = 0;

        char inputBuffer[INPUT_BUFFER_LENGTH];
        uint16_t bufferPos = 0;
        uint32_t timeout = 0;

        bool began = false;
        bool processing = false;

        Hook hooks[MAX_HOOK_COUNT];
        uint8_t hookCount = 0;
        virtual void runHooks();

        virtual void emitErrorMessage(const char*);
        #ifdef MANAGED_SERIAL_DEVICE_DEBUG
            virtual void debugMessage(String);
            virtual void debugMessage(const char*);
        #endif
        Stream* errorStream;
        Stream* stream;
};
