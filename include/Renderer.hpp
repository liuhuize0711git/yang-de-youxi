#pragma once
#include "Core.hpp"
#include <SFML/Graphics.hpp>
#include <random>

namespace lumen {
enum class Screen { Title, Play, Pause, Map, Controls, Ending, ConfirmNew };
struct Particle { Vec pos,vel; float life{},maxLife{},size{}; sf::Color color; };
class Renderer {
public:
    Renderer();
    void update(const Game&,float dt);
    void feed(const Cue&);
    void draw(sf::RenderTarget&,const Game&,Screen,int selection,bool hasSave,bool muted);
    void snapCamera(const Game&);
private:
    sf::VertexArray mesh{sf::PrimitiveType::Triangles};
    std::vector<Particle> particles;
    std::mt19937 rng{82741};
    Vec camera{};
    float time{};
    int lastRoom{-1};
    void rect(float,float,float,float,sf::Color);
    void tri(Vec,Vec,Vec,sf::Color);
    void line(Vec,Vec,float,sf::Color);
    void circle(Vec,float,sf::Color,int segments=20);
    void glow(Vec,float,sf::Color);
    void text(std::string,float,float,float,sf::Color,bool centered=false);
    void background(const Game&,bool title);
    void terrain(const Game&);
    void objects(const Game&);
    void hero(Vec,float,int,bool,float alpha=1);
    void warden(const Game&);
    void hud(const Game&);
    void map(const Game&);
    void controls();
    void panel(float,float,float,float);
};
}
