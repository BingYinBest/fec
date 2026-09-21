/*
 * Minimal implementations of the android-base functions actually used by
 * the fec tool (and nothing else). Standalone; not part of AOSP.
 */
#include <android-base/file.h>
#include <android-base/mapped_file.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/threads.h>
#include <android-base/unique_fd.h>

#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace android {
namespace base {

bool ReadFully(borrowed_fd fd, void* data, size_t size) {
    size_t done = 0;
    while (done < size) {
        ssize_t r = read(fd.get(), static_cast<char*>(data) + done, size - done);
        if (r <= 0) {
            return false;
        }
        done += static_cast<size_t>(r);
    }
    return true;
}

bool WriteFully(borrowed_fd fd, const void* data, size_t size) {
    size_t done = 0;
    while (done < size) {
        ssize_t r = write(fd.get(), static_cast<const char*>(data) + done, size - done);
        if (r <= 0) {
            return false;
        }
        done += static_cast<size_t>(r);
    }
    return true;
}

std::vector<std::string> Split(const std::string& s, const std::string& delimiters) {
    std::vector<std::string> result;
    size_t base = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if (delimiters.find(s[i]) != std::string::npos) {
            if (i > base) {
                result.emplace_back(s.substr(base, i - base));
            }
            base = i + 1;
        }
    }
    if (base < s.size()) {
        result.emplace_back(s.substr(base));
    }
    return result;
}

bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); i++) {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }
    return true;
}

uint64_t GetThreadId() {
    return static_cast<uint64_t>(syscall(SYS_gettid));
}

std::string StringPrintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string result;
    StringAppendV(&result, fmt, ap);
    va_end(ap);
    return result;
}

void StringAppendV(std::string* dst, const char* format, va_list ap) {
    va_list copy;
    va_copy(copy, ap);
    int n = vsnprintf(nullptr, 0, format, copy);
    va_end(copy);
    if (n < 0) {
        return;
    }
    size_t old_size = dst->size();
    dst->resize(old_size + static_cast<size_t>(n));
    vsnprintf(&(*dst)[old_size], static_cast<size_t>(n) + 1, format, ap);
}

std::unique_ptr<MappedFile> MappedFile::FromFd(borrowed_fd fd, off64_t offset, size_t length,
                                               int prot) {
    return FromOsHandle(fd.get(), offset, length, prot);
}

std::unique_ptr<MappedFile> MappedFile::FromOsHandle(os_handle h, off64_t offset, size_t length,
                                                    int prot) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        page_size = 4096;
    }
    off64_t page_offset = offset % page_size;
    off64_t page_aligned_offset = offset - page_offset;
    size_t aligned_length = length + static_cast<size_t>(page_offset);

    void* base = mmap(nullptr, aligned_length, prot, MAP_SHARED, h, page_aligned_offset);
    if (base == MAP_FAILED) {
        return nullptr;
    }
    return std::unique_ptr<MappedFile>(
        new MappedFile(static_cast<char*>(base), aligned_length, static_cast<size_t>(page_offset)));
}

MappedFile::~MappedFile() {
    Close();
}

void MappedFile::Close() {
    if (base_ != nullptr && base_ != MAP_FAILED) {
        munmap(base_, size_);
    }
    base_ = nullptr;
    size_ = 0;
    offset_ = 0;
}

MappedFile::MappedFile(MappedFile&& other)
    : base_(other.base_), size_(other.size_), offset_(other.offset_) {
    other.base_ = nullptr;
    other.size_ = 0;
    other.offset_ = 0;
}

MappedFile& MappedFile::operator=(MappedFile&& other) {
    if (this != &other) {
        Close();
        base_ = other.base_;
        size_ = other.size_;
        offset_ = other.offset_;
        other.base_ = nullptr;
        other.size_ = 0;
        other.offset_ = 0;
    }
    return *this;
}

}  // namespace base
}  // namespace android
