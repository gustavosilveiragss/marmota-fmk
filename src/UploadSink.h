#pragma once

#if __has_include(<FS.h>)

#include <FS.h>

#include <cstddef>
#include <cstdint>

#include "UploadVerdict.h"

namespace mrm {

// Recebe um arquivo em fatias (corpo cru de um POST): grava num tmp, valida o conteúdo se pedido e
// só no finish() faz rename para dir/name. Nunca sobrescreve e apaga o tmp em qualquer erro.
class UploadSink {
public:
    enum class Error : uint8_t { None, Name, Space, Exists, Io, Rejected, Aborted };
    using Verdict = UploadVerdict;

    using FreeFn = uint64_t (*)();                                 // bytes livres no FS
    using CheckFn = Verdict (*)(void* ctx, const uint8_t* data, size_t size); // fatias em ordem

    struct Config {
        const char* tmpPath = "/music/.up.tmp";
        uint32_t reserve = 64 * 1024; // livre que sobra depois do arquivo (config.json no mesmo FS)
        FreeFn freeBytes = nullptr;   // nullptr: não confere espaço
        CheckFn check = nullptr; // nullptr: aceita qualquer conteúdo
        void* checkCtx = nullptr;
    };

    struct Target {
        const char* dir; // pasta de destino, já validada pelo chamador
        const char* name;
        uint32_t size; // tamanho declarado
    };

    UploadSink() = default;
    explicit UploadSink(const Config& config)
        : config_(config) {}

    Error start(fs::FS& fs, const Target& target);
    Error write(const uint8_t* data, size_t size);
    Error finish();
    void abort();

    uint32_t written() const { return written_; }

    // Apaga o tmp que sobrou de queda de energia.
    static void sweep(fs::FS& fs, const char* tmpPath);

private:
    static constexpr size_t kPathMax = 128;

    Error fail(Error error);
    Error checkContent(const uint8_t* data, size_t size);

    Config config_;
    fs::FS* fs_ = nullptr;
    File file_;
    char dest_[kPathMax] = "";
    uint32_t size_ = 0;
    uint32_t written_ = 0;
    Verdict verdict_ = Verdict::Need;
    bool open_ = false;
};

} // namespace mrm

#endif
