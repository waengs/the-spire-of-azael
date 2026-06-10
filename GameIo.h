#ifndef GAMEIO_H
#define GAMEIO_H

#include <functional>
#include <ios>
#include <iostream>
#include <streambuf>
#include <string>

class GameIo {
public:
    using OutputCallback = std::function<void(const std::string& chunk)>;
    using InputProvider = std::function<std::string(const std::string& prompt)>;

    static void installGui(OutputCallback onOutput, InputProvider onInput);
    static void flushOutput();
    static void shutdown();

private:
    class OutBuffer : public std::streambuf {
    public:
        explicit OutBuffer(OutputCallback callback);
    protected:
        int overflow(int ch) override;
        std::streamsize xsputn(const char* s, std::streamsize count) override;
        int sync() override;
    private:
        OutputCallback callback_;
    };

    class InBuffer : public std::streambuf {
    public:
        explicit InBuffer(InputProvider provider);
    protected:
        int underflow() override;
        int uflow() override;
        std::streamsize xsgetn(char* s, std::streamsize count) override;
    private:
        InputProvider provider_;
        std::string buffer_;
        std::size_t readPos_ = 0;
        std::string currentPrompt_;
    };

    static void installStreams(OutputCallback onOutput, InputProvider onInput);
    static void restoreStreams();

    static std::streambuf* savedCoutBuf_;
    static std::streambuf* savedCinBuf_;
    static OutBuffer* outBuf_;
    static InBuffer* inBuf_;
    static bool active_;
};

#endif
