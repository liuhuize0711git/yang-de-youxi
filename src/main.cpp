#include "Core.hpp"
#include "Renderer.hpp"
#include "Audio.hpp"
#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <iostream>
#include <optional>

using namespace lumen;
namespace {
struct Options {
    bool silent{},hidden{};
    int frames{};
    std::string capture,scene;
};
Options options(int argc,char** argv) {
    Options o;
    for(int i=1;i<argc;++i) {
        std::string a=argv[i];
        if(a=="--no-audio") o.silent=true;
        else if(a=="--hidden") o.hidden=true;
        else if(a=="--frames" && i+1<argc) o.frames=std::max(1,std::atoi(argv[++i]));
        else if(a=="--capture" && i+1<argc) o.capture=argv[++i];
        else if(a=="--scene" && i+1<argc) o.scene=argv[++i];
        else if(a=="--help") {
            std::cout<<"Lumenfall 1.0.1: C++17 / SFML 3.x\n"
                <<"Options: --no-audio --frames N --capture image.png --hidden\n"
                <<"         --scene title|hub|garden|boss|map|ending\n"
                <<"Capture and scene modes never write to your save file.\n";
            std::exit(0);
        } else {std::cerr<<"Unknown option: "<<a<<'\n'; std::exit(2);}
    }
    if(!o.capture.empty() && o.frames==0) o.frames=1;
    return o;
}
bool held(sf::Keyboard::Key k) {return sf::Keyboard::isKeyPressed(k);}
}

int main(int argc,char** argv) {
    try {
        const Options opt=options(argc,argv);
        const bool inspection=!opt.scene.empty()||!opt.capture.empty();
        sf::RenderWindow window(sf::VideoMode({1280,720}),"LUMENFALL 1.0.1 - The Hollow Bell",sf::Style::Default);
        window.setKeyRepeatEnabled(false);window.setVerticalSyncEnabled(true);
        if(opt.hidden) window.setVisible(false);
        sf::RenderTexture canvas({640,360});canvas.setSmooth(false);
        Renderer renderer;
        Audio audio(!opt.silent);
        Game game;game.newGame();game.saveRequested=false;
        Progress saved;std::string saveError;
        const auto savePath=defaultSavePath();
        bool hasSave=loadProgress(savePath,saved,saveError);
        if(!hasSave) {
            auto backup=savePath;backup+=".bak";
            hasSave=loadProgress(backup,saved,saveError);
        }
        Screen screen=Screen::Title,returnScreen=Screen::Title,mapReturn=Screen::Play;
        int selection=hasSave?0:1;bool muted=opt.silent,started=false;
        if(opt.scene=="hub"||opt.scene=="garden"||opt.scene=="boss"||opt.scene=="map"||opt.scene=="ending") {
            game.progress.abilities=Dash|Wings|Bell;game.progress.visited=255;game.progress.shards=43;
            game.player.hp=game.progress.maxHealth();
            if(opt.scene=="hub") game.enter(1,{450,632});
            if(opt.scene=="garden") game.enter(3,{470,222});
            if(opt.scene=="boss") {
                game.enter(7,{340,452});game.boss.active=true;game.boss.state=BossState::DashTell;
                game.boss.timer=.6f;game.boss.duration=.95f;game.boss.phase=2;game.boss.hp=19;
                game.roomTitleTime=0;game.noticeTime=0;
            }
            if(opt.scene=="map") game.enter(1,{450,632});
            if(opt.scene=="ending") {game.enter(7,{470,452});game.progress.bossDefeated=true;}
            screen=opt.scene=="map"?Screen::Map:opt.scene=="ending"?Screen::Ending:Screen::Play;
            game.noticeTime=0;game.saveRequested=false;
        }
        renderer.snapCamera(game);
        auto persist=[&]() {
            if(!started || inspection) {game.saveRequested=false;return;}
            if(saveProgress(savePath,game.progress,saveError)) {saved=game.progress;hasSave=true;}
            else {game.message("SAVE FAILED - CHECK THE SAVE FOLDER",5);std::cerr<<"Save failed: "<<saveError<<'\n';}
            game.saveRequested=false;
        };
        auto startNew=[&]() {
            game.newGame();renderer.snapCamera(game);screen=Screen::Play;started=true;persist();
        };
        sf::Clock frameClock;
        float accumulator=0;
        Input edges;
        unsigned frame=0;
        while(window.isOpen()) {
            float elapsed=std::min(.1f,frameClock.restart().asSeconds());
            if(opt.frames>0) elapsed=1.f/60.f;
            while(const auto event=window.pollEvent()) {
                if(event->is<sf::Event::Closed>()) {persist();window.close();break;}
                if(event->is<sf::Event::FocusLost>() && screen==Screen::Play && !inspection) {
                    screen=Screen::Pause;selection=0;edges={};accumulator=0;
                }
                const auto* key=event->getIf<sf::Event::KeyPressed>();
                if(!key) continue;
                const auto k=key->code;
                using K=sf::Keyboard::Key;
                if(k==K::F) {muted=!muted;audio.setMuted(muted);}
                if(screen==Screen::Play) {
                    if(k==K::Escape||k==K::P) {screen=Screen::Pause;selection=0;edges={};accumulator=0;}
                    else if(k==K::M||k==K::Tab) {screen=Screen::Map;mapReturn=Screen::Play;edges={};accumulator=0;}
                    else if(k==K::Space||k==K::Z) edges.jump=true;
                    else if(k==K::J||k==K::X) edges.attack=true;
                    else if(k==K::K||k==K::LShift||k==K::RShift) edges.dash=true;
                    else if(k==K::E||k==K::W) edges.interact=true;
                } else if(screen==Screen::Title) {
                    if(k==K::Down||k==K::S) selection=(selection+1)%4;
                    if(k==K::Up||k==K::W) selection=(selection+3)%4;
                    if(k==K::Enter||k==K::Space) {
                        if(selection==0 && hasSave) {game.restore(saved);renderer.snapCamera(game);screen=Screen::Play;started=true;}
                        if(selection==1) {if(hasSave) screen=Screen::ConfirmNew;else startNew();}
                        if(selection==2) {returnScreen=Screen::Title;screen=Screen::Controls;}
                        if(selection==3) window.close();
                    }
                } else if(screen==Screen::ConfirmNew) {
                    if(k==K::Enter) startNew();
                    if(k==K::Escape) screen=Screen::Title;
                } else if(screen==Screen::Pause) {
                    if(k==K::Down||k==K::S) selection=(selection+1)%5;
                    if(k==K::Up||k==K::W) selection=(selection+4)%5;
                    if(k==K::Escape||k==K::P) screen=Screen::Play;
                    if(k==K::Enter||k==K::Space) {
                        if(selection==0) screen=Screen::Play;
                        if(selection==1) {mapReturn=Screen::Pause;screen=Screen::Map;}
                        if(selection==2) {returnScreen=Screen::Pause;screen=Screen::Controls;}
                        if(selection==3) {persist();screen=Screen::Title;selection=hasSave?0:1;}
                        if(selection==4) {persist();window.close();}
                    }
                } else if(screen==Screen::Map) {
                    if(k==K::Escape||k==K::M||k==K::Tab) screen=mapReturn;
                } else if(screen==Screen::Controls) {
                    if(k==K::Escape||k==K::Enter) screen=returnScreen;
                } else if(screen==Screen::Ending) {
                    if(k==K::Enter) {screen=Screen::Title;selection=0;}
                }
            }
            if(!window.isOpen()) break;
            if(screen==Screen::Play) {
                accumulator+=elapsed;
                while(accumulator>=Tick) {
                    Input in=edges;
                    if(!opt.hidden) {
                        using K=sf::Keyboard::Key;
                        in.move=float(held(K::D)||held(K::Right))-float(held(K::A)||held(K::Left));
                        in.jumpHeld=held(K::Space)||held(K::Z);in.up=held(K::Up);in.heal=held(K::H);
                    }
                    game.step(in);edges={};accumulator-=Tick;
                    for(auto c:game.cues) {renderer.feed(c);audio.play(c.effect);}game.cues.clear();
                    if(game.saveRequested) persist();
                    if(game.ending) {persist();screen=Screen::Ending;break;}
                }
            } else {accumulator=0;edges={};}
            audio.setBattle(screen==Screen::Play&&game.room==7&&game.boss.active);
            renderer.update(game,elapsed);
            canvas.clear();renderer.draw(canvas,game,screen,selection,hasSave,muted);canvas.display();
            window.clear({3,8,13});
            const auto size=window.getSize();
            const float scale=std::min(size.x/640.f,size.y/360.f);
            sf::Sprite present(canvas.getTexture());present.setScale({scale,scale});
            present.setPosition({(size.x-640*scale)*.5f,(size.y-360*scale)*.5f});
            // Explicit pixel-coordinate view also keeps resizing / letterboxing correct.
            if(size.x>0&&size.y>0) window.setView(sf::View(sf::FloatRect({0,0},{float(size.x),float(size.y)})));
            window.draw(present);window.display();
            ++frame;
            if(opt.frames>0 && frame>=unsigned(opt.frames)) {
                if(!opt.capture.empty() && !canvas.getTexture().copyToImage().saveToFile(opt.capture)) {
                    std::cerr<<"Failed to write capture\n";return 3;
                }
                window.close();
            }
        }
        return 0;
    } catch(const std::exception& e) {std::cerr<<"Lumenfall: "<<e.what()<<'\n';return 1;}
}
