#include "Core.hpp"
#include <fstream>
#include <functional>
#include <chrono>
#include <iostream>
#include <stdexcept>

using namespace lumen;
namespace {
int checks=0;
void require(bool result,const std::string& reason) {
    ++checks; if(!result) throw std::runtime_error(reason);
}
void run(Game& g,int frames,Input in={}) {
    for(int i=0;i<frames;++i) {g.step(in);in.jump=in.attack=in.dash=in.interact=false;g.cues.clear();}
}
Game clean(int room=0,Vec spawn={100,452},unsigned abilities=None) {
    Game g;g.newGame();g.progress.abilities=abilities;g.enter(room,spawn);
    g.enemies.clear();g.player.invincible=100;g.hitstop=0;run(g,2);return g;
}
void walk(Game& g,float x,int maxTicks=1800) {
    for(int i=0;i<maxTicks && std::abs(g.player.pos.x-x)>3;++i) {
        Input in;in.move=float(sign(x-g.player.pos.x));g.step(in);
    }
    g.player.vel.x=0;
    require(std::abs(g.player.pos.x-x)<5,"Walk could not reach "+std::to_string(x)+" in room "+std::to_string(g.room));
}
// Integration helper: moves with real controller inputs; never teleports during a jump.
void hop(Game& g,float x,float floor,bool doubleJump=false,bool dash=false) {
    const int room=g.room; const int health=g.player.hp;
    bool landed=false;
    for(int tick=0;tick<260;++tick) {
        Input in;in.jumpHeld=true;
        in.move=std::abs(x-g.player.pos.x)>3?float(sign(x-g.player.pos.x)):0;
        in.jump=tick==0 || (doubleJump&&tick==40);
        in.dash=dash&&tick==24;
        g.step(in);g.cues.clear();
        if(g.room!=room||g.player.hp<health) break;
        if(tick>12&&g.player.grounded&&std::abs(g.player.pos.x-x)<6&&std::abs(g.player.pos.y+HeroH-floor)<2) {landed=true;break;}
    }
    if(!landed) {
        throw std::runtime_error("Jump to ("+std::to_string(x)+","+std::to_string(floor)+") failed in room "+
            std::to_string(room)+", actual=("+std::to_string(g.player.pos.x)+","+std::to_string(g.player.pos.y+HeroH)+") room="+std::to_string(g.room));
    }
    ++checks;g.player.vel.x=0;
}
void testMovement() {
    auto g=clean();Input in;in.jump=true;in.jumpHeld=true;g.step(in);
    float peak=g.player.pos.y;
    for(int i=0;i<100;++i) {in.jump=false;g.step(in);peak=std::min(peak,g.player.pos.y);}
    require(452-peak>70 && 452-peak<80,"Full jump height");
    auto shortJump=clean();Input tap;tap.jump=true;shortJump.step(tap);float low=shortJump.player.pos.y;
    for(int i=0;i<100;++i) {tap.jump=false;shortJump.step(tap);low=std::min(low,shortJump.player.pos.y);}
    require(452-low<45,"Variable jump height");
    g=clean();Input locked;locked.dash=true;g.step(locked);require(g.player.dashTime==0,"Dash is ability gated");
    g.progress.abilities=Dash;g.step(locked);require(g.player.dashTime>0,"Unlocked dash starts");
    g.player.invincible=0;int hp=g.player.hp;g.hurt(2,g.player.pos);require(g.player.hp==hp,"Dash invulnerability");
    g=clean();g.player.pos={100,390};g.player.vel={0,100};g.player.grounded=false;
    for(int i=0;i<100;++i) {Input b;b.jump=i==26;b.jumpHeld=true;g.step(b);}
    require(g.player.hp==5,"Buffered jump does not cause damage");
    // A buffered press shortly before landing must cause a second upward flight.
    g=clean();g.player.pos={100,447};g.player.grounded=false;g.player.coyote=0;g.player.vel.y=180;
    Input buffered;buffered.jump=true;buffered.jumpHeld=true;g.step(buffered);run(g,5,{0,false,true});
    require(g.player.vel.y<0,"Landing consumes buffered jump");
    g=clean();g.player.pos={100,420};g.player.grounded=false;g.player.coyote=.08f;
    Input late;late.jump=true;late.jumpHeld=true;g.step(late);require(g.player.vel.y<-350,"Coyote jump");
}
void testMapAndProgression() {
    auto world=makeWorld();require(world.size()==8,"Eight complete rooms");
    for(const auto& r:world) for(const auto& p:r.portals) {
        require(p.target>=0&&p.target<8,"Valid room link");
        const auto& to=world[p.target];Rect hero{p.arrival.x,p.arrival.y,HeroW,HeroH};
        for(const auto& b:to.solids) require(!hero.overlaps(b),"Portal spawn outside terrain");
        for(const auto& b:to.hazards) require(!hero.overlaps(b),"Portal spawn outside hazards");
    }
    auto g=clean(1,{1055,632});g.interact();require(g.room==1,"Bell door rejects missing key");
    g.progress.abilities=Dash|Wings|Bell;g.interact();require(g.room==6,"Bell door opens");
    g=clean(2,{1125,452},Dash);Input dash;dash.dash=true;dash.move=1;run(g,24,dash);
    require(g.progress.barriers==1,"Dash physically breaks veil");
    require(g.player.pos.x>1194,"Dash passes broken veil");
    g=clean(1,{106,348},None);g.interact();require(g.room==1,"High exit rejects missing wings");

    // Main-route geometry: physically acquire both traversal abilities in order.
    g=clean(2,{300,452});hop(g,370,432);hop(g,480,400);hop(g,575,480);
    walk(g,660);hop(g,710,426);hop(g,815,410);hop(g,965,422);
    require(g.progress.has(Dash),"Cistern dash pickup reachable without abilities");
    g.enter(4,{65,452});g.enemies.clear();run(g,2);walk(g,325);hop(g,390,420);
    walk(g,420);hop(g,680,480,false,true);hop(g,775,420);hop(g,910,358);hop(g,1090,304,false,true);
    require(g.progress.has(Wings),"Archive wings reachable using dash");
    g.enter(1,{490,632});g.enemies.clear();run(g,2);walk(g,345);hop(g,260,600);
    hop(g,400,544);hop(g,270,488);hop(g,140,376,true);
    g.interact();require(g.room==3,"Double jump unlocks actual high route");
    g.enemies.clear();run(g,2);hop(g,215,540);hop(g,345,480);hop(g,470,420);
    hop(g,325,308,true);hop(g,485,250,true);hop(g,645,310);hop(g,780,370);hop(g,900,430);hop(g,1080,490);
    walk(g,1180);g.interact();require(g.room==5,"Garden reaches foundry");
    g.enemies.clear();run(g,2);walk(g,370);hop(g,440,423);hop(g,560,360);hop(g,710,300,true);
    hop(g,850,360);hop(g,965,416);hop(g,1145,355,true);
    require(g.progress.has(Bell),"Foundry bell pickup reachable");
    g.enter(6,{65,452});g.enemies.clear();run(g,2);walk(g,240);hop(g,300,418);
    hop(g,540,418,false,true);hop(g,700,355,true);hop(g,815,480);
    g.interact();require(g.progress.checkpointRoom==6,"Pre-boss checkpoint usable");
    walk(g,1030);g.interact();require(g.room==7,"Main route reaches boss arena");
    g=clean(3,{720,652},Dash|Wings);hop(g,850,580,true);hop(g,1010,490,true);
    require(g.room==3,"Lower garden secret has an escape route");
}
void testCombatAndRecovery() {
    auto g=clean(0,{800,452});g.player.invincible=0;
    Enemy e;e.pos={834,458};e.left=834;e.right=834;e.hp=3;g.enemies={e};
    Input attack;attack.attack=true;run(g,26,attack);require(g.enemies[0].hp==2,"One hit per swing");
    run(g,15);run(g,26,attack);require(g.enemies[0].hp==1,"Second swing hits once");
    run(g,15);run(g,26,attack);require(g.enemies[0].hp<=0,"Third swing kills");
    g=clean();g.player.hp=1;g.player.heals=3;Input heal;heal.heal=true;run(g,100,heal);
    require(g.player.hp==3&&g.player.heals==2,"Healing spends exactly one charge");
    g.player.pos={132,452};g.interact();require(g.player.hp==5&&g.player.heals==3,"Lantern restores supplies");
    g.progress.abilities=Dash|Wings;g.progress.shards=7;g.player.invincible=0;g.player.hp=1;
    g.hurt(1,{0,0});run(g,130);require(g.player.hp==6,"Respawn restores upgraded maximum life");
    require(g.progress.has(Dash|Wings)&&g.progress.shards==7,"Death preserves exploration progress");
}
void testBoss() {
    auto g=clean(7,{280,452},Dash|Wings|Bell);
    run(g,3);require(g.boss.active,"Arena starts boss");
    g.player.pos={185,452};g.interact();require(g.room==7,"Arena exit locked during battle");
    std::array<bool,16> seen{};
    for(int i=0;i<9000;++i) {g.player.invincible=10;g.step({});seen[int(g.boss.state)]=true;}
    require(seen[int(BossState::Charge)]&&seen[int(BossState::Airborne)]&&seen[int(BossState::Rain)],"Boss cycles charge, slam, rain");
    require(seen[int(BossState::Recover)],"Boss offers recover windows");
    // Keep the target stationary to verify the full player-attack -> phase change -> death pipeline.
    g.boss.pos={500,408};g.boss.state=BossState::Recover;g.boss.timer=999;g.player.invincible=1000;
    g.player.pos={g.boss.pos.x-35,452};g.player.facing=1;g.player.attackCooldown=0;
    bool phase2=false;
    for(int i=0;i<6000&&!g.progress.bossDefeated;++i) {
        if(g.boss.state==BossState::Enrage) {phase2=true;g.boss.timer=0;}
        if(g.boss.state==BossState::Idle) {g.boss.state=BossState::Recover;g.boss.timer=999;}
        Input in;in.attack=i%50==0;g.step(in);
    }
    require(phase2&&g.boss.phase==2,"Half health triggers second phase");
    require(g.progress.bossDefeated&&g.boss.hp==0&&g.shots.empty(),"Boss death clears attacks: hp="+std::to_string(g.boss.hp)+" hero="+std::to_string(g.player.pos.x)+","+std::to_string(g.player.pos.y)+" state="+std::to_string(int(g.boss.state)));
    g.player.pos={470,452};g.interact();require(g.ending,"Victory light reaches ending");
    g=clean(7,{300,452},Dash|Wings|Bell);run(g,3);
    g.boss.state=BossState::Recover;g.boss.timer=3;g.player.pos=g.boss.pos;g.player.invincible=0;
    int hp=g.player.hp;run(g,1);require(g.player.hp==hp,"Recovering boss contact is safe");
    g.respawn();g.enter(7,{300,452});require(g.boss.hp==42&&!g.boss.active,"Death / reentry resets unfinished encounter");
}
void testSave() {
    auto unique=std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto dir=std::filesystem::temp_directory_path()/("lumenfall-regression-"+std::to_string(unique));
    auto file=dir/"roundtrip.dat";std::string error;
    Progress p;p.abilities=Dash|Wings;p.shards=31;p.checkpointRoom=4;p.checkpoint={700,452};p.playSeconds=175.2f;
    require(saveProgress(file,p,error),"Save write");Progress restored;
    require(loadProgress(file,restored,error),"Save read");
    require(restored.abilities==p.abilities&&restored.shards==p.shards&&restored.checkpointRoom==4,"Save roundtrip contents");
    p.abilities=7;require(saveProgress(file,p,error),"Replacement save");
    auto backup=file;backup+=".bak";require(loadProgress(backup,restored,error)&&restored.abilities==3,"Previous version backup");
    {std::ofstream f(file,std::ios::app);f<<"CORRUPT";}
    require(!loadProgress(file,restored,error),"Corrupt save rejected");
    p.checkpoint={-100,999};require(saveProgress(file,p,error),"Write invalid checkpoint fixture");
    require(!loadProgress(file,restored,error),"Invalid checkpoint rejected");
    std::filesystem::remove_all(dir);
}
}
int main() {
    const std::pair<const char*,std::function<void()>> tests[]={
        {"movement",testMovement},{"map + physical progression",testMapAndProgression},
        {"combat + checkpoints",testCombatAndRecovery},{"boss encounter",testBoss},{"save validation",testSave}};
    int failed=0;
    for(const auto& [name,test]:tests) {
        try {test();std::cout<<"PASS "<<name<<'\n';}
        catch(const std::exception& e) {++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}
    }
    std::cout<<checks<<" checks, "<<failed<<" failed suites\n";return failed?1:0;
}
