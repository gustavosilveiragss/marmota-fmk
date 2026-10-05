#include "UploadSink.h"

#if __has_include(<FS.h>)

#include <cstdio>

#include "PathRules.h"

namespace mrm {

using Error = UploadSink::Error;
using Verdict = UploadSink::Verdict;

Error UploadSink::fail(Error error) {
    abort();
    return error;
}

void UploadSink::abort() {
    if (!fs_)
        return;

    if (file_)
        file_.close();
    fs_->remove(config_.tmpPath);
    open_ = false;
}

void UploadSink::sweep(fs::FS& fs, const char* tmpPath) {
    if (fs.exists(tmpPath))
        fs.remove(tmpPath);
}

Error UploadSink::start(fs::FS& fs, const Target& target) {
    abort();
    fs_ = &fs;
    written_ = 0;
    size_ = target.size;
    verdict_ = config_.check ? Verdict::Need : Verdict::Ok;
    if (!validTrackName(target.name) || !target.dir || target.dir[0] != '/')
        return Error::Name;

    if (snprintf(dest_, sizeof(dest_), "%s/%s", target.dir, target.name) >= static_cast<int>(sizeof(dest_)))
        return Error::Name;

    if (fs.exists(dest_))
        return Error::Exists;

    if (config_.freeBytes) {
        const uint64_t avail = config_.freeBytes();
        if (avail < static_cast<uint64_t>(target.size) + config_.reserve)
            return Error::Space;
    }

    if (!fs.exists(target.dir) && !fs.mkdir(target.dir))
        return Error::Io;
    file_ = fs.open(config_.tmpPath, FILE_WRITE);
    if (!file_)
        return Error::Io;
    open_ = true;
    return Error::None;
}

Error UploadSink::checkContent(const uint8_t* data, size_t size) {
    if (verdict_ != Verdict::Need)
        return Error::None;
    verdict_ = config_.check(config_.checkCtx, data, size);
    return verdict_ == Verdict::Bad ? Error::Rejected : Error::None;
}

Error UploadSink::write(const uint8_t* data, size_t size) {
    if (!open_)
        return Error::Aborted;

    if (size > size_ - written_)
        return fail(Error::Rejected);

    if (checkContent(data, size) != Error::None)
        return fail(Error::Rejected);

    if (file_.write(data, size) != size)
        return fail(Error::Io);
    written_ += static_cast<uint32_t>(size);
    return Error::None;
}

Error UploadSink::finish() {
    if (!open_)
        return Error::Aborted;

    if (written_ != size_)
        return fail(Error::Rejected);

    if (verdict_ == Verdict::Need)
        return fail(Error::Rejected);
    file_.flush();
    const bool whole = file_.size() == size_; // close() não avisa de erro de gravação no fim
    file_.close();

    if (!whole)
        return fail(Error::Io);

    if (fs_->exists(dest_))
        return fail(Error::Exists);

    if (!fs_->rename(config_.tmpPath, dest_))
        return fail(Error::Io);
    open_ = false;
    return Error::None;
}

} // namespace mrm

#endif
