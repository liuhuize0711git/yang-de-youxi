#pragma once
#include "Core.hpp"
#include <memory>
namespace lumen {
class Audio {
public:
    explicit Audio(bool enabled=true);
    ~Audio();
    void play(Effect);
    void setMuted(bool);
    void setBattle(bool);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
