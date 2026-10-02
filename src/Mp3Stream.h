#pragma once

// MP3 de um arquivo para PCM 16 bits estereo 44,1 kHz num ring, lido por quem entrega o audio (o
// callback do A2DP). So existe quando o projeto traz a libhelix em lib_deps; sem ela este header e o
// .cpp ficam vazios e o fmk nao paga flash nem RAM.
#if __has_include("libhelix-mp3/mp3dec.h") && __has_include(<FS.h>)
#define MRM_HAS_MP3_STREAM 1

#include <FS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/stream_buffer.h>

#include <atomic>

#include "Mp3Probe.h"

namespace mrm {

// Uma task no core 1 decodifica (unico escritor do ring) e read() so copia do ring (unico leitor).
// Tudo estatico: ring de 16 KB (~93 ms de audio), pilha, fila de comandos e buffers. O helix aloca o
// estado dele uma vez em begin().
class Mp3Stream {
public:
    static constexpr uint32_t kRate = 44100;
    static constexpr size_t kMaxPath = 128;

    // Diagnostico do decoder (teste de banda); so a task escreve.
    struct Stats {
        uint32_t frames = 0;
        uint32_t busyUs = 0; // soma do tempo dentro do MP3Decode
        uint32_t maxUs = 0;
        uint32_t errors = 0; // frames descartados
    };

    bool begin(fs::FS& fs); // false se o helix nao conseguiu memoria
    // Troca a faixa: abre path, pula ate info.dataOffset e decodifica. false se a taxa nao for
    // 44,1 kHz, o caminho nao couber ou a fila estiver cheia.
    bool start(const char* path, const Mp3Info& info);
    void pause(); // read() entrega silencio sem consumir o ring
    void resume();
    void stop(); // BLOQUEIA ate a task fechar o arquivo (~20 ms): depois disso o arquivo pode sumir

    // So para o callback do audio. Sempre devolve len (silencio onde nao ha musica); (nullptr, -1)
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
        char path[kMaxPath];
    };
    static constexpr size_t kRing = 16 * 1024;
    static constexpr size_t kStack = 6 * 1024;
    static constexpr size_t kQueue = 4;
    static constexpr size_t kIn = 4096;
    static constexpr size_t kPcm = 1152 * 2; // amostras de um frame MPEG-1 estereo

    static void entry(void* self);
    void run();
    bool send(Op op, const char* path = "", uint32_t offset = 0);
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

    uint32_t seq_ = 0; // so o loop
    std::atomic<uint32_t> ack_{0};
    std::atomic<uint32_t> written_{0};  // bytes que a task pos no ring (so a task)
    std::atomic<uint32_t> read_{0};     // bytes que read() tirou do ring (so o callback)
    std::atomic<uint32_t> boundary_{0}; // written_ no inicio da faixa: antes disso e faixa velha
    std::atomic<bool> playing_{false};
    std::atomic<bool> silent_{false}; // pausado: read() nao consome
    std::atomic<bool> primed_{false}; // ring com folga para comecar sem engasgo
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
