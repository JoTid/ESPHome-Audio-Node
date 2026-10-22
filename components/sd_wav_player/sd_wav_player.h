#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "esphome/components/speaker/speaker.h"
#include "esphome/core/component.h"

namespace esphome::sd_wav_player {

class SDWavPlayer : public Component {
 public:
  void set_speaker(speaker::Speaker *speaker) { this->speaker_ = speaker; }

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void play_random();

 protected:
  bool mount_card_();
  bool scan_wav_files_();
  bool open_random_wav_();
  bool open_wav_(const char *path, FILE **file, size_t *data_length);
  void stop_playback_();

  speaker::Speaker *speaker_{nullptr};
  FILE *file_{nullptr};
  bool mounted_{false};
  bool playing_{false};
  size_t data_remaining_{0};
  size_t buffer_length_{0};
  size_t buffer_offset_{0};
  std::string last_path_;
  std::vector<std::string> wav_files_;
  uint8_t audio_buffer_[1024]{};
};

}  // namespace esphome::sd_wav_player
