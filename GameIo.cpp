#include "GameIo.h"

#include <iostream>

std::streambuf* GameIo::savedCoutBuf_ = nullptr;
std::streambuf* GameIo::savedCinBuf_ = nullptr;
GameIo::OutBuffer* GameIo::outBuf_ = nullptr;
GameIo::InBuffer* GameIo::inBuf_ = nullptr;
bool GameIo::active_ = false;

GameIo::OutBuffer::OutBuffer(OutputCallback callback) : callback_(std::move(callback)) {}

int GameIo::OutBuffer::overflow(int ch) {
    if (ch == traits_type::eof()) {
        return traits_type::not_eof(ch);
    }
    if (callback_) {
        callback_(std::string(1, static_cast<char>(ch)));
    }
    return ch;
}

std::streamsize GameIo::OutBuffer::xsputn(const char* s, std::streamsize count) {
    if (callback_ && count > 0) {
        callback_(std::string(s, static_cast<std::size_t>(count)));
    }
    return count;
}

int GameIo::OutBuffer::sync() {
    return 0;
}

GameIo::InBuffer::InBuffer(InputProvider provider) : provider_(std::move(provider)) {}

int GameIo::InBuffer::underflow() {
    if (readPos_ >= buffer_.size()) {
        buffer_.clear();
        readPos_ = 0;

        if (!provider_) {
            return traits_type::eof();
        }

        std::string line = provider_(currentPrompt_);
        currentPrompt_.clear();
        if (!line.empty() && line.back() != '\n') {
            line.push_back('\n');
        }
        buffer_ = std::move(line);
    }

    if (readPos_ >= buffer_.size()) {
        return traits_type::eof();
    }
    return static_cast<unsigned char>(buffer_[readPos_]);
}

int GameIo::InBuffer::uflow() {
    const int ch = underflow();
    if (ch == traits_type::eof()) {
        return traits_type::eof();
    }
    return static_cast<unsigned char>(buffer_[readPos_++]);
}

std::streamsize GameIo::InBuffer::xsgetn(char* s, std::streamsize count) {
    if (count <= 0 || !s) {
        return 0;
    }

    std::streamsize copied = 0;
    while (copied < count) {
        if (readPos_ >= buffer_.size()) {
            buffer_.clear();
            readPos_ = 0;

            if (!provider_) {
                return copied > 0 ? copied : -1;
            }

            std::string line = provider_(currentPrompt_);
            currentPrompt_.clear();
            if (!line.empty() && line.back() != '\n') {
                line.push_back('\n');
            }
            buffer_ = std::move(line);
            if (buffer_.empty()) {
                return copied > 0 ? copied : -1;
            }
        }

        while (readPos_ < buffer_.size() && copied < count) {
            s[copied++] = buffer_[readPos_++];
        }
    }

    return copied;
}

void GameIo::installStreams(OutputCallback onOutput, InputProvider onInput) {
    shutdown();

    std::ios::sync_with_stdio(false);

    savedCoutBuf_ = std::cout.rdbuf();
    savedCinBuf_ = std::cin.rdbuf();

    outBuf_ = new OutBuffer(std::move(onOutput));
    inBuf_ = new InBuffer(std::move(onInput));

    std::cout.rdbuf(outBuf_);
    std::cin.rdbuf(inBuf_);
    std::cin.tie(&std::cout);
    std::cout.clear();
    std::cin.clear();
    active_ = true;
}

void GameIo::flushOutput() {
    if (active_) {
        std::cout.flush();
    }
}

void GameIo::restoreStreams() {
    if (!active_) {
        return;
    }
    if (savedCoutBuf_) {
        std::cout.rdbuf(savedCoutBuf_);
        savedCoutBuf_ = nullptr;
    }
    if (savedCinBuf_) {
        std::cin.rdbuf(savedCinBuf_);
        savedCinBuf_ = nullptr;
    }
    delete outBuf_;
    outBuf_ = nullptr;
    delete inBuf_;
    inBuf_ = nullptr;
    active_ = false;
}

void GameIo::installGui(OutputCallback onOutput, InputProvider onInput) {
    installStreams(std::move(onOutput), std::move(onInput));
}

void GameIo::shutdown() {
    restoreStreams();
}
