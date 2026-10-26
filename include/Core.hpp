#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace lumen {
constexpr float Tick = 1.f / 120.f;
constexpr float HeroW = 20.f, HeroH = 28.f;
struct Vec {
    float x{}, y{};
    Vec operator+(Vec b) const { return {x+b.x, y+b.y}; }
    Vec operator-(Vec b) const { return {x-b.x, y-b.y}; }
    Vec operator*(float k) const { return {x*k, y*k}; }
};
struct Rect {
    float x{}, y{}, w{}, h{};
    float right() const { return x+w; }
    float bottom() const { return y+h; }
    Vec center() const { return {x+w*.5f, y+h*.5f}; }
    bool overlaps(Rect b) const {
        return x < b.right() && right() > b.x && y < b.bottom() && bottom() > b.y;
    }
};
inline float distance(Vec a, Vec b) { return std::hypot(a.x-b.x, a.y-b.y); }
inline int sign(float n) { return n < 0 ? -1 : 1; }
enum Ability : unsigned { None=0, Dash=1, Wings=2, Bell=4 };
struct Input {
    float move{};
    bool jump{}, jumpHeld{}, dash{}, attack{}, up{}, interact{}, heal{};
};
enum class ItemKind { Dash, Wings, Bell, Shard };
struct Item { Vec pos; ItemKind kind; int id; };
struct Portal { Vec pos; int target; Vec arrival; unsigned requirement{}; std::string label; };
struct Barrier { Rect box; int id; };
struct Sign { Vec pos; std::string text; };
enum class EnemyKind { Crawler, Moth, Sentinel };
struct EnemySpawn { Vec pos; EnemyKind kind; float left, right; };
struct Room {
    std::string name, subtitle;
    float width{}, height{};
    int mapX{}, mapY{}, theme{};
    std::vector<Rect> solids, hazards;
    std::vector<Portal> portals;
    std::vector<Vec> checkpoints;
    std::vector<Item> items;
    std::vector<Barrier> barriers;
    std::vector<EnemySpawn> enemies;
    std::vector<Sign> signs;
};
std::vector<Room> makeWorld();

struct Progress {
    unsigned abilities{}, shards{}, barriers{}, visited{1};
    int checkpointRoom{};
    Vec checkpoint{132,452};
    bool bossDefeated{};
    float playSeconds{};
    int deaths{};
    int shardCount() const;
    int maxHealth() const { return 5 + std::min(2, shardCount()/3); }
    bool has(unsigned bits) const { return (abilities & bits) == bits; }
};
struct Player {
    Vec pos{100,452}, vel{};
    int hp{5}, heals{3}, facing{1}, combo{}, attackId{};
    bool grounded{}, airJump{}, airDash{}, attackUp{};
    float coyote{}, jumpBuffer{}, dashTime{}, dashCooldown{}, attackTime{}, attackCooldown{};
    float comboTimer{}, invincible{}, hurtTimer{}, healTime{}, deadTime{};
    Rect box() const { return {pos.x,pos.y,HeroW,HeroH}; }
    Rect slash() const;
    bool attacking() const { return attackTime > .035f && attackTime < .20f; }
};
struct Enemy {
    Vec pos, vel;
    EnemyKind kind{};
    float left{}, right{}, phase{}, cooldown{1}, flash{};
    int hp{3}, facing{-1}, lastAttack{-1};
    Rect box() const;
};
enum class BossState { Sleeping, Intro, Idle, SweepTell, Sweep, DashTell, Charge,
    SlamTell, Airborne, Recover, RainTell, Rain, Enrage, Dead };
struct Boss {
    Vec pos{640,408}, start{}, target{};
    BossState state{BossState::Sleeping};
    float timer{}, duration{}, flash{};
    int hp{42}, facing{-1}, phase{1}, sequence{}, lastAttack{-1};
    bool active{}, landed{};
    Rect box() const { return {pos.x,pos.y,58,72}; }
    Rect sweep() const { return {facing>0?pos.x+40:pos.x-86, pos.y+15,104,57}; }
};
enum class ShotKind { Wave, Orb, Column };
struct Shot { Vec pos, vel; float life{}, delay{}; ShotKind kind; int damage{1}; };
enum class Effect { Jump, Dash, Slash, Hit, Hurt, Death, Heal, Chime, Break, Boom, Victory };
struct Cue { Effect effect; Vec pos; int value{}; };

// Pure gameplay model: no graphics/audio/device dependency. Fixed-step deterministic simulation.
class Game {
public:
    std::vector<Room> rooms{makeWorld()};
    Progress progress;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Shot> shots;
    Boss boss;
    int room{};
    float clock{}, roomTitleTime{4}, noticeTime{}, hitstop{}, shake{};
    std::string notice;
    std::vector<Cue> cues;
    bool saveRequested{}, ending{};

    void newGame();
    void restore(const Progress& p);
    void enter(int id, Vec spawn);
    void step(const Input& input, float dt=Tick);
    void interact();
    void hurt(int damage, Vec source);
    void respawn();
    void message(const std::string& text, float seconds=4);
    std::string objective() const;
    std::string prompt() const;
    std::string bossAction() const;
    bool doorOpen(const Portal& p) const { return progress.has(p.requirement); }
    bool barrierOpen(const Barrier& b) const { return (progress.barriers&(1u<<b.id))!=0; }
    bool itemTaken(const Item& item) const;

private:
    void movePlayer(const Input&, float);
    void updateEnemies(float);
    void updateBoss(float);
    void updateShots(float);
    void collect();
    void beginBoss(BossState, float duration);
    std::vector<Rect> collisionSolids() const;
};

std::filesystem::path defaultSavePath();
bool saveProgress(const std::filesystem::path&, const Progress&, std::string& error);
bool loadProgress(const std::filesystem::path&, Progress&, std::string& error);
} // namespace lumen
