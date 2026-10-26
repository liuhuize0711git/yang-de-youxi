#include "Renderer.hpp"
#include <cctype>
#include <iomanip>
#include <sstream>

namespace lumen {
namespace {
const sf::Color Ink(8,15,23), White(234,230,210), Muted(128,153,158), Teal(109,228,210), Gold(239,184,103), Red(245,111,105);
sf::Color alpha(sf::Color c,float a) {c.a=std::uint8_t(std::clamp(a,0.f,255.f));return c;}
sf::Color mix(sf::Color a,sf::Color b,float t) {
    return {std::uint8_t(a.r+(b.r-a.r)*t),std::uint8_t(a.g+(b.g-a.g)*t),std::uint8_t(a.b+(b.b-a.b)*t)};
}
unsigned hash(unsigned x) { x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; return x^(x>>16); }
float fract(float n) {return n-std::floor(n);}
std::array<unsigned char,7> glyph(char ch) {
    static const std::array<std::array<unsigned char,7>,36> data{{
        {{14,17,17,31,17,17,17}},{{30,17,17,30,17,17,30}},{{14,17,16,16,16,17,14}},
        {{30,17,17,17,17,17,30}},{{31,16,16,30,16,16,31}},{{31,16,16,30,16,16,16}},
        {{14,17,16,23,17,17,15}},{{17,17,17,31,17,17,17}},{{31,4,4,4,4,4,31}},
        {{7,2,2,2,18,18,12}},{{17,18,20,24,20,18,17}},{{16,16,16,16,16,16,31}},
        {{17,27,21,21,17,17,17}},{{17,25,21,19,17,17,17}},{{14,17,17,17,17,17,14}},
        {{30,17,17,30,16,16,16}},{{14,17,17,17,21,18,13}},{{30,17,17,30,20,18,17}},
        {{15,16,16,14,1,1,30}},{{31,4,4,4,4,4,4}},{{17,17,17,17,17,17,14}},
        {{17,17,17,17,17,10,4}},{{17,17,17,21,21,21,10}},{{17,17,10,4,10,17,17}},
        {{17,17,10,4,4,4,4}},{{31,1,2,4,8,16,31}},
        {{14,17,19,21,25,17,14}},{{4,12,4,4,4,4,14}},{{14,17,1,2,4,8,31}},
        {{30,1,1,14,1,1,30}},{{2,6,10,18,31,2,2}},{{31,16,16,30,1,1,30}},
        {{14,16,16,30,17,17,14}},{{31,1,2,4,8,8,8}},{{14,17,17,14,17,17,14}},
        {{14,17,17,15,1,1,14}}
    }};
    if(ch>='A'&&ch<='Z') return data[ch-'A'];
    if(ch>='0'&&ch<='9') return data[ch-'0'+26];
    switch(ch) {
        case '.':return {{0,0,0,0,0,12,12}};
        case ':':return {{0,12,12,0,12,12,0}};
        case ',':return {{0,0,0,0,0,4,8}};
        case '-':return {{0,0,0,31,0,0,0}};
        case '/':return {{1,1,2,4,8,16,16}};
        case '+':return {{0,4,4,31,4,4,0}};
        case '[':return {{14,8,8,8,8,8,14}};
        case ']':return {{14,2,2,2,2,2,14}};
        case '(':return {{2,4,8,8,8,4,2}};
        case ')':return {{8,4,2,2,2,4,8}};
        case '=':return {{0,0,31,0,31,0,0}};
        case '?':return {{14,17,1,2,4,0,4}};
        case '!':return {{4,4,4,4,4,0,4}};
        case '>':return {{16,8,4,2,4,8,16}};
        case '<':return {{1,2,4,8,4,2,1}};
        default:return {};
    }
}
sf::Color accent(int theme) {
    const sf::Color colors[]={Teal,{128,207,150},{163,155,226},Gold,{238,143,104}};
    return colors[std::clamp(theme,0,4)];
}
}
Renderer::Renderer()=default;
void Renderer::rect(float x,float y,float w,float h,sf::Color c) {
    if(w<=0||h<=0||x>640||y>360||x+w<0||y+h<0) return;
    tri({x,y},{x+w,y},{x+w,y+h},c); tri({x,y},{x+w,y+h},{x,y+h},c);
}
void Renderer::tri(Vec a,Vec b,Vec c,sf::Color color) {
    mesh.append({{a.x,a.y},color});mesh.append({{b.x,b.y},color});mesh.append({{c.x,c.y},color});
}
void Renderer::line(Vec a,Vec b,float width,sf::Color color) {
    Vec d=b-a; float len=std::max(.001f,std::hypot(d.x,d.y));
    Vec n={-d.y/len*width*.5f,d.x/len*width*.5f};
    tri(a+n,b+n,b-n,color);tri(a+n,b-n,a-n,color);
}
void Renderer::circle(Vec p,float radius,sf::Color color,int segments) {
    for(int i=0;i<segments;++i) {
        float a=i*6.2831853f/segments,b=(i+1)*6.2831853f/segments;
        tri(p,p+Vec{std::cos(a),std::sin(a)}*radius,p+Vec{std::cos(b),std::sin(b)}*radius,color);
    }
}
void Renderer::glow(Vec p,float radius,sf::Color c) {
    for(int i=10;i>0;--i) circle(p,radius*i/10.f,alpha(c,(11-i)*.6f));
}
void Renderer::text(std::string str,float x,float y,float scale,sf::Color c,bool centered) {
    scale=std::max(1.f,std::round(scale));
    std::istringstream lines(str); std::string row;
    while(std::getline(lines,row)) {
        float px=x-(centered?float(row.size())*6.f*scale*.5f:0);
        for(unsigned char ch:row) {
            const auto bits=glyph(char(std::toupper(ch)));
            for(int r=0;r<7;++r) for(int col=0;col<5;++col)
                if(bits[r]&(1<<(4-col))) rect(std::floor(px+col*scale),std::floor(y+r*scale),scale,scale,c);
            px+=scale*6;
        }
        y+=10*scale;
    }
}
void Renderer::panel(float x,float y,float w,float h) {
    rect(x,y,w,h,{6,13,21,241}); rect(x,y,w,1,{71,93,99,220});
    rect(x,y+h-1,w,1,{42,62,70,220});
}
void Renderer::snapCamera(const Game& g) {
    const auto& r=g.rooms[g.room];
    camera={std::clamp(g.player.pos.x-310.f,0.f,std::max(0.f,r.width-640)),
        std::clamp(g.player.pos.y-195.f,0.f,std::max(0.f,r.height-360))};
    if(g.room==7) camera={160,180};
    lastRoom=g.room;
}
void Renderer::update(const Game& g,float dt) {
    time+=dt;
    if(lastRoom!=g.room) {snapCamera(g);particles.clear();}
    const auto& r=g.rooms[g.room];
    Vec goal={std::clamp(g.player.pos.x-310.f+g.player.vel.x*.13f,0.f,std::max(0.f,r.width-640)),
        std::clamp(g.player.pos.y-190.f,0.f,std::max(0.f,r.height-360))};
    if(g.room==7) goal={160,180};
    camera=camera+(goal-camera)*(1-std::exp(-dt*7));
    for(auto& p:particles) {p.life-=dt;p.pos=p.pos+p.vel*dt;p.vel.y+=100*dt;}
    particles.erase(std::remove_if(particles.begin(),particles.end(),[](auto& p){return p.life<=0;}),particles.end());
}
void Renderer::feed(const Cue& c) {
    const bool big=c.effect==Effect::Boom||c.effect==Effect::Victory||c.effect==Effect::Chime;
    sf::Color color=c.effect==Effect::Hurt?Red:c.effect==Effect::Hit?Gold:Teal;
    if(big) color=Gold;
    std::uniform_real_distribution<float> u(0,1);
    for(int i=0;i<(big?48:14);++i) {
        float a=u(rng)*6.283185f,s=20+u(rng)*(big?155:95),life=.25f+u(rng)*.7f;
        particles.push_back({c.pos,{std::cos(a)*s,std::sin(a)*s},life,life,1+u(rng)*2,color});
    }
}

void Renderer::background(const Game& g,bool title) {
    const int theme=g.rooms[g.room].theme;
    const auto ac=accent(theme);
    sf::Color top=mix(Ink,ac,.04f),bottom=mix(Ink,ac,.18f);
    for(int y=0;y<360;y+=3) rect(0,float(y),640,3,mix(top,bottom,y/360.f));
    // Sky aperture, distant masonry and parallax ribs.
    Vec moon={475-camera.x*.06f,76-camera.y*.04f};
    glow(moon,125,ac); circle(moon,39,alpha(mix(ac,White,.2f),28));
    circle(moon+Vec{-9,-6},36,alpha(top,230));
    for(int layer=0;layer<3;++layer) {
        float parallax=.08f+layer*.13f;
        for(int i=-2;i<12;++i) {
            unsigned h=hash(unsigned(i+80+layer*30+theme*12));
            float x=i*102.f-std::fmod(camera.x*parallax,102.f);
            float y=100.f+float(h%100)-camera.y*parallax*.25f+layer*32;
            sf::Color c=mix(top,ac,.055f+layer*.028f);
            rect(x,y,60,360,c);
            tri({x-3,y},{x+30,y-30-float(h%35)},{x+63,y},c);
            rect(x+10,y+30,10,150,alpha(Ink,65));
            rect(x+39,y+30,10,150,alpha(Ink,65));
            for(int j=0;j<3;++j) rect(x+13+j*12,y+16,3,9,alpha(ac,22));
        }
    }
    // Broken foreground arches are deliberately silhouettes, leaving gameplay edges readable.
    for(int i=-1;i<5;++i) {
        float x=i*230.f-std::fmod(camera.x*.40f,230.f);
        sf::Color c=mix(top,ac,.11f);
        rect(x,0,16,310,c); rect(x+166,0,16,330,c);
        for(int j=0;j<9;++j) {
            float xx=x+16+j*17;
            float yy=75-std::sin((j+.5f)/9.f*3.14159f)*36;
            rect(xx,yy,18,10,c);
        }
        rect(x-5,255,26,9,c); rect(x+161,268,26,9,c);
    }
    // Soft shafts; all original geometry, no external texture files.
    for(int i=0;i<3;++i) {
        float x=345+i*117-camera.x*.05f;
        tri({x,-20},{x-125,360},{x+8,360},alpha(ac,7));
        tri({x+6,-20},{x-65,360},{x+90,360},alpha(ac,4));
    }
    for(int i=0;i<48;++i) {
        unsigned h=hash(unsigned(i+738));
        float x=fract(float(h%10000)/10000.f+time*(.0015f+(i%4)*.0005f))*640;
        float y=fract(float((h>>8)%10000)/10000.f-time*.003f)*360;
        float a=25+35*(.5f+.5f*std::sin(time+i));
        rect(x,y,i%7==0?2.f:1.f,1,alpha(ac,a));
    }
    for(int i=0;i<6;++i) {
        float y=205+i*27+std::sin(time*.15f+i)*4;
        rect(0,y,640,8,alpha(ac,3));
    }
    if(theme==1) {
        for(int i=0;i<14;++i) {
            float x=i*61.f-std::fmod(camera.x*.55f,61.f);
            float length=70+float(hash(i+88)%210);
            for(int j=0;j<8;++j) {
                Vec a{x+std::sin(j*.6f+i)*8,j*length/8},b{x+std::sin((j+1)*.6f+i)*8,(j+1)*length/8};
                line(a,b,2,{35,69,57,120});
                tri(b,b+Vec{-9,-5},b+Vec{-6,3},{46,82,65,150});
            }
        }
    } else if(theme==2) {
        for(int i=-1;i<6;++i) {
            float x=i*145.f-std::fmod(camera.x*.3f,145.f);
            rect(x,90,106,240,{20,28,43,180});
            for(int row=0;row<5;++row) {
                float y=112+row*41.f;
                for(int book=0;book<12;++book) {
                    unsigned h=hash(i*73+row*31+book+1000);
                    rect(x+7+book*8.f,y+float(h%12),5,29-float(h%12),{std::uint8_t(43+h%18),45,63,190});
                }
                rect(x,y+30,106,3,{52,51,66,170});
            }
        }
    } else if(theme==3) {
        for(int i=0;i<4;++i) {
            Vec p={i*240.f-std::fmod(camera.x*.25f,240.f)+40,145.f+float(i%2)*70};
            circle(p,52,{48,44,40,150});circle(p,44,{15,27,33,220});
            for(int j=0;j<8;++j) {
                float a=j*.7854f+time*.055f;
                line(p,p+Vec{std::cos(a)*51,std::sin(a)*51},5,{68,59,44,120});
            }
            circle(p,8,{88,72,49,140});
            line(p+Vec{0,-60},p+Vec{0,-200},3,{62,57,49,120});
        }
    } else if(theme==4) {
        circle({320,143},99,{83,58,45,45});circle({320,143},94,{13,22,30,180});
        for(int j=0;j<12;++j) {
            float a=j*.5236f;
            line(Vec{320,143}+Vec{std::cos(a),std::sin(a)}*88,
                 Vec{320,143}+Vec{std::cos(a),std::sin(a)}*96,2,{137,99,62,80});
        }
        tri({293,76},{265,197},{373,197},{68,57,48,65});
        tri({347,76},{265,197},{373,197},{68,57,48,65});
        rect(256,195,125,7,{117,85,56,60});
        line({285,0},{285,80},3,{78,66,53,95});line({355,0},{355,80},3,{78,66,53,95});
    }
    if(title) {
        rect(0,0,365,360,{5,12,21,135});
        rect(0,324,640,36,Ink);
        for(int i=0;i<48;++i) rect(i*14.f,320+float(hash(i)%8),13,40,mix(Ink,ac,.07f));
    }
}
void Renderer::terrain(const Game& g) {
    auto& room=g.rooms[g.room]; auto ac=accent(room.theme);
    for(const auto& b:room.solids) {
        float x=std::floor(b.x-camera.x),y=std::floor(b.y-camera.y);
        if(x>640||x+b.w<0||y>360||y+b.h<0) continue;
        rect(x,y,b.w,b.h,{15,26,34});
        rect(x,y,b.w,3,mix(ac,Muted,.65f));
        rect(x,y+3,b.w,5,mix(Ink,ac,.24f));
        for(int yy=10;yy<int(b.h);yy+=16) {
            for(int xx=0;xx<int(b.w);xx+=30) {
                unsigned h=hash(unsigned(int(b.x)+xx*7+yy*41));
                float bx=x+xx+(yy%32==10?0:10);
                rect(bx,y+yy,26,1,{35,46,50,150});
                rect(bx,y+yy,1,13,{5,13,22,160});
                if(h%3==0) rect(bx+8,y+yy+5,8,2,alpha(ac,12));
            }
        }
        for(int xx=3;xx<int(b.w)-4;xx+=11) {
            unsigned h=hash(unsigned(int(b.x)+xx*17));
            if(h%4!=0) {
                rect(x+xx,y-float(2+h%5),1,5,alpha(ac,90));
                rect(x+xx+1,y-2,2,2,alpha(ac,90));
            }
            if(h%17==0) { // luminous mushrooms
                rect(x+xx,y-9,2,9,{85,127,126});
                rect(x+xx-3,y-10,8,3,alpha(ac,170));
                glow({x+xx,y-10},16,alpha(ac,100));
            }
        }
        if(b.h<30) {
            for(int j=15;j<int(b.w);j+=29) {
                float len=8+float(hash(unsigned(j+int(b.x)))%22);
                line({x+j,y+b.h},{x+j-3,y+b.h+len},1,alpha(ac,60));
                rect(x+j-5,y+b.h+len-4,5,2,alpha(ac,80));
            }
        }
    }
    for(const auto& h:room.hazards) {
        float x=h.x-camera.x,y=h.y-camera.y;
        rect(x,y,h.w,h.h,{54,23,36});
        for(float i=0;i<h.w;i+=12) {
            tri({x+i,y+9},{x+i+5,y-8},{x+i+10,y+9},{154,69,72});
            rect(x+i+4,y-6,2,5,Red);
        }
        glow({x+h.w/2,y},40,Red);
    }
    // Hanging wires fixed in world space.
    for(int i=0;i<int(room.width/170);++i) {
        float x=i*173.f+55-camera.x, len=80+float(hash(i+111)%160)-camera.y;
        if(len>0) {
            line({x,0},{x-11,len},1,{32,55,59});
            rect(x-13,len,5,8,mix(ac,Ink,.7f));
            rect(x-12,len+4,3,2,alpha(ac,125));
        }
    }
}
void Renderer::hero(Vec p,float velocity,int facing,bool airborne,float opacity) {
    const auto c=[&](sf::Color col){return alpha(col,col.a*opacity);};
    float x=std::floor(p.x),y=std::floor(p.y);
    const float bob=airborne?0:std::sin(time*12)*std::min(1.f,std::abs(velocity)/180.f);
    y+=bob;
    const float foot=airborne?2:std::sin(time*15)*std::min(2.f,std::abs(velocity)/80.f);
    rect(x+3,y+23+foot,5,5,c({36,51,58})); rect(x+12,y+23-foot,5,5,c({36,51,58}));
    tri({x+5,y+12},{x-2,y+26},{x+22,y+26},c({126,111,89}));
    tri({x+5,y+12},{x+3,y+24},{x+16,y+24},c({188,173,142}));
    rect(x+3,y+3,15,15,c({79,82,72}));
    rect(x+4,y+1,12,15,c(White)); rect(x+1,y+5,18,8,c(White));
    rect(x+5,y-3,4,5,c(White)); rect(x+12,y-1,4,3,c(White));
    rect(x+5+(facing>0?2.f:0.f),y+7,10,6,c({25,47,51}));
    rect(x+(facing>0?10.f:5.f),y+8,2,2,c(Teal));
    rect(x+(facing>0?14.f:9.f),y+8,2,2,c(Teal));
    rect(x+3,y+15,16,3,c({210,118,66}));
    float tail=7+std::abs(velocity)*.035f;
    tri({x+10,y+16},{x+10-facing*tail,y+17+std::sin(time*9)*2},{x+10-facing*tail,y+20},c(Gold));
    rect(x+(facing>0?18.f:-1.f),y+18,3,5,c(White));
    rect(x+7,y+20,1,2,c({90,83,71}));rect(x+10,y+22,1,2,c({90,83,71}));
}
void Renderer::objects(const Game& g) {
    auto ac=accent(g.rooms[g.room].theme);
    for(const auto& p:g.rooms[g.room].portals) {
        Vec v=p.pos-camera; bool open=g.doorOpen(p);
        sf::Color c=open?ac:Gold;
        rect(v.x-18,v.y-49,36,49,{5,12,21});
        glow(v-Vec{0,24},42,c);
        rect(v.x-20,v.y-48,5,48,{47,66,69});rect(v.x+15,v.y-48,5,48,{47,66,69});
        line(v+Vec{-18,-47},v+Vec{0,-59},5,{47,66,69});line(v+Vec{0,-59},v+Vec{18,-47},5,{47,66,69});
        if(open) {
            rect(v.x-13,v.y-43,2,40,alpha(c,120));rect(v.x+11,v.y-43,2,40,alpha(c,120));
            for(int i=0;i<4;++i) rect(v.x-7+i*5,v.y-35+std::sin(time*2+i)*8,1,10,alpha(c,80));
        } else {
            for(int i=-10;i<=10;i+=10) rect(v.x+i,v.y-44,2,44,alpha(c,150));
            circle(v-Vec{0,27},5,Gold);rect(v.x-1,v.y-29,2,5,Ink);
        }
    }
    for(auto cp:g.rooms[g.room].checkpoints) {
        Vec v=cp-camera; glow(v-Vec{0,30},66,Gold);
        rect(v.x-12,v.y-4,24,4,{58,63,64});rect(v.x-2,v.y-28,4,25,{99,90,70});
        rect(v.x-7,v.y-43,14,17,{64,65,61});rect(v.x-4,v.y-40,8,11,Gold);
        rect(v.x-2,v.y-38,4,7,White);
        line(v+Vec{-10,-44},v+Vec{0,-49},2,Muted);line(v+Vec{0,-49},v+Vec{10,-44},2,Muted);
        circle(v-Vec{0,34},2+std::sin(time*4),White);
    }
    for(const auto& s:g.rooms[g.room].signs) {
        Vec v=s.pos-camera;rect(v.x-2,v.y-23,3,23,{90,99,96});
        rect(v.x-10,v.y-29,20,12,{49,69,74});rect(v.x-6,v.y-26,12,1,Muted);rect(v.x-6,v.y-23,8,1,Muted);
    }
    for(const auto& b:g.rooms[g.room].barriers) if(!g.barrierOpen(b)) {
        float x=b.box.x-camera.x,y=b.box.y-camera.y;
        rect(x,y,b.box.w,b.box.h,{74,139,142,60});
        for(int j=0;j<int(b.box.h);j+=9) line({x,y+j},{x+b.box.w,y+j+8},2,alpha(Teal,90));
        rect(x,y,2,b.box.h,Teal);rect(x+b.box.w-2,y,2,b.box.h,Teal);
    }
    for(const auto& i:g.rooms[g.room].items) if(!g.itemTaken(i)) {
        Vec v=i.pos-camera;v.y+=std::sin(time*2.5f+i.id)*3;
        bool shard=i.kind==ItemKind::Shard; sf::Color c=shard?Teal:i.kind==ItemKind::Bell?Gold:White;
        glow(v,shard?32:58,c);
        float sz=shard?5.f:10.f;
        tri(v+Vec{0,-sz},v+Vec{sz,0},v+Vec{0,sz},c);
        tri(v+Vec{0,-sz},v+Vec{0,sz},v+Vec{-sz,0},mix(c,Ink,.4f));
        rect(v.x-1,v.y-3,2,6,White);
        if(!shard) {
            float ang=time*.7f;
            for(int j=0;j<4;++j) {float a=ang+j*1.5708f;circle(v+Vec{std::cos(a)*19,std::sin(a)*19},1.5f,c);}
        }
    }
    for(const auto& e:g.enemies) if(e.hp>0) {
        Vec v=e.pos-camera;sf::Color c=e.flash>0?White:Red;
        if(e.kind==EnemyKind::Crawler) {
            for(int i=0;i<4;++i) rect(v.x+i*6,v.y+17+std::sin(time*13+i)*2,3,6,{54,57,69});
            rect(v.x+1,v.y+7,22,13,{51,56,67});rect(v.x+4,v.y+2,17,13,{76,77,81});
            rect(v.x+(e.facing>0?17.f:4.f),v.y+9,4,3,c);
            tri(v+Vec{5,4},v+Vec{9,-3},v+Vec{12,4},{86,86,83});
        } else if(e.kind==EnemyKind::Moth) {
            float flap=std::sin(time*12+e.phase)*9;
            tri(v+Vec{12,10},v+Vec{-10,flap},v+Vec{0,20},{101,89,126});
            tri(v+Vec{12,10},v+Vec{34,flap},v+Vec{25,20},{101,89,126});
            rect(v.x+8,v.y+5,9,14,{41,40,60});rect(v.x+10,v.y+6,5,3,c);
            glow(v+Vec{12,9},14,c);
        } else {
            rect(v.x+3,v.y+28,7,12,{37,47,60});rect(v.x+18,v.y+28,7,12,{37,47,60});
            rect(v.x,v.y+10,28,20,{55,65,77});rect(v.x+5,v.y+1,19,17,{95,96,99});
            rect(v.x+8,v.y+8,12,3,c);rect(v.x+12,v.y+22,5,5,Gold);
        }
        if(e.flash>0) {rect(v.x,v.y-6,28,2,{30,40,45});rect(v.x,v.y-6,28.f*e.hp/(e.kind==EnemyKind::Sentinel?5:3),2,Red);}
    }
    if(g.room==7) warden(g);
    for(const auto& s:g.shots) {
        Vec v=s.pos-camera;
        if(s.kind==ShotKind::Column) {
            if(s.delay>0) {
                rect(v.x-12,0,24,480-camera.y,{243,100,91,16});
                rect(v.x-13,480-camera.y-4,26,4,alpha(Red,130+std::sin(time*20)*80));
                line({v.x,0},{v.x,480-camera.y},1,alpha(Red,80));
            } else {rect(v.x-13,0,26,480-camera.y,Red);rect(v.x-4,0,8,480-camera.y,White);}
        } else if(s.kind==ShotKind::Wave) {
            if(s.delay>0) continue;
            glow(v,23,Gold);
            tri(v+Vec{-14,10},v+Vec{0,-15},v+Vec{14,10},Gold);
            tri(v+Vec{-6,8},v+Vec{0,-10},v+Vec{6,8},White);
        } else {glow(v,s.delay>0?22:14,Red);circle(v,s.delay>0?3:5,Red,8);circle(v,2,White,8);}
    }
    const auto& p=g.player;
    Vec pos=p.pos-camera;
    if(p.hp>0) {
        if(p.dashTime>0) for(int i=4;i>0;--i) hero(pos-Vec{p.facing*i*13.f,0},0,p.facing,true,.12f+(4-i)*.05f);
        glow(pos+Vec{10,10},30,alpha(Teal,80));
        if(p.invincible<=0||int(time*18)%2==0) hero(pos,p.vel.x,p.facing,!p.grounded);
        if(p.attackTime>0) {
            float t=1-p.attackTime/.25f;
            Vec center=pos+Vec{10,14};
            float base=p.attackUp?-1.5708f:p.facing>0?0.f:3.14159f;
            float radius=p.combo==2?54:46;
            for(int i=0;i<18;++i) {
                float a=base-1.0f+i*.10f+t*.5f,b=a+.14f;
                Vec pa=center+Vec{std::cos(a),std::sin(a)}*radius;
                Vec pb=center+Vec{std::cos(b),std::sin(b)}*radius;
                line(pa,pb,2.5f,alpha(p.combo==2?Gold:Teal,(1-t)*220));
                tri(center,pa,pb,alpha(Teal,(1-t)*12));
            }
        }
        if(p.healTime>0) {rect(pos.x-5,pos.y-11,30,3,{29,50,59});rect(pos.x-5,pos.y-11,30*p.healTime/.78f,3,Teal);glow(pos,45,Teal);}
    }
    for(const auto& pfx:particles) {Vec v=pfx.pos-camera;rect(v.x,v.y,pfx.size,pfx.size,alpha(pfx.color,255*pfx.life/pfx.maxLife));}
    if(g.room==7 && g.progress.bossDefeated) {
        Vec v=Vec{480,453}-camera;glow(v,100,Gold);circle(v,8+std::sin(time*2)*2,Gold);circle(v,4,White);
        for(int i=0;i<6;++i) circle(v+Vec{std::sin(time+i)*20,std::cos(time+i)*25},1,White);
    }
}
void Renderer::warden(const Game& g) {
    const auto& b=g.boss;
    Vec v=b.pos-camera;
    if(g.progress.bossDefeated) {
        for(int i=0;i<9;++i) rect(v.x+i*6,v.y+62-float(hash(i)%13),5,10,{65,65,66});
        return;
    }
    const bool tell=b.state==BossState::DashTell||b.state==BossState::SweepTell||b.state==BossState::SlamTell||b.state==BossState::RainTell;
    auto metal=b.flash>0?White:sf::Color(66,73,82);
    auto light=b.phase==2?Red:Gold;
    if(b.state==BossState::Recover) light=Teal;
    if(tell) {
        glow(v+Vec{29,28},80,Red);
        float remain=b.timer/std::max(.01f,b.duration);
        rect(v.x+2,v.y-30,54,3,{45,39,47});rect(v.x+2,v.y-30,54*(1-remain),3,Red);
        if(b.state==BossState::DashTell) {
            rect(180-camera.x,477-camera.y,610,3,alpha(Red,120));
            for(int i=0;i<5;++i) {
                float x=v.x+29+b.facing*(55+i*33.f);
                line({x,460-camera.y},{x+b.facing*9.f,466-camera.y},2,Red);
                line({x+b.facing*9.f,466-camera.y},{x,472-camera.y},2,Red);
            }
        }
        if(b.state==BossState::SweepTell) {
            Rect a=b.sweep();rect(a.x-camera.x,a.y-camera.y,a.w,a.h,{244,108,100,30});
        }
    }
    if(b.state==BossState::Airborne) {
        float x=b.target.x+29-camera.x;
        rect(x-35,475-camera.y,70,5,alpha(Red,150+80*std::sin(time*25)));
    }
    // Segmented brass automaton with a hollow bell for a chest.
    rect(v.x+7,v.y+56,14,16,metal);rect(v.x+38,v.y+56,14,16,metal);
    rect(v.x+3,v.y+68,23,5,{92,85,76});rect(v.x+34,v.y+68,23,5,{92,85,76});
    tri(v+Vec{14,33},v+Vec{4,67},v+Vec{55,67},{38,41,53});
    rect(v.x+7,v.y+15,45,39,metal);rect(v.x+2,v.y+17,12,23,metal);rect(v.x+46,v.y+17,12,23,metal);
    rect(v.x-5,v.y+26,10,29,{89,86,83});rect(v.x+54,v.y+26,10,29,{89,86,83});
    rect(v.x+14,v.y+4,31,21,{117,113,103});rect(v.x+18,v.y-3,23,10,{77,85,87});
    line(v+Vec{18,6},v+Vec{8,-13},5,{172,161,135});line(v+Vec{8,-13},v+Vec{15,-22},4,{172,161,135});
    line(v+Vec{41,6},v+Vec{51,-13},5,{172,161,135});line(v+Vec{51,-13},v+Vec{44,-22},4,{172,161,135});
    rect(v.x+18,v.y+11,24,4,{19,31,38});rect(v.x+21,v.y+12,5,2,light);rect(v.x+34,v.y+12,5,2,light);
    circle(v+Vec{29,36},15,{145,118,81},8);circle(v+Vec{29,36},11,{17,30,39},8);
    glow(v+Vec{29,36},40,light);circle(v+Vec{29,36},6+std::sin(time*5),light,8);
    rect(v.x+27,v.y+31,4,9,White);
    if(b.state==BossState::Sweep) {
        auto hit=b.sweep();rect(hit.x-camera.x,hit.y-camera.y,hit.w,hit.h,alpha(Red,80));
        line(v+Vec{29,35},v+Vec{29+b.facing*120.f,55},6,Gold);
    }
}

void Renderer::hud(const Game& g) {
    rect(0,0,640,45,{5,12,21,150});
    for(int i=0;i<g.progress.maxHealth();++i) {
        float x=20+i*17.f;sf::Color c=i<g.player.hp?Gold:sf::Color(51,63,69);
        tri({x,11},{x+6,17},{x,25},c);tri({x,11},{x,25},{x-6,17},mix(c,Ink,.24f));
        if(i<g.player.hp) rect(x-1,13,2,6,White);
    }
    for(int i=0;i<3;++i) {rect(16+i*11.f,31,6,5,i<g.player.heals?Teal:sf::Color(44,58,67));}
    text("H  HEAL",55,31,1,Muted);
    text("ECHOES "+std::to_string(g.progress.shardCount())+" / 8",152,15,1,Muted);
    text("DASH",390,15,1,g.progress.has(Dash)?Teal:sf::Color(70,82,89));
    text("WINGS",430,15,1,g.progress.has(Wings)?Teal:sf::Color(70,82,89));
    text("BELL",478,15,1,g.progress.has(Bell)?Gold:sf::Color(70,82,89));
    if(g.progress.has(Dash)) rect(390,27,24*(1-g.player.dashCooldown/.52f),2,Teal);
    text("M MAP   ESC",545,15,1,Muted);
    if(g.roomTitleTime>0) {
        float a=std::min(1.f,g.roomTitleTime)*255;
        rect(20,230,32,2,alpha(Gold,a));
        text(g.rooms[g.room].name,20,241,1.7f,alpha(White,a));
        text(g.rooms[g.room].subtitle,20,260,.85f,alpha(Muted,a));
    }
    if(g.room==7 && g.boss.active) {
        text("THE HOLLOW WARDEN",320,52,1.25f,White,true);
        rect(165,66,310,5,{42,38,44});rect(165,66,310*g.boss.hp/42.f,5,g.boss.phase==2?Red:Gold);
        text(g.bossAction(),320,79,1,Gold,true);
    }
    const std::string prompt=g.prompt();
    if(!prompt.empty()) {
        float w=float(prompt.size())*6+20;panel(320-w/2,292,w,20);
        text(prompt,320,298,1,White,true);
    }
    if(g.noticeTime>0) {
        int lines=1+int(std::count(g.notice.begin(),g.notice.end(),'\n'));
        float h=16+lines*10.f;panel(24,354-h,592,h);
        text(g.notice,320,361-h,1,White,true);
    } else text(g.objective(),320,341,.85f,Muted,true);
    if(g.player.hp<=0) {
        rect(0,0,640,360,{7,13,23,175});
        text("A LIGHT NEVER TRULY GOES OUT",320,172,1.5f,White,true);
    }
}
void Renderer::map(const Game& g) {
    panel(26,28,588,300);text("MEMORY OF THE UNDERWORLD",320,48,2,White,true);
    text(g.objective(),320,76,1,Gold,true);
    auto center=[](const Room& r){return Vec{80+r.mapX*145.f,109+r.mapY*71.f};};
    const int edges[][2]={{0,1},{1,2},{2,4},{4,1},{1,3},{3,5},{5,1},{1,6},{6,7}};
    for(auto& e:edges) line(center(g.rooms[e[0]])+Vec{22,15},center(g.rooms[e[1]])+Vec{22,15},1,{49,70,78});
    for(int i=0;i<int(g.rooms.size());++i) {
        const auto& r=g.rooms[i];Vec p=center(r)-Vec{44,0};
        bool seen=(g.progress.visited&(1u<<i))!=0;
        rect(p.x,p.y,130,46,seen?sf::Color(30,54,61):sf::Color(16,30,39));
        rect(p.x,p.y,130,1,i==g.room?Gold:Muted);
        if(i==g.room) {rect(p.x,p.y,2,46,Gold);rect(p.x+128,p.y,2,46,Gold);}
        std::string label=seen?r.name:"UNEXPLORED";
        auto split=label.find(' ',7);
        if(label.size()>17 && split!=std::string::npos) label[split]='\n';
        text(label,p.x+65,p.y+9,.85f,seen?White:Muted,true);
        if(i==g.room) text("YOU",p.x+65,p.y+33,.85f,Gold,true);
        else if(seen&&!r.checkpoints.empty()) text("LANTERN",p.x+65,p.y+33,.7f,Teal,true);
    }
    text("M / ESC CLOSE     GOLD = CURRENT ROOM     E = USE DOOR",320,310,.9f,Muted,true);
}
void Renderer::controls() {
    panel(65,34,510,292);text("HOW TO CARRY THE LIGHT",320,55,2,White,true);
    const char* rows[][2]={{"A / D   OR   LEFT / RIGHT","MOVE"},{"SPACE   /   Z","JUMP - HOLD FOR HEIGHT"},
        {"J / X","ATTACK - THIRD HIT IS STRONGER"},{"UP + J","UPWARD ATTACK"},
        {"K / SHIFT","EMBER DASH - AFTER UNLOCKING"},{"SPACE IN MID-AIR","DOUBLE JUMP - AFTER UNLOCKING"},
        {"H (HOLD STILL)","HEAL 2 LIFE - USES A CHARGE"},{"E / W","DOOR / MEMORY / LANTERN"},
        {"M / TAB","MAP"},{"ESC / P","PAUSE"},{"F","MUTE / UNMUTE"}};
    for(int i=0;i<11;++i) {
        text(rows[i][0],87,93+i*16.f,.9f,Gold);
        text(rows[i][1],282,93+i*16.f,.9f,White);
    }
    text("LANTERNS RESTORE ALL LIFE AND HEALING CHARGES",320,282,.9f,Muted,true);
    text("ENTER / ESC RETURN",320,305,1,Teal,true);
}

void Renderer::draw(sf::RenderTarget& target,const Game& g,Screen screen,int selected,bool hasSave,bool muted) {
    mesh.clear();
    const bool title=screen==Screen::Title||screen==Screen::ConfirmNew;
    Vec originalCamera=camera;
    if(g.shake>0 && screen==Screen::Play) {camera.x+=std::sin(time*159)*g.shake;camera.y+=std::cos(time*131)*g.shake*.6f;}
    background(g,title);
    if(!title) {terrain(g);objects(g);}
    else {
        // Title tableau: the hero stands before the unlit bell.
        glow({505,230},100,Gold);
        line({506,0},{506,130},3,{75,90,88});
        tri({488,128},{470,215},{543,215},{61,75,80});
        tri({523,128},{470,215},{543,215},{61,75,80});
        rect(467,211,80,9,{125,115,91});circle({507,220},8,{180,142,87});
        circle({507,172},13,{19,36,46});circle({507,172},7,Gold);glow({507,172},50,Gold);
        rect(440,321,125,6,{86,112,112});hero({458,291},0,1,false);
        text("LUMENFALL",41,107,5.2f,White);
        text("THE HOLLOW BELL",44,155,1.3f,Gold);
        text("A SMALL LIGHT. A FORGOTTEN WORLD.",44,178,.95f,Muted);
        const char* menu[]={"CONTINUE","NEW JOURNEY","HOW TO PLAY","QUIT"};
        for(int i=0;i<4;++i) {
            sf::Color c=i==selected?White:Muted;
            if(i==0&&!hasSave) c={62,77,86};
            if(i==selected) {text(">",44,214+i*22.f,1.2f,Gold);rect(60,226+i*22.f,148,1,{83,85,74});}
            text(menu[i],64,214+i*22.f,1.2f,c);
        }
        text("ARROWS SELECT  /  ENTER",44,334,.85f,Muted);
        text("C++ / SFML 3",590,334,.8f,Muted,true);
    }
    camera=originalCamera;
    // A shallow vignette, preserving readable terrain and platform edges.
    for(int i=0;i<14;++i) {rect(0,float(i)*2,640,2,{2,8,14,std::uint8_t(30-i*2)});rect(0,358-i*2.f,640,2,{2,8,14,std::uint8_t(28-i*2)});}
    if(screen==Screen::Play) hud(g);
    if(screen==Screen::Map) {rect(0,0,640,360,{3,9,17,180});map(g);}
    if(screen==Screen::Controls) {rect(0,0,640,360,{3,9,17,185});controls();}
    if(screen==Screen::Pause) {
        rect(0,0,640,360,{3,9,17,165});panel(184,60,272,240);
        text("THE LIGHT RESTS",320,84,1.9f,White,true);
        const char* menu[]={"RESUME","MAP","HOW TO PLAY","RETURN TO TITLE","QUIT"};
        for(int i=0;i<5;++i) text((i==selected?"> ":"  ")+std::string(menu[i]),320,132+i*25.f,1.2f,i==selected?Gold:Muted,true);
        text("F - AUDIO "+std::string(muted?"OFF":"ON"),320,278,.9f,Muted,true);
    }
    if(screen==Screen::ConfirmNew) {
        rect(0,0,640,360,{3,9,17,170});panel(110,122,420,118);
        text("BEGIN A NEW JOURNEY?",320,145,1.7f,White,true);
        text("YOUR CURRENT SAVE WILL BE REPLACED",320,179,1,Gold,true);
        text("ENTER CONFIRM    /    ESC CANCEL",320,211,1,Muted,true);
    }
    if(screen==Screen::Ending) {
        rect(0,0,640,360,{8,19,29,215});glow({320,132},140,Gold);
        circle({320,112},22,Gold);circle({313,106},20,{32,44,50});
        text("AND THE WORLD REMEMBERED DAWN",320,168,1.65f,White,true);
        text("ONE SMALL LIGHT WAS ENOUGH.",320,195,1,Muted,true);
        const int mins=int(g.progress.playSeconds)/60,secs=int(g.progress.playSeconds)%60;
        std::ostringstream stats;stats<<"ECHOES "<<g.progress.shardCount()<<" / 8    TIME "<<mins<<":"<<std::setw(2)<<std::setfill('0')<<secs<<"    FALLS "<<g.progress.deaths;
        text(stats.str(),320,241,1,Gold,true);
        text("ENTER - RETURN TO TITLE",320,303,1,Muted,true);
    }
    target.draw(mesh);
}
} // namespace lumen
