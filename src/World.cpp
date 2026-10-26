#include "Core.hpp"

namespace lumen {
std::vector<Room> makeWorld() {
    std::vector<Room> r(8);
    auto base=[&](int i,const char* name,const char* sub,float w,float h,int mx,int my,int theme) {
        r[i].name=name; r[i].subtitle=sub; r[i].width=w; r[i].height=h;
        r[i].mapX=mx; r[i].mapY=my; r[i].theme=theme;
    };
    base(0,"THE WAKING WELL","A small light in a world that forgot the sun",1240,540,0,1,0);
    r[0].solids={{0,480,570,60},{700,480,540,60},{545,436,75,14},{660,416,82,14},
        {300,424,100,16},{412,368,80,16},{526,312,90,16}};
    r[0].hazards={{574,514,124,26}};
    r[0].checkpoints={{142,480}};
    r[0].portals={{{1180,480},1,{60,632},None,"LANTERN CROSSROADS"}};
    r[0].items={{{567,290},ItemKind::Shard,0}};
    r[0].enemies={{{845,458},EnemyKind::Crawler,790,1040}};
    r[0].signs={{{235,480},"A / D MOVE     SPACE JUMP\nHOLD JUMP TO REACH HIGHER"},
        {{465,480},"J STRIKES THE DARKNESS\nUP + J STRIKES ABOVE"},
        {{935,480},"E AT A LANTERN: REST + SAVE\nHOLD H TO SPEND A HEALING CHARGE"}};

    base(1,"LANTERN CROSSROADS","Every road remembers where it began",1120,720,1,1,0);
    r[1].solids={{0,660,1120,60},{220,600,130,16},{365,544,110,16},
        {225,488,112,16},{70,376,160,20},{520,604,120,16},{690,548,100,16}};
    r[1].checkpoints={{490,660}};
    r[1].portals={{{42,660},0,{1130,452},None,"THE WAKING WELL"},
        {{880,660},2,{60,452},None,"MOSS CISTERN"},
        {{140,376},3,{70,572},Wings,"HANGING GARDENS"},
        {{1065,660},6,{60,452},Bell,"BRIDGE OF ASH"}};
    r[1].items={{{730,526},ItemKind::Shard,1}};
    r[1].signs={{{585,660},"THE GARDEN HANGS ABOVE\nTHE CISTERN SLEEPS TO THE EAST"},
        {{995,660},"THE GREAT BELL SEALS THIS WAY\nFIND ITS HEART IN THE FOUNDRY"}};

    base(2,"MOSS CISTERN","Roots drink the last echoes of the rain",1400,540,1,2,1);
    r[2].solids={{0,480,360,60},{475,480,230,60},{830,480,570,60},
        {335,432,85,14},{445,400,80,14},{675,426,80,14},{790,410,80,14},
        {930,422,90,14},{1060,366,80,14}};
    r[2].hazards={{365,516,106,24},{710,516,114,24}};
    r[2].checkpoints={{575,480}};
    r[2].items={{{965,402},ItemKind::Dash,0},{{1100,344},ItemKind::Shard,2}};
    r[2].barriers={{{1170,250,24,230},0}};
    r[2].portals={{{45,480},1,{825,632},None,"LANTERN CROSSROADS"},
        {{1340,480},4,{65,452},Dash,"HUSH ARCHIVE"}};
    r[2].enemies={{{230,458},EnemyKind::Crawler,190,320},
        {{650,370},EnemyKind::Moth,540,700},{{1000,458},EnemyKind::Crawler,860,1140}};
    r[2].signs={{{1120,480},"K / SHIFT: DASH THROUGH THE VEIL\nA DASH TURNS HARM INTO LIGHT"}};

    base(3,"HANGING GARDENS","The sky is only a memory of blue",1260,760,1,0,1);
    r[3].solids={{0,600,300,160},{170,540,100,16},{300,480,100,16},
        {430,420,100,16},{280,308,100,16},{430,250,120,18},
        {605,310,100,16},{740,370,100,16},{865,430,120,16},{1000,490,260,270},
        {650,680,160,80}};
    r[3].hazards={{305,738,690,22}};
    r[3].portals={{{55,600},1,{140,348},Wings,"LANTERN CROSSROADS"},
        {{1190,490},5,{65,452},None,"BELL FOUNDRY"}};
    r[3].items={{{487,228},ItemKind::Shard,3},{{730,658},ItemKind::Shard,4}};
    // The lower secret shelf can always be escaped with the acquired double jump.
    r[3].solids.push_back({830,580,95,16});
    r[3].enemies={{{350,400},EnemyKind::Moth,240,510},{{790,290},EnemyKind::Moth,650,870}};
    r[3].signs={{{90,600},"SPACE AGAIN IN MID-AIR\nTHE SECOND LEAP REMEMBERS THE SKY"}};

    base(4,"HUSH ARCHIVE","Some stories wait for someone to return",1310,540,2,2,2);
    r[4].solids={{0,480,420,60},{635,480,675,60},{350,420,95,16},
        {730,420,100,16},{850,358,120,16},{1010,304,150,20},
        {1170,420,100,16}};
    r[4].hazards={{425,508,205,32}};
    r[4].checkpoints={{710,480}};
    r[4].items={{{1100,282},ItemKind::Wings,0},{{1220,398},ItemKind::Shard,5}};
    r[4].portals={{{45,480},2,{1290,452},Dash,"MOSS CISTERN"},
        {{1250,480},1,{570,632},Wings,"CROSSROADS SHORTCUT"}};
    r[4].enemies={{{240,458},EnemyKind::Crawler,170,350},
        {{890,440},EnemyKind::Sentinel,840,1020}};
    r[4].signs={{{300,480},"JUMP THEN DASH ACROSS THE CHASM\nTHE LANTERN ON THE FAR SIDE WILL WAIT"}};

    base(5,"BELL FOUNDRY","A heart of brass still beats below the ash",1320,540,2,0,3);
    r[5].solids={{0,480,430,60},{550,480,280,60},{950,480,370,60},
        {405,423,100,16},{520,360,100,16},{670,300,100,16},
        {810,360,100,16},{925,416,100,16},{1090,355,130,18}};
    r[5].hazards={{435,512,110,28},{835,512,110,28}};
    r[5].checkpoints={{210,480}};
    r[5].items={{{1155,332},ItemKind::Bell,0},{{720,278},ItemKind::Shard,6}};
    r[5].portals={{{45,480},3,{1140,462},None,"HANGING GARDENS"},
        {{1260,480},1,{1015,632},Bell,"CROSSROADS LIFT"}};
    r[5].enemies={{{340,440},EnemyKind::Sentinel,280,395},
        {{695,458},EnemyKind::Crawler,580,780},{{1080,440},EnemyKind::Sentinel,990,1210}};

    base(6,"BRIDGE OF ASH","Beyond this light there is no turning back in battle",1100,540,2,1,3);
    r[6].solids={{0,480,320,60},{520,480,580,60},{265,418,110,18},
        {480,418,110,18},{640,355,110,18}};
    r[6].hazards={{325,515,190,25}};
    r[6].checkpoints={{815,480}};
    r[6].items={{{695,333},ItemKind::Shard,7}};
    r[6].portals={{{45,480},1,{1010,632},Bell,"LANTERN CROSSROADS"},
        {{1040,480},7,{195,452},Bell,"HEART OF THE BELL"}};
    r[6].signs={{{885,480},"THE WARDEN TELEGRAPHS EVERY STRIKE\nJUMP THE WAVES. DASH THROUGH THE CHARGE.\nSTRIKE WHILE ITS LIGHT IS DIM."}};

    base(7,"HEART OF THE BELL","Wake the light. Break the silence.",960,540,3,1,4);
    r[7].solids={{0,480,960,60},{285,385,95,14},{580,385,95,14}};
    r[7].portals={{{195,480},6,{975,452},None,"BRIDGE OF ASH"}};
    return r;
}
}
