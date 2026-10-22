#include "sd_wav_player.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

namespace esphome::sd_wav_player {

static const char *const TAG = "sd_wav_player";
static constexpr char MOUNT_POINT[] = "/sdcard";

static uint16_t read_le16_(const uint8_t *data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

static uint32_t read_le32_(const uint8_t *data) {
  return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

static bool parse_pcm_wav_(FILE *file, size_t *data_length) {
  uint8_t riff_header[12];
  if (fread(riff_header, sizeof(riff_header), 1, file) != 1 || memcmp(riff_header, "RIFF", 4) != 0 ||
      memcmp(riff_header + 8, "WAVE", 4) != 0) {
    return false;
  }

  bool found_format = false;
  while (true) {
    uint8_t chunk_header[8];
    if (fread(chunk_header, sizeof(chunk_header), 1, file) != 1) {
      return false;
    }
    uint32_t chunk_size = read_le32_(chunk_header + 4);

    if (memcmp(chunk_header, "fmt ", 4) == 0) {
      uint8_t format[16];
      if (chunk_size < sizeof(format) || fread(format, sizeof(format), 1, file) != 1) {
        return false;
      }
      const bool supported = read_le16_(format) == 1 && read_le16_(format + 2) == 1 &&
                             read_le32_(format + 4) == 16000 && read_le32_(format + 8) == 32000 &&
                             read_le16_(format + 12) == 2 && read_le16_(format + 14) == 16;
      if (!supported || fseek(file, static_cast<long>(chunk_size - sizeof(format)), SEEK_CUR) != 0) {
        return false;
      }
      found_format = true;
    } else if (memcmp(chunk_header, "data", 4) == 0) {
      if (!found_format || chunk_size == 0 || (chunk_size & 1U) != 0) {
        return false;
      }
      *data_length = chunk_size;
      return true;
    } else {
      const uint32_t padded_size = chunk_size + (chunk_size & 1U);
      if (fseek(file, static_cast<long>(padded_size), SEEK_CUR) != 0) {
        return false;
      }
    }
  }
}

void SDWavPlayer::setup() {
  this->mounted_ = this->mount_card_();
  if (this->mounted_) {
    this->scan_wav_files_();
  }
}

bool SDWavPlayer::mount_card_() {
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  // Reduziere die Taktfrequenz für bessere Kompatibilität (20 MHz statt 40 MHz)
  host.max_freq_khz = 20000;

  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.clk = GPIO_NUM_39;
  slot.cmd = GPIO_NUM_38;
  slot.d0 = GPIO_NUM_40;
  slot.width = 1;
  // Interne Pull-ups aktivieren (notwendig ohne externe Pull-up-Widerstände)
  slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_vfs_fat_mount_config_t mount_config = VFS_FAT_MOUNT_DEFAULT_CONFIG();
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 4;
  // allocation_unit_size ist kritisch für FAT32-Kompatibilität;
  // 16 KB entspricht der typischen Windows-Cluster-Größe für 1 GB-Karten
  mount_config.allocation_unit_size = 16 * 1024;

  sdmmc_card_t *card = nullptr;
  esp_err_t err = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot, &mount_config, &card);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount microSD card: %s (0x%x)", esp_err_to_name(err), err);
    if (err == ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "Hint: card may already be mounted or another driver is active");
    } else if (err == ESP_FAIL) {
      ESP_LOGE(TAG, "Hint: FAT filesystem not recognized. Try reformatting the card "
                    "as FAT32 without a partition table (using SD Formatter tool), "
                    "or check pull-up resistors on CMD/D0 lines.");
    }
    return false;
  }

  ESP_LOGI(TAG, "microSD card mounted at %s", MOUNT_POINT);
  sdmmc_card_print_info(stdout, card);
  return true;
}

bool SDWavPlayer::open_wav_(const char *path, FILE **file, size_t *data_length) {
  FILE *candidate = fopen(path, "rb");
  if (candidate == nullptr) {
    ESP_LOGW(TAG, "Could not open %s", path);
    return false;
  }
  if (!parse_pcm_wav_(candidate, data_length)) {
    ESP_LOGW(TAG, "Skipping %s: expected 16 kHz, 16-bit, mono PCM WAV", path);
    fclose(candidate);
    return false;
  }
  *file = candidate;
  return true;
}

bool SDWavPlayer::scan_wav_files_() {
  this->wav_files_.clear();
  DIR *directory = opendir(MOUNT_POINT);
  if (directory == nullptr) {
    ESP_LOGE(TAG, "Could not open %s", MOUNT_POINT);
    return false;
  }

  std::vector<std::string> valid_files;
  struct dirent *entry;
  while ((entry = readdir(directory)) != nullptr) {
    std::string filename(entry->d_name);
    if (filename.size() < 5) {
      continue;
    }
    std::string extension = filename.substr(filename.size() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension != ".wav") {
      continue;
    }

    const std::string path = std::string(MOUNT_POINT) + "/" + filename;
    struct stat file_status;
    if (stat(path.c_str(), &file_status) != 0 || !S_ISREG(file_status.st_mode)) {
      continue;
    }
    FILE *test_file = nullptr;
    size_t test_data_length = 0;
    if (this->open_wav_(path.c_str(), &test_file, &test_data_length)) {
      fclose(test_file);
      this->wav_files_.push_back(path);
    }
  }
  closedir(directory);

  if (this->wav_files_.empty()) {
    ESP_LOGE(TAG, "No supported WAV files found in %s", MOUNT_POINT);
    return false;
  }
  ESP_LOGI(TAG, "Found %u supported WAV file(s)", static_cast<unsigned>(this->wav_files_.size()));
  return true;
}

bool SDWavPlayer::open_random_wav_() {
  bool last_path_found = false;
  for (const auto &path : this->wav_files_) {
    if (path == this->last_path_) {
      last_path_found = true;
      break;
    }
  }
  const size_t candidate_count =
      this->wav_files_.size() - (last_path_found && this->wav_files_.size() > 1 ? 1 : 0);
  if (candidate_count == 0) {
    ESP_LOGE(TAG, "No supported WAV files are available");
    return false;
  }
  size_t selected_index = esp_random() % candidate_count;
  const std::string *selected_path = nullptr;
  for (const auto &path : this->wav_files_) {
    if (last_path_found && this->wav_files_.size() > 1 && path == this->last_path_) {
      continue;
    }
    if (selected_index-- == 0) {
      selected_path = &path;
      break;
    }
  }
  if (selected_path == nullptr ||
      !this->open_wav_(selected_path->c_str(), &this->file_, &this->data_remaining_)) {
    return false;
  }

  this->last_path_ = *selected_path;
  ESP_LOGI(TAG, "Playing %s", selected_path->c_str());
  this->buffer_length_ = 0;
  this->buffer_offset_ = 0;
  this->playing_ = true;
  return true;
}

void SDWavPlayer::play_random() {
  if (!this->mounted_) {
    ESP_LOGW(TAG, "microSD card was not mounted at startup; retrying mount");
    this->mounted_ = this->mount_card_();
    if (!this->mounted_) {
      ESP_LOGE(TAG, "Cannot play: microSD card is not mounted");
      return;
    }
    this->scan_wav_files_();
  }
  if (this->wav_files_.empty()) {
    ESP_LOGE(TAG, "Cannot play: no supported WAV files were found on the microSD card");
    return;
  }
  if (this->playing_ || (this->speaker_ != nullptr && this->speaker_->is_running())) {
    ESP_LOGW(TAG, "Ignoring motion trigger while audio output is busy");
    return;
  }
  if (this->speaker_ == nullptr) {
    ESP_LOGE(TAG, "No I2S speaker is configured");
    return;
  }
  if (!this->open_random_wav_()) {
    return;
  }
  this->speaker_->start();
}

void SDWavPlayer::loop() {
  if (!this->playing_ || this->speaker_ == nullptr || this->file_ == nullptr) {
    return;
  }

  if (this->buffer_offset_ == this->buffer_length_) {
    if (this->data_remaining_ == 0) {
      this->stop_playback_();
      return;
    }
    const size_t requested = std::min(sizeof(this->audio_buffer_), this->data_remaining_);
    this->buffer_length_ = fread(this->audio_buffer_, 1, requested, this->file_);
    this->buffer_offset_ = 0;
    if (this->buffer_length_ == 0) {
      ESP_LOGE(TAG, "Read error while streaming WAV from microSD");
      this->stop_playback_();
      return;
    }
    this->data_remaining_ -= this->buffer_length_;
  }

  const size_t remaining = this->buffer_length_ - this->buffer_offset_;
  const size_t written = this->speaker_->play(this->audio_buffer_ + this->buffer_offset_, remaining, 0);
  this->buffer_offset_ += written;
}

void SDWavPlayer::stop_playback_() {
  if (this->file_ != nullptr) {
    fclose(this->file_);
    this->file_ = nullptr;
  }
  this->playing_ = false;
  this->speaker_->finish();
}

}  // namespace esphome::sd_wav_player
