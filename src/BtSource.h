#pragma once

// Fone Bluetooth como destino de áudio (A2DP source): varredura com lista, conexão por BDA, estado e
// desligamento de verdade. Só existe quando o projeto traz a lib ESP32-A2DP em lib_deps; sem ela
// este header e o .cpp ficam vazios e o fmk não paga flash.
#if __has_include("BluetoothA2DPSource.h")
#define MRM_HAS_BT_SOURCE 1

#include <BluetoothA2DPSource.h>

#include <atomic>

namespace mrm {

class BtSource : public BluetoothA2DPSource {
public:
    static constexpr uint8_t kMaxFound = 8;
    static constexpr uint8_t kNameLen = 32;

    enum class State : uint8_t { Off,
                                 Idle,
                                 Scanning,
                                 Connecting,
                                 Connected, // link de pé, ainda sem áudio
                                 Streaming, // fone aceitou o áudio
                                 Failed };  // última conexão não fechou; vale até o próximo scan ou connect

    struct Found {
        uint8_t bda[6];
        char name[kNameLen + 1];
        int8_t rssi;
    };

    // Liga o rádio. A pilha sobe em outra task e começa uma varredura sozinha (a lib não deixa
    // pular); o dado enviado ao fone e silêncio até set_data_callback() trocar. O callback trocado
    // também recebe (nullptr, -1) quando a pilha esvazia a fila e deve devolver 0. Auto-reconexão
    // da lib fica desligada: quem guarda o BDA do último fone e o chamador.
    void start() override;
    // Desliga de verdade (o heap volta). BLOQUEIA: com tentativa de conexão no ar espera o page
    // terminar (até ~5,2 s); end() espera a desconexão por até ~2,3 s (A2DP_DISCONNECT_LIMIT x 100 ms)
    // e leva ~0,5 s de pausas fixas mesmo sem fone.
    void stop();

    // Chamar do loop(): esvazia a fila do callback do GAP na lista e vigia timeouts.
    void update();

    // Uma varredura de 12,8 s, sem relancar sozinha. Zera a lista. false se desligado ou com link.
    bool scan();
    // Conecta por BDA; se estiver varrendo, a varredura e cancelada antes. false se não puder.
    bool connect(const uint8_t bda[6]);
    // Desfaz o link ou cancela a tentativa. A lib mexe só no próprio BDA, não no NVS.
    void disconnect() override;

    State state() const { return state_; }
    uint8_t foundCount() const { return foundCount_; }
    const Found& found(uint8_t i) const { return found_[i]; }
    // Achados descartados porque a fila do GAP encheu (diagnóstico).
    uint16_t dropped() const { return dropped_; }

protected:
    void app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) override;
    // A lib chama isto a cada 10 s e usa para reconectar sozinha e iniciar a mídia; só deixamos passar
    // com link de pé (durante Connecting um ack de media enganaria a máquina de estados dela).
    void a2d_app_heart_beat(void* arg) override;

private:
    struct Event {
        uint8_t bda[6];
        int8_t rssi;
        char name[kNameLen + 1];
    };
    static constexpr uint8_t kQueue = 16;

    static void onConnection(esp_a2d_connection_state_t st, void* self);
    static void onAudio(esp_a2d_audio_state_t st, void* self);

    void onFound(const esp_bt_gap_cb_param_t::disc_res_param& r);
    void onDiscovery(esp_bt_gap_discovery_state_t st);
    void push(const Event& e);
    void addFound(const Event& e);
    bool beginDiscovery();
    void connectNow();

    std::atomic<State> state_{State::Off};
    std::atomic<bool> ready_{false};    // a pilha subiu (1o evento de varredura)
    std::atomic<bool> scanLive_{false}; // inquiry em andamento no controlador
    std::atomic<bool> connectPending_{false};
    std::atomic<bool> rescan_{false};
    std::atomic<bool> dropExpected_{false}; // desconexão pedida por nos: o evento não e queda do fone
    std::atomic<bool> paging_{false};       // connect_to no ar até o evento CONNECTED ou DISCONNECTED
    std::atomic<uint16_t> dropped_{0};

    Event queue_[kQueue];
    std::atomic<uint8_t> head_{0};
    std::atomic<uint8_t> tail_{0};

    Found found_[kMaxFound];
    uint8_t foundCount_ = 0;
    uint32_t stateAt_ = 0; // millis da última entrada em Scanning ou Connecting
};

} // namespace mrm

#endif
