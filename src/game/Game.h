#pragma once

// Os jogos moram no fmk porque varios projetos vao usa-los: o motor (Game, GameHost, GameStore,
// GameView) e as telas de tutorial, contagem, pausa e fim sao genericos. Cada esquema de entrada
// tem a sua subpasta de jogos (twobtn para 2 botoes) e o projeto escolhe qual incluir.

#include "../Locale.h"
#include "../Ssd1306Display.h"

namespace mrm {
namespace game {

// Entrada de um passo de simulacao: o que as duas teclas estao fazendo agora.
struct GameInput {
    bool a;      // A segurado
    bool b;      // B segurado
    bool aPress; // A acabou de ser apertado
    bool bPress; // B acabou de ser apertado
};

// Textos que o tutorial mostra: nome, objetivo e o que cada tecla faz neste jogo.
struct GameText {
    Text name;
    Text goal;
    Text keyA;
    Text keyB;
    Text rules[3]; // pagina 2 do tutorial: as regras em 3 linhas
};

enum class GamePhase : uint8_t { Tutorial,
                                 Countdown,
                                 Playing,
                                 Paused,
                                 Over };

constexpr uint8_t kTutorialPages = 2; // controles e regras
constexpr int16_t kFieldTop = 12;     // abaixo da faixa de pontos
constexpr uint32_t kCountdownMs = 1500;

inline uint32_t nextRandom(uint32_t& state) { // xorshift32: sem estado global nem biblioteca
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

// Um jogo e so simulacao e desenho: nao conhece menu, tutorial nem pausa (isso e do GameHost).
// A simulacao anda em passos fixos de kTickMs, independente da taxa de quadros.
class Game {
public:
    static constexpr uint32_t kTickMs = 33;
    static constexpr uint8_t kLitA = 1; // mascara devolvida por demo()
    static constexpr uint8_t kLitB = 2;

    virtual const GameText& text() const = 0;
    virtual void reset(uint32_t seed) = 0;
    virtual void tick(const GameInput& in) = 0;
    virtual void draw(Panel& o, uint32_t now) const = 0;
    // Mini cena em loop do tutorial, na faixa de y 26 a 48. Devolve as teclas que acende.
    virtual uint8_t demo(Panel& o, uint32_t now) const = 0;
    virtual bool over() const = 0;
    virtual uint16_t score() const = 0;

protected:
    ~Game() = default;
};

// Faixa de pontos no topo de um jogo: vidas a esquerda (bolinhas) e pontuacao a direita.
void hud(Panel& o, uint16_t score, uint8_t lives);

} // namespace game
} // namespace mrm
