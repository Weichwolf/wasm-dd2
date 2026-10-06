// Model the installed Chromium converter using its independently pinned header.
// These synthetic fixtures cover all S16 values; they are not original game PCM.
#include "audio_sample_types.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using Traits = media::SignedInt16SampleTypeTraits;

int main(int argc, char **argv) {
    if (argc != 3) return 1;
    const std::string mode = argv[1], directory = argv[2];
    if (mode != "canonical" && mode != "chromium" && mode != "inverse") return 1;
    FILE *input = std::fopen((directory + "/input.pcm").c_str(), "wb");
    FILE *source = std::fopen((directory + "/expected-source.f32").c_str(), "wb");
    FILE *model = std::fopen((directory + "/expected-device.pcm").c_str(), "wb");
    if (!input || !source || !model) return 2;
    unsigned mismatches = 0, source_changes = 0;
    for (int frame = 0; frame < 65536; ++frame) {
        for (int channel = 0; channel < 2; ++channel) {
            const int16_t n = channel ? 32767 - frame : frame - 32768;
            const float canonical = n / 32768.0f;
            float value = canonical;
            if (mode == "chromium") value = Traits::ToFloat(n);
            if (mode == "inverse" && n > 0) value = n / 32767.0f;
            const int16_t converted = Traits::FromFloat(value);
            mismatches += converted != n;
            source_changes += std::memcmp(&value, &canonical, sizeof(value)) != 0;
            if (std::fwrite(&n, sizeof(n), 1, input) != 1 ||
                std::fwrite(&value, sizeof(value), 1, source) != 1 ||
                std::fwrite(&converted, sizeof(converted), 1, model) != 1) return 3;
        }
    }
    if (std::fclose(input) || std::fclose(source) || std::fclose(model)) return 4;
    std::printf("{\"samples\":131072,\"s16_mismatches\":%u,"
                "\"canonical_float_changes\":%u}\n", mismatches, source_changes);
}
