#include <iostream>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <expected>
#include <string>
#include <string_view>
#include <vector>
#include <format>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#ifndef SAFEMAP_H_
#define SAFEMAP_H_

//clase SafeMap
class SafeMap {
 public:
  explicit SafeMap(void* addr = nullptr, size_t length = 0) noexcept : addr_{addr}, length_{length} {}
  SafeMap(const SafeMap&) = delete;               
  SafeMap& operator=(const SafeMap&) = delete;    
  SafeMap(SafeMap&& other) noexcept : addr_{other.addr_}, length_{other.length_} {
    other.addr_ = nullptr;
    other.length_ = 0;
  }
  SafeMap& operator=(SafeMap&& other) noexcept {  
    if (this != &other) {
      release();
      addr_ = other.addr_;
      length_ = other.length_;
      other.addr_ = nullptr;
      other.length_ = 0;
    }
    return *this;
  }
  ~SafeMap() noexcept { 
    release();
  }

  [[nodiscard]] std::string_view get() const noexcept {
    if (!addr_ || length_ == 0) {
      return {};
    }
    return std::string_view(static_cast<const char*>(addr_), length_);
  }
  [[nodiscard]] size_t size() const noexcept { return length_; }
  const void* data() const { return addr_; }
  [[nodiscard]] bool is_valid() const noexcept { return addr_ != nullptr; }

 private:
  void release() noexcept {
    if (addr_) {
      munmap(addr_, length_);
      addr_ = nullptr;
      length_ = 0;
    }
  }

  void* addr_;
  size_t length_;
};


//clase SafeFD
class SafeFD {
 public:
  explicit SafeFD(int fd) noexcept : fd_{fd} { }
  explicit SafeFD() noexcept : fd_{-1} { }
  SafeFD(const SafeFD&) = delete;
  SafeFD& operator=(const SafeFD&) = delete;
  SafeFD(SafeFD&& other) noexcept: fd_{other.fd_} { other.fd_ = -1; }
  SafeFD& operator=(SafeFD&& other) noexcept {
    if (this != &other && fd_ != other.fd_) {
      // Cerrar el descriptor de archivo actual
      close(fd_);
      fd_ = other.fd_;
      other.fd_ = -1;
    }
    return *this;
  }
  ~SafeFD() noexcept {
    if (fd_ >= 0) {
      close(fd_);
    }
  }

  [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0; }
  [[nodiscard]] int get() const noexcept { return fd_; }

 private:
  int fd_;
};

#endif