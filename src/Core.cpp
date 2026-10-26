#include "Core.hpp"
#include <limits>

namespace lumen {
namespace {
void decay(float& v,float dt) { v=std::max(0.f,v-dt); }
float approach(float v,float target,float amount) {
    return v<target?std::min(v+amount,target):std::max(v-amount,target);
}
}
int Progress::shardCount() const {
    unsigned n=shards; int count=0; while(n){count+=int(n&1u); n>>=1;} return count;
}
Rect Player::slash() const {
    if(attackUp) return {pos.x-13,pos.y-45,46,50};
    return {facing>0?pos.x+12:pos.x-49,pos.y-6,57,42};
}
Rect Enemy::box() const {
    return {pos.x,pos.y,kind==EnemyKind::Sentinel?28.f:24.f,kind==EnemyKind::Sentinel?40.f:22.f};
}

void Game::newGame() {
    progress={}; player={}; boss={}; ending=false;
    enter(0,{100,452});
    message("A / D MOVE     SPACE JUMP     J ATTACK     E INTERACT",6);
    saveRequested=true;
}
void Game::restore(const Progress& p) {
    progress=p; player={}; player.hp=progress.maxHealth(); ending=false;
    enter(p.checkpointRoom,p.checkpoint);
    message("THE LANTERN REMEMBERS YOU",3);
}
void Game::enter(int id,Vec spawn) {
    room=std::clamp(id,0,int(rooms.size())-1);
    player.pos=spawn; player.vel={}; player.grounded=false; player.airDash=false;
    player.airJump=false; player.dashTime=0; player.attackTime=0; player.healTime=0;
    player.jumpBuffer=0; player.coyote=0; player.invincible=.65f;
    shots.clear(); enemies.clear(); boss={};
    for(const auto& s:rooms[room].enemies) {
        Enemy e; e.pos=s.pos; e.kind=s.kind; e.left=s.left; e.right=s.right;
        e.hp=s.kind==EnemyKind::Sentinel?5:3; e.phase=s.pos.x*.017f;
        enemies.push_back(e);
    }
    if(progress.bossDefeated) { boss.hp=0; boss.state=BossState::Dead; }
    progress.visited|=1u<<room; roomTitleTime=4.f;
}
std::vector<Rect> Game::collisionSolids() const {
    auto solids=rooms[room].solids;
    for(const auto& b:rooms[room].barriers) if(!barrierOpen(b)) solids.push_back(b.box);
    if(room==7 && boss.active && boss.hp>0) {
        solids.push_back({165,0,15,480}); solids.push_back({790,0,15,480});
    }
    return solids;
}
void Game::message(const std::string& text,float seconds) { notice=text; noticeTime=seconds; }

void Game::step(const Input& in,float dt) {
    if(dt<=0 || dt>.05f || ending) return;
    clock+=dt; progress.playSeconds+=dt;
    decay(roomTitleTime,dt); decay(noticeTime,dt); decay(shake,dt*14);
    if(hitstop>0) { decay(hitstop,dt); return; }
    if(player.hp<=0) {
        player.deadTime+=dt;
        if(player.deadTime>.9f) respawn();
        return;
    }
    movePlayer(in,dt);
    if(player.hp<=0) return;
    collect();
    if(in.interact) interact();
    updateEnemies(dt);
    updateBoss(dt);
    updateShots(dt);
    if(player.pos.y>rooms[room].height+40) {
        hurt(1,{player.pos.x,player.pos.y+40});
        if(player.hp>0) {
            // Falling returns to the last lantern. Progress is never taken away.
            const int hp=player.hp, heals=player.heals;
            enter(progress.checkpointRoom,progress.checkpoint);
            player.hp=hp; player.heals=heals; player.invincible=1.2f;
        }
    }
}

void Game::movePlayer(const Input& in,float dt) {
    auto& p=player;
    decay(p.invincible,dt); decay(p.hurtTimer,dt); decay(p.dashCooldown,dt);
    decay(p.attackCooldown,dt); decay(p.attackTime,dt); decay(p.comboTimer,dt);
    decay(p.jumpBuffer,dt);
    if(p.grounded) { p.coyote=.11f; p.airJump=false; p.airDash=false; }
    else decay(p.coyote,dt);
    if(in.jump) p.jumpBuffer=.13f;

    if(in.heal && p.grounded && std::abs(in.move)<.1f && p.dashTime<=0 && p.attackTime<=0 && p.hurtTimer<=0
       && p.hp<progress.maxHealth() && p.heals>0) {
        p.healTime+=dt;
        if(p.healTime>=.78f) {
            --p.heals; p.hp=std::min(p.hp+2,progress.maxHealth()); p.healTime=0;
            cues.push_back({Effect::Heal,p.box().center()});
        }
    } else p.healTime=0;

    if(in.dash && progress.has(Dash) && p.dashCooldown<=0 && !p.airDash && p.hurtTimer<=0) {
        p.dashTime=.18f; p.dashCooldown=.52f; p.airDash=true; p.healTime=0;
        if(std::abs(in.move)>.1f) p.facing=sign(in.move);
        p.vel={p.facing*620.f,0}; cues.push_back({Effect::Dash,p.box().center()});
    }
    if(in.attack && p.attackCooldown<=0 && p.dashTime<=0 && p.hurtTimer<=0) {
        p.combo=p.comboTimer>0?(p.combo+1)%3:0; p.comboTimer=.9f;
        p.attackTime=.25f; p.attackCooldown=p.combo==2?.40f:.29f;
        ++p.attackId; p.attackUp=in.up; p.healTime=0;
        cues.push_back({Effect::Slash,p.box().center(),p.combo});
    }
    const bool dashing=p.dashTime>0;
    if(dashing) {
        decay(p.dashTime,dt); p.vel.x=p.facing*620.f; p.vel.y=0;
        // Break before resolving collision, otherwise the wall would stop the dash first.
        Rect next=p.box(); next.x+=p.vel.x*dt;
        for(const auto& b:rooms[room].barriers) if(!barrierOpen(b) && next.overlaps(b.box)) {
            progress.barriers|=1u<<b.id; saveRequested=true;
            cues.push_back({Effect::Break,b.box.center()}); shake=3;
        }
    } else {
        if(p.hurtTimer<=0) {
            float wanted=in.move*(p.healTime>0?0.f:178.f);
            p.vel.x=approach(p.vel.x,wanted,(p.grounded?1800.f:1050.f)*dt);
            if(std::abs(in.move)>.1f) p.facing=sign(in.move);
        }
        if(p.jumpBuffer>0 && (p.coyote>0 || (progress.has(Wings) && !p.airJump))) {
            const bool second=p.coyote<=0;
            p.vel.y=second?-405.f:-410.f;
            if(second) p.airJump=true;
            p.jumpBuffer=0; p.coyote=0; p.grounded=false; p.healTime=0;
            cues.push_back({Effect::Jump,p.box().center(),second?1:0});
        }
        // A released jump falls sooner; holding jump preserves the full arc.
        p.vel.y=std::min(650.f,p.vel.y+1100.f*dt*((!in.jumpHeld && p.vel.y<0)?2.3f:1.f));
    }

    const auto solids=collisionSolids();
    p.pos.x+=p.vel.x*dt;
    for(const auto& s:solids) if(s.h>22 && p.box().overlaps(s)) {
        if(p.vel.x>0) p.pos.x=s.x-HeroW;
        else if(p.vel.x<0) p.pos.x=s.right();
        p.vel.x=0; p.dashTime=0;
    }
    if(dashing && p.dashTime<=0) p.vel.x=std::clamp(p.vel.x,-178.f,178.f);
    p.pos.x=std::clamp(p.pos.x,0.f,rooms[room].width-HeroW);
    const float previousBottom=p.pos.y+HeroH;
    p.pos.y+=p.vel.y*dt; p.grounded=false;
    for(const auto& s:solids) if(p.box().overlaps(s)) {
        // Thin ledges are one-way: jump up through them, land on them from above.
        if(s.h<=22 && (p.vel.y<0 || previousBottom>s.y+.5f)) continue;
        if(p.vel.y>=0) { p.pos.y=s.y-HeroH; p.grounded=true; }
        else p.pos.y=s.bottom();
        p.vel.y=0;
    }
    for(const auto& h:rooms[room].hazards) if(p.box().overlaps(h)) {
        // Terrain hazards are never made harmless by a dash or hit invulnerability.
        p.invincible=0; p.dashTime=0;
        hurt(1,{p.pos.x,p.pos.y+30});
        if(p.hp>0) {
            const int hp=p.hp,heals=p.heals;
            enter(progress.checkpointRoom,progress.checkpoint); p.hp=hp; p.heals=heals;
        }
        break;
    }
}

void Game::hurt(int damage,Vec source) {
    auto& p=player;
    if(p.invincible>0 || p.dashTime>0 || p.hp<=0) return;
    p.hp=std::max(0,p.hp-damage); p.invincible=1.1f; p.hurtTimer=.18f;
    p.attackTime=0; p.healTime=0; p.vel={sign(p.box().center().x-source.x)*240.f,-180};
    shake=4; hitstop=.055f; cues.push_back({Effect::Hurt,p.box().center()});
    if(p.hp==0) { p.deadTime=0; ++progress.deaths; cues.push_back({Effect::Death,p.box().center()}); }
}
void Game::respawn() {
    player={}; player.hp=progress.maxHealth();
    enter(progress.checkpointRoom,progress.checkpoint); saveRequested=true;
    message("THE LIGHT RETURNS. NOTHING YOU FOUND IS LOST.",3);
}

bool Game::itemTaken(const Item& i) const {
    if(i.kind==ItemKind::Shard) return (progress.shards&(1u<<i.id))!=0;
    const unsigned bit=i.kind==ItemKind::Dash?Dash:i.kind==ItemKind::Wings?Wings:Bell;
    return progress.has(bit);
}
void Game::collect() {
    for(const auto& i:rooms[room].items) if(!itemTaken(i) && distance(player.box().center(),i.pos)<31) {
        if(i.kind==ItemKind::Shard) {
            int old=progress.maxHealth(); progress.shards|=1u<<i.id;
            if(progress.maxHealth()>old) { player.hp=progress.maxHealth(); message("THREE ECHOES UNITED - MAXIMUM LIFE INCREASED",4); }
            else message("ECHO FOUND - "+std::to_string(progress.shardCount())+" / 8   (3 ECHOES = MORE LIFE)",4);
        } else if(i.kind==ItemKind::Dash) {
            progress.abilities|=Dash;
            message("EMBER DASH AWAKENED\nK / SHIFT - DASH THROUGH VEILS AND ENEMY ATTACKS",6);
        } else if(i.kind==ItemKind::Wings) {
            progress.abilities|=Wings;
            message("THREADWINGS AWAKENED\nSPACE IN MID-AIR - RETURN TO THE HIGH CROSSROADS DOOR",6);
        } else {
            progress.abilities|=Bell;
            message("THE BELL HEART IS YOURS\nTHE ASH BRIDGE AT THE CROSSROADS IS NOW OPEN",6);
        }
        cues.push_back({Effect::Chime,i.pos}); saveRequested=true;
    }
}
void Game::interact() {
    const Vec c=player.box().center();
    if(room==7 && progress.bossDefeated && distance(c,{480,450})<70) {
        ending=true; saveRequested=true; cues.push_back({Effect::Victory,c}); return;
    }
    for(const auto& cp:rooms[room].checkpoints) if(distance(c,cp-Vec{0,14})<48) {
        progress.checkpointRoom=room; progress.checkpoint={cp.x-10,cp.y-HeroH};
        player.hp=progress.maxHealth(); player.heals=3; shots.clear();
        message("LANTERN LIT - LIFE AND HEALING CHARGES RESTORED",3);
        cues.push_back({Effect::Chime,cp}); saveRequested=true; return;
    }
    for(const auto& p:rooms[room].portals) if(distance(c,p.pos-Vec{0,18})<45) {
        if(room==7 && boss.active && boss.hp>0) { message("THE WARDEN HAS SEALED THE CHAMBER",2); return; }
        if(!doorOpen(p)) {
            message(p.requirement==Wings?"THREADWINGS REQUIRED - SEARCH THE ARCHIVE":
                p.requirement==Bell?"BELL HEART REQUIRED - SEARCH THE FOUNDRY":"EMBER DASH REQUIRED",3); return;
        }
        enter(p.target,p.arrival); saveRequested=true; return;
    }
    for(const auto& s:rooms[room].signs) if(distance(c,s.pos-Vec{0,14})<65) { message(s.text,6); return; }
}
std::string Game::prompt() const {
    const auto c=player.box().center();
    if(room==7 && progress.bossDefeated && distance(c,{480,450})<70) return "[ E ]  RELEASE THE DAWN";
    for(auto cp:rooms[room].checkpoints) if(distance(c,cp-Vec{0,14})<48) return "[ E ]  REST / SAVE";
    for(const auto& p:rooms[room].portals) if(distance(c,p.pos-Vec{0,18})<45) {
        if(room==7 && boss.active && boss.hp>0) return "SEALED";
        return doorOpen(p)?"[ E ]  "+p.label:"[ E ]  SEALED PASSAGE";
    }
    for(const auto& s:rooms[room].signs) if(distance(c,s.pos-Vec{0,14})<65) return "[ E ]  READ MEMORY";
    return "";
}
std::string Game::objective() const {
    if(progress.bossDefeated) return "RETURN LIGHT TO THE HEART OF THE BELL";
    if(!progress.has(Dash)) return "FIND THE EMBER IN THE MOSS CISTERN";
    if(!progress.has(Wings)) return "CROSS THE VEIL. SEARCH THE HUSH ARCHIVE";
    if(!progress.has(Bell)) return "RETURN TO THE CROSSROADS. CLIMB TO THE GARDEN";
    return "CROSS THE ASH BRIDGE. AWAKEN THE BELL";
}

void Game::updateEnemies(float dt) {
    for(auto& e:enemies) {
        if(e.hp<=0) continue;
        e.phase+=dt; decay(e.cooldown,dt); decay(e.flash,dt);
        if(player.attacking() && e.lastAttack!=player.attackId && player.slash().overlaps(e.box())) {
            e.lastAttack=player.attackId; e.hp-=player.combo==2?2:1; e.flash=.17f;
            e.pos.x=std::clamp(e.pos.x+player.facing*12.f,e.left,e.right);
            hitstop=.035f; shake=1.5f;
            cues.push_back({Effect::Hit,e.box().center(),e.hp<=0?1:0});
            if(e.hp<=0) continue;
        }
        if(e.kind==EnemyKind::Crawler) {
            e.pos.x+=e.facing*42.f*dt;
            if(e.pos.x<e.left) {e.pos.x=e.left;e.facing=1;}
            if(e.pos.x>e.right) {e.pos.x=e.right;e.facing=-1;}
        } else if(e.kind==EnemyKind::Moth) {
            e.pos.x+=e.facing*52.f*dt;
            e.pos.y+=std::sin(e.phase*2.8f)*17.f*dt;
            if(e.pos.x<e.left || e.pos.x>e.right) e.facing=-e.facing;
            if(e.cooldown<=0 && distance(e.pos,player.pos)<320) {
                Vec d=player.box().center()-e.box().center(); float len=std::max(1.f,std::hypot(d.x,d.y));
                shots.push_back({e.box().center(),d*(125.f/len),4,0,ShotKind::Orb,1}); e.cooldown=2.8f;
            }
        } else {
            e.facing=sign(player.pos.x-e.pos.x);
            if(e.cooldown<=0 && distance(e.pos,player.pos)<300) {
                shots.push_back({{e.pos.x+14,e.pos.y+21},{e.facing*145.f,0},3.5f,.38f,ShotKind::Orb,1});
                e.cooldown=2.25f;
            }
        }
        if(player.box().overlaps(e.box())) hurt(1,e.box().center());
    }
}

void Game::beginBoss(BossState state,float duration) {
    boss.state=state; boss.timer=duration; boss.duration=duration;
}
std::string Game::bossAction() const {
    switch(boss.state) {
        case BossState::Intro:return "THE HOLLOW WARDEN";
        case BossState::SweepTell:case BossState::Sweep:return "CLEAVE - KEEP YOUR DISTANCE";
        case BossState::DashTell:case BossState::Charge:return "CHARGE - JUMP OR DASH THROUGH";
        case BossState::SlamTell:case BossState::Airborne:return "BELLFALL - MOVE, THEN JUMP THE WAVES";
        case BossState::RainTell:case BossState::Rain:return "ASH RAIN - MOVE OUT OF THE MARKS";
        case BossState::Recover:return "EXPOSED - STRIKE THE HEART";
        case BossState::Enrage:return "THE BELL IS BREAKING - PHASE II";
        default:return "";
    }
}
void Game::updateBoss(float dt) {
    if(room!=7 || progress.bossDefeated) return;
    auto& b=boss; decay(b.flash,dt);
    if(!b.active) {
        if(player.pos.x>255) {
            b.active=true; beginBoss(BossState::Intro,1.8f);
            shots.clear(); message("THE HOLLOW WARDEN",2.5f); cues.push_back({Effect::Boom,b.box().center()});
        }
        return;
    }
    b.timer-=dt;
    const bool vulnerable=b.state!=BossState::Intro && b.state!=BossState::Enrage && b.state!=BossState::Airborne;
    if(vulnerable && player.attacking() && b.lastAttack!=player.attackId && player.slash().overlaps(b.box())) {
        b.lastAttack=player.attackId; b.hp-=player.combo==2?2:1; b.flash=.13f;
        hitstop=.045f; shake=2; cues.push_back({Effect::Hit,b.box().center()});
        if(b.hp<=0) {
            b.hp=0; b.active=false; beginBoss(BossState::Dead,0); progress.bossDefeated=true;
            shots.clear(); shake=8; saveRequested=true;
            cues.push_back({Effect::Victory,b.box().center()});
            message("THE BELL IS SILENT\nMEET THE LIGHT AT THE CENTER. PRESS E.",7); return;
        }
        if(b.hp<=21 && b.phase==1) {
            b.phase=2; shots.clear(); beginBoss(BossState::Enrage,1.5f);
            cues.push_back({Effect::Boom,b.box().center()}); shake=6; return;
        }
    }
    const float fast=b.phase==2?.78f:1.f;
    switch(b.state) {
        case BossState::Intro:case BossState::Enrage:
            if(b.timer<=0) beginBoss(BossState::Idle,.65f);
            break;
        case BossState::Idle:
            b.facing=sign(player.pos.x-b.pos.x);
            if(b.timer<=0) {
                const int seq=b.sequence++%4;
                if(seq==0) beginBoss(BossState::DashTell,.95f*fast);
                else if(seq==1) beginBoss(BossState::SlamTell,.85f*fast);
                else if(seq==2) beginBoss(BossState::RainTell,1.0f*fast);
                else if(distance(b.pos,player.pos)<155) beginBoss(BossState::SweepTell,.8f*fast);
                else beginBoss(BossState::DashTell,.85f*fast);
            }
            break;
        case BossState::SweepTell:
            if(b.timer<=0) {beginBoss(BossState::Sweep,.22f); cues.push_back({Effect::Slash,b.box().center()});}
            break;
        case BossState::Sweep:
            if(b.sweep().overlaps(player.box())) hurt(2,b.box().center());
            if(b.timer<=0) beginBoss(BossState::Recover,1.10f);
            break;
        case BossState::DashTell:
            if(b.timer<=0) {beginBoss(BossState::Charge,1.6f); cues.push_back({Effect::Dash,b.box().center()});}
            break;
        case BossState::Charge:
            b.pos.x+=b.facing*(b.phase==2?485.f:370.f)*dt;
            if(player.box().overlaps(b.box())) hurt(2,b.box().center());
            if(b.pos.x<195 || b.pos.x>713 || b.timer<=0) {
                b.pos.x=std::clamp(b.pos.x,195.f,713.f); shake=5;
                if(b.phase==2) shots.push_back({{b.pos.x+29,468},{-b.facing*185.f,0},3,0,ShotKind::Wave,1});
                beginBoss(BossState::Recover,1.3f); cues.push_back({Effect::Boom,b.box().center()});
            }
            break;
        case BossState::SlamTell:
            if(b.timer<=0) {
                b.start=b.pos; b.target={std::clamp(player.pos.x-20,210.f,700.f),408};
                beginBoss(BossState::Airborne,.9f);
            }
            break;
        case BossState::Airborne: {
            float t=std::clamp(1.f-b.timer/b.duration,0.f,1.f);
            b.pos=b.start*(1-t)+b.target*t;
            b.pos.y-=std::sin(t*3.14159265f)*210.f;
            if(t>.8f && player.box().overlaps(b.box())) hurt(2,b.box().center());
            if(b.timer<=0) {
                b.pos=b.target;
                shots.push_back({{b.pos.x+29,468},{-190,0},4,0,ShotKind::Wave,1});
                shots.push_back({{b.pos.x+29,468},{190,0},4,0,ShotKind::Wave,1});
                if(b.phase==2) {
                    shots.push_back({{b.pos.x+29,468},{-190,0},4,.46f,ShotKind::Wave,1});
                    shots.push_back({{b.pos.x+29,468},{190,0},4,.46f,ShotKind::Wave,1});
                }
                shake=6; cues.push_back({Effect::Boom,b.box().center()});
                beginBoss(BossState::Recover,1.45f);
            }
            break;
        }
        case BossState::RainTell:
            if(b.timer<=0) {
                float px=std::clamp(player.pos.x+10,210.f,745.f);
                const int count=b.phase==2?5:3;
                for(int i=0;i<count;++i) {
                    float x=std::clamp(px+(i-count/2)*88.f,205.f,750.f);
                    shots.push_back({{x,0},{0,0},.24f,.85f+i*.12f,ShotKind::Column,1});
                }
                beginBoss(BossState::Rain,1.8f);
            }
            break;
        case BossState::Rain:
            if(b.timer<=0) beginBoss(BossState::Recover,1.25f);
            break;
        case BossState::Recover:
            if(b.timer<=0) beginBoss(BossState::Idle,.65f*fast);
            break;
        default:break;
    }
    // Touching the recovering boss is safe, making the advertised punish window real.
    if(b.state!=BossState::Recover && b.state!=BossState::Intro && b.state!=BossState::Enrage
       && b.state!=BossState::Airborne && player.box().overlaps(b.box())) hurt(1,b.box().center());
}
void Game::updateShots(float dt) {
    for(auto& s:shots) {
        if(s.life<=0) continue;
        if(s.delay>0) { s.delay-=dt; continue; }
        s.life-=dt; s.pos=s.pos+s.vel*dt;
        Rect hit;
        if(s.kind==ShotKind::Column) hit={s.pos.x-13,0,26,480};
        else if(s.kind==ShotKind::Wave) hit={s.pos.x-14,s.pos.y-12,28,24};
        else hit={s.pos.x-5,s.pos.y-5,10,10};
        if(player.box().overlaps(hit)) {
            hurt(s.damage,s.pos);
            if(s.kind==ShotKind::Orb) s.life=0;
        }
        if(s.kind==ShotKind::Orb) {
            for(const auto& block:rooms[room].solids) if(hit.overlaps(block)) {s.life=0;break;}
            if(player.attacking() && player.slash().overlaps(hit)) {
                s.life=0; cues.push_back({Effect::Hit,s.pos});
            }
        }
        if(s.pos.x<0 || s.pos.x>rooms[room].width) s.life=0;
    }
    shots.erase(std::remove_if(shots.begin(),shots.end(),[](const Shot& s){return s.life<=0;}),shots.end());
}
} // namespace lumen
