#pragma once

// MP3 de um arquivo para PCM 16 bits estéreo 44,1 kHz num ring, lido por quem entrega o áudio (o
// callback do A2DP). Só existe quando o projeto traz a libhelix em lib_deps; sem ela este header e o
// .cpp ficam vazios e o fmk não paga flash nem RAM.
#if __has_include("libhelix-mp3/mp3dec.h") && __has_include(<FS.h>)
#define MRM_HAS_MP3_STREAM 1

#include <FS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/stream_buffer.h>

#include <atomic>

#include "Mp3Probe.h"
#include "Mp3Resample.h"

#ifndef MRM_MP3_RING_BYTES
#define MRM_MP3_RING_BYTES (16 * 1024) // PCM entre o decoder e o Bluetooth: 1 KB = 5,8 ms
#endif
#ifndef MRM_MP3_STACK_BYTES
#define MRM_MP3_STACK_BYTES (6 * 1024)
#endif
#ifndef MRM_MP3_IN_BYTES
#define MRM_MP3_IN_BYTES 4096 // bytes de MP3 lidos do arquivo; precisa caber um frame (até 1441)
#endif

namespace mrm {

// Uma task no core 1 decodifica (único escritor do ring) e read() só copia do ring (único leitor).
// Tudo estático: ring de 16 KB (~93 ms de áudio), pilha, fila de comandos e buffers. O helix aloca o
// estado dele uma vez em begin().
class Mp3Stream {
public:
    static constexpr uint32_t kRate = 44100; // saída para o Bluetooth; faixas a 48 kHz são convertidas
    static constexpr size_t kMaxPath = 128;

    // Diagnóstico do decoder (teste de banda); só a task escreve.
    struct Stats {
        uint32_t frames = 0;
        uint32_t busyUs = 0; // soma do tempo dentro do MP3Decode
        uint32_t maxUs = 0;
        uint32_t errors = 0; // frames descartados
    };

    bool begin(fs::FS& fs); // false se o helix não conseguiu memória
    // Troca a faixa: abre path, pula até info.dataOffset e decodifica. false se a taxa não for
    // 44,1 ou 48 kHz, o caminho não couber ou a fila estiver cheia.
    bool start(const char* path, const Mp3Info& info);
    void pause(); // read() entrega silêncio sem consumir o ring
    void resume();
    void stop(); // BLOQUEIA até a task fechar o arquivo (~20 ms): depois disso o arquivo pode sumir

    // Só para o callback do áudio. Sempre devolve len (silêncio onde não há música); (nullptr, -1)
    // da pilha A2DP ao esvaziar a fila devolve 0.
    int32_t read(uint8_t* data, int32_t len);

    uint32_t playedFrames() const; // da faixa atual, entregues ao callback
    bool finished() const;         // arquivo acabou e o ring esvaziou
    uint32_t underruns() const { return underruns_; }
    uint32_t buffered() const { return written_.load() - read_.load(); } // bytes no ring, inclui faixa velha
    const Stats& stats() const { return stats_; }
    uint32_t stackFree() const; // bytes de pilha que a task nunca usou

private:
    enum class Op : uint8_t { Play,
                              Pause,
                              Resume,
                              Stop };
    struct Cmd {
        Op op;
        uint32_t seq;
        uint32_t offset;
        uint32_t rate; // taxa do arquivo
        char path[kMaxPath];
    };
    // Buffers estáticos: o device encolhe com -D quando a RAM aperta (o Bluetooth precisa de folga no heap).
    static constexpr size_t kRing = MRM_MP3_RING_BYTES;
    static constexpr size_t kStack = MRM_MP3_STACK_BYTES;
    static constexpr size_t kQueue = 4;
    static constexpr size_t kIn = MRM_MP3_IN_BYTES;
    static constexpr size_t kPcm = 1152 * 2; // amostras de um frame MPEG-1 estéreo

    static void entry(void* self);
    void run();
    bool send(Op op, const char* path = "", uint32_t offset = 0, uint32_t rate = kRate);
    bool poll();
    void apply(const Cmd& c);
    void decode();
    void finish();
    void refill();
    bool push(const uint8_t* p, size_t n);

    fs::FS* fs_ = nullptr;
    void* dec_ = nullptr;
    TaskHandle_t task_ = nullptr;
    StreamBufferHandle_t ring_ = nullptr;
    QueueHandle_t queue_ = nullptr;

    File file_;
    uint8_t* inPtr_ = in_;
    int inLeft_ = 0;
    bool fileDone_ = false;
    bool paused_ = false;
    uint16_t badRun_ = 0; // erros seguidos: arquivo corrompido vira fim
    Stats stats_;
    uint32_t rate_ = kRate; // da faixa atual; só a task
    Resample48to44 resample_;

    uint32_t seq_ = 0; // só o loop
    std::atomic<uint32_t> ack_{0};
    std::atomic<uint32_t> written_{0};  // bytes que a task pôs no ring (só a task)
    std::atomic<uint32_t> read_{0};     // bytes que read() tirou do ring (só o callback)
    std::atomic<uint32_t> boundary_{0}; // written_ no início da faixa: antes disso e faixa velha
    std::atomic<bool> playing_{false};
    std::atomic<bool> silent_{false}; // pausado: read() não consome
    std::atomic<bool> primed_{false}; // ring com folga para começar sem engasgo
    std::atomic<bool> eof_{false};
    std::atomic<uint32_t> underruns_{0};

    uint8_t in_[kIn];
    int16_t pcm_[kPcm];
    uint8_t ringStore_[kRing + 1];
    StaticStreamBuffer_t ringBuf_;
    uint8_t queueStore_[kQueue * sizeof(Cmd)];
    StaticQueue_t queueBuf_;
    StackType_t stack_[kStack / sizeof(StackType_t)];
    StaticTask_t tcb_;
};

} // namespace mrm

#endif
