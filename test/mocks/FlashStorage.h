#ifndef MOCK_FLASH_STORAGE_H
#define MOCK_FLASH_STORAGE_H

// Mock FlashStorage for host-based testing.
// Provides a simple in-memory store that mimics the FlashStorage API.

template <typename T>
class FlashStorageClass {
public:
  FlashStorageClass() : written_(false) {}

  T read() const {
    return data_;
  }

  void write(const T& value) {
    data_ = value;
    written_ = true;
  }

  bool wasWritten() const { return written_; }
  void resetWritten() { written_ = false; }

private:
  T data_{};
  bool written_;
};

// Macro that mimics the FlashStorage library's declaration macro
#define FlashStorage(name, type) FlashStorageClass<type> name

#endif
