#include "Mp3Stream.h"

#ifdef MRM_HAS_MP3_STREAM

#include <Arduino.h>
#include <cstring>

#include "Log.h"
#include "libhelix-mp3/mp3dec.h"

namespace mrm {

namespace {

constexpr UBaseType_t kPriority = 3;
constexpr BaseType_t kCore = 1;
constexpr TickType_t kSendWait = pdMS_TO_TICKS(20);
constexpr uint32_t kStopWaitMs = 200;
constexpr uint32_t kPrime = 8 * 1024; // ~46 ms no ring antes de soltar a faixa
constexpr uint16_t kMaxBadRun = 200; // frames ruins seguidos

} // namespace

bool Mp3Stream::begin(fs::FS& fs) {
    if (task_)
        return true;
    fs_ = &fs;
    dec_ = MP3InitDecoder();
    if (!dec_) {
        log("Mp3Stream: helix sem memoria");
        return false;
    }
    ring_ = xStreamBufferCreateStatic(kRing, 1, ringStore_, &ringBuf_);
    queue_ = xQueueCreateStatic(kQueue, sizeof(Cmd), queueStore_, &queueBuf_);
    task_ = xTaskCreateStaticPinnedToCore(entry, "mp3dec", kStack, this, kPriority, stack_, &tcb_, kCore);
    return true;
}

bool Mp3Stream::send(Op op, const char* path, uint32_t offset, uint32_t rate) {
    if (!task_)
        return false;
    Cmd c{op, seq_ + 1, offset, rate, {}};
    strlcpy(c.path, path, sizeof(c.path));
    if (xQueueSend(queue_, &c, kSendWait) != pdTRUE)
        return false;
    seq_ = c.seq;
    return true;
}

bool Mp3Stream::start(const char* path, const Mp3Info& info) {
    if ((info.sampleRate != kRate && info.sampleRate != Resample48to44::kInRate) || strlen(path) >= kMaxPath)
        return false;
    return send(Op::Play, path, info.dataOffset, info.sampleRate);
}

void Mp3Stream::pause() {
    send(Op::Pause);
}

void Mp3Stream::resume() {
    send(Op::Resume);
}

void Mp3Stream::stop() {
    if (!send(Op::Stop))
        return;
    const uint32_t t0 = millis();
    while (ack_.load() != seq_ && millis() - t0 < kStopWaitMs)
        vTaskDelay(1);
}

uint32_t Mp3Stream::playedFrames() const {
    if (ack_.load() != seq_ || !playing_)
        return 0;
    const int32_t d = static_cast<int32_t>(read_.load() - boundary_.load());
    return d > 0 ? d / 4 : 0;
}

bool Mp3Stream::finished() const {
    return ack_.load() == seq_ && playing_ && eof_ && read_.load() == written_.load();
}

uint32_t Mp3Stream::stackFree() const {
    return task_ ? uxTaskGetStackHighWaterMark(task_) : 0;
}

// Roda na task do A2DP: nada de bloquear. Leituras sempre em multiplos de 4 (um frame estereo) e o
// ring tem capacidade multipla de 4, entao as escritas parciais tambem ficam alinhadas.
int32_t Mp3Stream::read(uint8_t* data, int32_t len) {
    if (!data || len <= 0)
        return 0;
    const size_t want = static_cast<size_t>(len) & ~size_t(3);
    uint32_t r = read_.load(std::memory_order_relaxed);
    for (;;) { // descarta o que sobrou da faixa anterior
        const int32_t stale = static_cast<int32_t>(boundary_.load(std::memory_order_acquire) - r);
        if (stale <= 0)
            break;
        const size_t n = xStreamBufferReceive(ring_, data, min<size_t>(stale, want), 0);
        if (n == 0)
            break;
        r += n;
    }
    size_t got = 0;
    if (playing_ && !silent_ && primed_) {
        got = xStreamBufferReceive(ring_, data, want, 0);
        r += got;
        if (got < want && !eof_)
            underruns_.fetch_add(1, std::memory_order_relaxed);
    }
    read_.store(r, std::memory_order_release);
    memset(data + got, 0, len - got);
    return len;
}

void Mp3Stream::entry(void* self) {
    static_cast<Mp3Stream*>(self)->run();
}

void Mp3Stream::run() {
    for (;;) {
        if (!file_ || eof_) {
            Cmd c;
            if (xQueueReceive(queue_, &c, portMAX_DELAY) == pdTRUE)
                apply(c);
            continue;
        }
        if (poll())
            decode();
    }
}

// Comandos entre frames e durante o envio. Pausa segura a task aqui ate o proximo comando; true se a
// faixa atual segue (play e stop trocam o arquivo e o resto do frame vai fora).
bool Mp3Stream::poll() {
    Cmd c;
    while (xQueueReceive(queue_, &c, paused_ ? portMAX_DELAY : 0) == pdTRUE) {
        apply(c);
        if (c.op == Op::Play || c.op == Op::Stop)
            return false;
    }
    return true;
}

void Mp3Stream::apply(const Cmd& c) {
    switch (c.op) {
    case Op::Pause:
        paused_ = true;
        silent_ = true;
        break;
    case Op::Resume:
        paused_ = false;
        silent_ = false;
        break;
    case Op::Play:
    case Op::Stop:
        playing_ = false;
        boundary_.store(written_.load(), std::memory_order_release);
        eof_ = false;
        primed_ = false;
        paused_ = false;
        silent_ = false;
        badRun_ = 0;
        inPtr_ = in_;
        inLeft_ = 0;
        fileDone_ = true;
        if (file_)
            file_.close();
        if (c.op == Op::Play) {
            rate_ = c.rate;
            resample_.reset();
            file_ = fs_->open(c.path, "r");
            fileDone_ = !file_ || !file_.seek(c.offset);
            eof_ = fileDone_; // arquivo sumiu: a faixa termina na hora e o player segue
            playing_ = true;
        }
        break;
    }
    ack_.store(c.seq);
}

void Mp3Stream::refill() {
    if (fileDone_ || inLeft_ >= MAINBUF_SIZE)
        return;
    memmove(in_, inPtr_, inLeft_);
    inPtr_ = in_;
    const int n = file_.read(in_ + inLeft_, kIn - inLeft_);
    if (n <= 0)
        fileDone_ = true;
    else
        inLeft_ += n;
}

void Mp3Stream::finish() {
    fileDone_ = true;
    inLeft_ = 0;
    primed_ = true;
    eof_ = true;
}

void Mp3Stream::decode() {
    refill();
    const int sync = inLeft_ > 0 ? MP3FindSyncWord(inPtr_, inLeft_) : -1;
    if (sync < 0) {
        if (fileDone_)
            return finish();
        inPtr_ += inLeft_ - 1; // guarda o ultimo byte: pode ser o comeco de um sync
        inLeft_ = 1;
        return;
    }
    inPtr_ += sync;
    inLeft_ -= sync;
    uint8_t* before = inPtr_;
    const uint32_t t0 = micros();
    const int err = MP3Decode(dec_, &inPtr_, &inLeft_, pcm_, 0);
    const uint32_t us = micros() - t0;
    MP3FrameInfo fi;
    MP3GetLastFrameInfo(dec_, &fi);
    if (err == ERR_MP3_MAINDATA_UNDERFLOW)
        return; // normal nos primeiros frames: o reservatorio de bits ainda esta vazio
    if (err == ERR_MP3_INDATA_UNDERFLOW && fileDone_)
        return finish(); // ultimo frame cortado
    if (err != ERR_MP3_NONE || fi.samprate != static_cast<int>(rate_) || fi.outputSamps <= 0) {
        ++stats_.errors;
        if (inPtr_ == before) { // nao andou: pula o sync falso
            ++inPtr_;
            --inLeft_;
        }
        if (++badRun_ > kMaxBadRun)
            finish(); // arquivo corrompido: vira fim de faixa
        return;
    }
    badRun_ = 0;
    ++stats_.frames;
    stats_.busyUs += us;
    stats_.maxUs = max(stats_.maxUs, us);
    size_t samples = fi.outputSamps;
    if (fi.nChans == 1) { // mono: duplica de tras para frente no mesmo buffer
        for (size_t i = samples; i-- > 0;)
            pcm_[2 * i] = pcm_[2 * i + 1] = pcm_[i];
        samples *= 2;
    }
    if (rate_ == kRate) {
        push(reinterpret_cast<const uint8_t*>(pcm_), samples * sizeof(int16_t));
        return;
    }
    bool pushing = true; // push() devolve false quando um comando interrompe: o resto do frame se perde
    resample_.feed(pcm_, samples / 2, [&](const int16_t* out, size_t frames) {
        if (pushing)
            pushing = push(reinterpret_cast<const uint8_t*>(out), frames * 2 * sizeof(int16_t));
    });
}

// Bloqueia no ring cheio em fatias de 20 ms para atender comandos; nunca gira em vazio.
bool Mp3Stream::push(const uint8_t* p, size_t n) {
    while (n > 0) {
        const size_t k = xStreamBufferSend(ring_, p, n, kSendWait);
        written_.fetch_add(k, std::memory_order_release);
        p += k;
        n -= k;
        if (written_.load() - boundary_.load() >= kPrime)
            primed_ = true;
        if (!poll())
            return false;
    }
    return true;
}

} // namespace mrm

#endif
