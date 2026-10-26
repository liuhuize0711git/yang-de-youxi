#include "Audio.hpp"
#ifdef LUMEN_AUDIO
#include <SFML/Audio.hpp>
#include <array>
#include <cmath>
#include <random>
#include <stdexcept>

namespace lumen {
struct Audio::Impl {
    std::array<sf::SoundBuffer,11> effects;
    std::vector<std::unique_ptr<sf::Sound>> voices;
    sf::SoundBuffer ambientBuffer,battleBuffer;
    std::unique_ptr<sf::Sound> ambient,battle;
    bool muted{}, fighting{};
    std::size_t next{};
    static constexpr int Rate=22050;
    static void fill(sf::SoundBuffer& buffer,const std::vector<std::int16_t>& samples) {
        if(!buffer.loadFromSamples(samples.data(),samples.size(),1,Rate,{sf::SoundChannel::Mono}))
            throw std::runtime_error("Unable to initialize generated audio");
    }
    Impl() {
        std::mt19937 rng(183); std::uniform_real_distribution<float> noise(-1,1);
        constexpr float tau=6.28318530718f;
        for(int id=0;id<11;++id) {
            const bool melodic=id==int(Effect::Heal)||id==int(Effect::Chime)||id==int(Effect::Victory);
            float duration=melodic?1.1f:.24f;
            if(id==int(Effect::Death)||id==int(Effect::Boom)) duration=.55f;
            std::vector<std::int16_t> data(static_cast<int>(duration*Rate));
            double phase=0;
            for(std::size_t i=0;i<data.size();++i) {
                float t=float(i)/Rate,u=t/duration;
                float freq=180.f+id*36.f;
                if(id==0) freq=240+500*u;
                if(id==1) freq=600-430*u;
                if(id==int(Effect::Death)||id==int(Effect::Boom)) freq=90-55*u;
                if(melodic) {
                    const int intervals[]={0,3,7,12};
                    freq=440*std::pow(2.f,intervals[std::min(3,int(u*4))]/12.f);
                }
                phase+=tau*freq/Rate;
                float env=std::min(1.f,t/.008f)*std::pow(1-u,2.f);
                float v=std::sin(phase)*.45f+std::sin(phase*2.002)*.15f;
                if(id==1||id==2||id==3||id==4||id==8||id==9) v=v*.5f+noise(rng)*.38f;
                data[i]=std::int16_t(v*env*17000);
            }
            fill(effects[id],data);
        }
        for(int i=0;i<14;++i) voices.push_back(std::make_unique<sf::Sound>(effects[0]));
        for(int combat=0;combat<2;++combat) {
            constexpr int length=24;
            std::vector<std::int16_t> data(Rate*length);
            const float notes[]={146.8324f,174.6141f,220.f,261.6256f,220.f,174.6141f,130.8128f,164.8138f};
            for(int i=0;i<Rate*length;++i) {
                float t=float(i)/Rate;
                // A 24-second phrase with a fade at both ends is safe to loop.
                float fade=std::min({1.f,t*.6f,(length-t)*.6f});
                float v=(std::sin(t*tau*73.4162f)+std::sin(t*tau*110.f))*.07f;
                int beat=int(t/(combat?.375f:.75f));
                float bt=std::fmod(t,combat?.375f:.75f);
                float f=notes[beat%8]*(combat?1.f:2.f);
                v+=std::sin(t*tau*f)*std::exp(-bt*5)*.11f;
                if(combat) v+=std::sin(t*tau*48)*std::exp(-bt*24)*.22f;
                data[i]=std::int16_t(v*fade*24000);
            }
            fill(combat?battleBuffer:ambientBuffer,data);
        }
        ambient=std::make_unique<sf::Sound>(ambientBuffer); ambient->setLooping(true); ambient->setVolume(35); ambient->play();
        battle=std::make_unique<sf::Sound>(battleBuffer); battle->setLooping(true); battle->setVolume(0); battle->play();
    }
};
Audio::Audio(bool enabled) { if(enabled) { try {impl=std::make_unique<Impl>();} catch(...) {} } }
Audio::~Audio()=default;
void Audio::play(Effect e) {
    if(!impl||impl->muted) return;
    auto& s=impl->voices[impl->next++%impl->voices.size()];
    s->stop(); s->setBuffer(impl->effects[std::size_t(e)]); s->setVolume(40); s->play();
}
void Audio::setMuted(bool muted) {
    if(!impl) return;
    impl->muted=muted;
    impl->ambient->setVolume(muted||impl->fighting?0.f:35.f);
    impl->battle->setVolume(muted||!impl->fighting?0.f:48.f);
    if(muted) for(auto& s:impl->voices) s->stop();
}
void Audio::setBattle(bool fighting) {
    if(!impl) return;
    impl->fighting=fighting; setMuted(impl->muted);
}
}
#else
namespace lumen {
struct Audio::Impl {};
Audio::Audio(bool) {} Audio::~Audio()=default;
void Audio::play(Effect) {} void Audio::setMuted(bool) {} void Audio::setBattle(bool) {}
}
#endif
