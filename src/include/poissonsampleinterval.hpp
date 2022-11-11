#pragma once
#include <_types/_uint64_t.h>
#include <random>
#include <unordered_map>

/**
 * @brief "triggers" samples using a geometric distribution
 *
 * Sampled recording allocated objects with a rate of every
 * SAMPLE_INTERVAL bytes (on average).
 *
 */

class PoissonSampleInterval {
 public:
  /**
   * @brief Construct a new SampleInterval object
   *
   */
  PoissonSampleInterval(uint64_t SAMPLE_INTERVAL) : gen(rd()), d(1.0 / SAMPLE_INTERVAL), allocs(0), frees(0) {
    resetAlloc();
  }

  uint64_t allocs;
  uint64_t frees;
  
  /**
   * @brief Deallocate an object; if sampled, return the size of the recorded sampling interval, else 0.
   *
   * @param sample 
   * @return uint64_t the previous sample interval if we crossed it; 0 otherwise
   */
  inline bool decrement(uint64_t, void * ptr, size_t& ret) {
    auto found = _allocSize.find(ptr) != _allocSize.end();
    if (!found) {
      // Not found
      return false;
    } else {
      // It was sampled. Return the recorded size, removing the object first.
      ret = _allocSize[ptr];
      _allocSize.erase(ptr);
      frees += ret;
      printf_("DEALLOC %p %lu (%lu)\n", ptr, ret, (allocs - frees) / 1048576);
      return true;
    }
  }

  /**
   * @brief increment by the sample amount, triggering an interval reset when we
   * cross the threshold
   *
   * @param sample the amount to decrement the sample interval by
   * @return uint64_t the previous sample interval if we crossed it; 0 otherwise
   */
  inline bool increment(uint64_t sample, void * ptr, size_t& ret) {
    if (unlikely(sample > _tillNextAlloc)) {
      auto prev = _countdownAlloc;
      auto diff = sample - _tillNextAlloc;
      resetAlloc();
      ret = prev + diff;
      _allocSize[ptr] = ret;
      allocs += ret;
      printf_("ALLOC %p %lu (%lu)\n", ptr, ret, (allocs - frees) / 1048576);
      return true;
    }
    _tillNextAlloc -= sample;
    return false;
  }

 private:

  std::random_device rd;
  std::mt19937 gen;
  std::geometric_distribution<uint64_t> d;

  uint64_t _tillNextAlloc;
  uint64_t _countdownAlloc;  /// the number of frees since the last sample interval

  std::unordered_map<void *, uint64_t> _allocSize;
  
  void resetAlloc() {
    // Generate a new sample from the exponential distribution.
    _countdownAlloc = d(gen);
    _tillNextAlloc = _countdownAlloc;
  }
};
