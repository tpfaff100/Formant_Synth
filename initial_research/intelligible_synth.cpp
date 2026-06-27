//clang++ -std=c++20 -O3 intelligible_synth.cpp -o intelligible_synth
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>
#include <algorithm>

const int SAMPLE_RATE = 22050;
const double PI = 3.14159265358979323846;

struct FormantProperties {
    double f1, f2, f3;
    double a1, a2, a3;
};

struct DynamicPhoneme {
    double duration;        // Total duration in seconds
    FormantProperties start;
    FormantProperties end;
    bool is_fricative;      // Triggers high frequency noise-mimicry
    bool is_plosive;        // Triggers sharp exponential release curves
};

// C++20 designated initializer dictionary maps precise acoustic properties
std::map<char, DynamicPhoneme> DICTIONARY = {
    // Vowels (Steady, perfectly balanced amplitude weights)
    {'a', {.duration = 0.40, .start = {730, 1090, 2440, 1.0, 0.7, 0.3}, .end = {730, 1090, 2440, 1.0, 0.7, 0.3}, .is_fricative = false, .is_plosive = false}},
    {'e', {.duration = 0.40, .start = {530, 1840, 2480, 1.0, 0.6, 0.4}, .end = {530, 1840, 2480, 1.0, 0.6, 0.4}, .is_fricative = false, .is_plosive = false}},
    {'i', {.duration = 0.40, .start = {270, 2290, 3010, 1.0, 0.8, 0.4}, .end = {270, 2290, 3010, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false}},
    {'o', {.duration = 0.40, .start = {570,  840, 2410, 1.0, 0.5, 0.2}, .end = {570,  840, 2410, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = false}},
    {'u', {.duration = 0.40, .start = {300,  870, 2240, 1.0, 0.4, 0.1}, .end = {300,  870, 2240, 1.0, 0.4, 0.1}, .is_fricative = false, .is_plosive = false}},

    // Plosives (Sharp burst transitions moving into an inherent vowel base)
    {'b', {.duration = 0.35, .start = {180,  600, 1500, 0.8, 0.3, 0.1}, .end = {530, 1840, 2480, 1.0, 0.6, 0.4}, .is_fricative = false, .is_plosive = true}},
    {'d', {.duration = 0.35, .start = {300, 2200, 2800, 0.9, 0.7, 0.4}, .end = {530, 1840, 2480, 1.0, 0.6, 0.4}, .is_fricative = false, .is_plosive = true}},
    {'g', {.duration = 0.35, .start = {250, 1400, 2000, 0.9, 0.5, 0.2}, .end = {530, 1840, 2480, 1.0, 0.6, 0.4}, .is_fricative = false, .is_plosive = true}},
    {'p', {.duration = 0.35, .start = {150,  700, 1200, 0.5, 0.2, 0.1}, .end = {730, 1090, 2440, 1.0, 0.7, 0.3}, .is_fricative = false, .is_plosive = true}},
    {'t', {.duration = 0.35, .start = {450, 2400, 3100, 0.6, 0.6, 0.5}, .end = {530, 1840, 2480, 1.0, 0.6, 0.4}, .is_fricative = false, .is_plosive = true}},
    {'k', {.duration = 0.35, .start = {350, 1800, 2700, 0.7, 0.5, 0.3}, .end = {730, 1090, 2440, 1.0, 0.7, 0.3}, .is_fricative = false, .is_plosive = true}},

    // Sibilants & Fricatives (Requires high spectrum noise simulation + vowel release)
    {'s', {.duration = 0.45, .start = {4500, 5800, 7000, 0.8, 0.9, 0.8}, .end = {530, 1840, 2480, 0.8, 0.5, 0.3}, .is_fricative = true,  .is_plosive = false}},
    {'f', {.duration = 0.40, .start = {2800, 3800, 4800, 0.4, 0.4, 0.3}, .end = {530, 1840, 2480, 0.9, 0.6, 0.4}, .is_fricative = true,  .is_plosive = false}},
    {'h', {.duration = 0.35, .start = {1200, 2200, 3200, 0.3, 0.3, 0.2}, .end = {730, 1090, 2440, 1.0, 0.7, 0.3}, .is_fricative = true,  .is_plosive = false}},
    {'z', {.duration = 0.45, .start = {3500, 4500, 5500, 0.7, 0.7, 0.6}, .end = {270, 2290, 3010, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false}},

    // Liquids & Glides (Smooth, fully vocalized non-linear sweeps)
    {'l', {.duration = 0.40, .start = {310, 1050, 2600, 0.8, 0.5, 0.3},  .end = {530, 1840, 2480, 1.0, 0.6, 0.4}, .is_fricative = false, .is_plosive = false}},
    {'m', {.duration = 0.35, .start = {250,  950, 2300, 0.8, 0.2, 0.05}, .end = {730, 1090, 2440, 1.0, 0.7, 0.3}, .is_fricative = false, .is_plosive = false}},
    {'n', {.duration = 0.35, .start = {250, 1650, 2300, 0.8, 0.2, 0.05}, .end = {530, 1840, 2480, 1.0, 0.6, 0.4}, .is_fricative = false, .is_plosive = false}},
    {'r', {.duration = 0.40, .start = {350, 1000, 1400, 0.8, 0.6, 0.5},  .end = {530, 1840, 2480, 1.0, 0.6, 0.4}, .is_fricative = false, .is_plosive = false}},
    {'w', {.duration = 0.40, .start = {280,  650, 2200, 0.9, 0.3, 0.1},  .end = {730, 1090, 2440, 1.0, 0.7, 0.3}, .is_fricative = false, .is_plosive = false}}
};

void write_wav_header(std::ofstream& file, int data_size) {
    file.write("RIFF", 4);
    int chunk_size = 36 + data_size;
    file.write(reinterpret_cast<const char*>(&chunk_size), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    int subchunk1_size = 16;
    short audio_format = 1; short num_channels = 1; int sample_rate = SAMPLE_RATE;
    int byte_rate = SAMPLE_RATE * 2; short block_align = 2; short bits_per_sample = 16;
    file.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
    file.write(reinterpret_cast<const char*>(&audio_format), 2);
    file.write(reinterpret_cast<const char*>(&num_channels), 2);
    file.write(reinterpret_cast<const char*>(&sample_rate), 4);
    file.write(reinterpret_cast<const char*>(&byte_rate), 4);
    file.write(reinterpret_cast<const char*>(&block_align), 2);
    file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&data_size), 4);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <string> [-o output.wav]\n";
        return 1;
    }

    std::string input_text = argv[1];
    std::string output_filename = "intelligible.wav";
    std::transform(input_text.begin(), input_text.end(), input_text.begin(), ::tolower);

    std::vector<short> pcm_data;
    double p1 = 0.0, p2 = 0.0, p3 = 0.0; // Phase integrators
    unsigned int noise_seed = 12345;     // Fast LCG random tracker for fricative white-noise mimicry

    std::cout << "[*] Running Acoustic Formant Model Synthesis (C++20)...\n";

    for (char c : input_text) {
        if (c == ' ') {
            int space_samples = static_cast<int>(0.15 * SAMPLE_RATE);
            for (int s = 0; s < space_samples; ++s) pcm_data.push_back(0);
            continue;
        }

        if (!DICTIONARY.contains(c)) continue; // Clean C++20 lookup map check
        const auto& ph = DICTIONARY[c];
        int total_samples = static_cast<int>(ph.duration * SAMPLE_RATE);

        for (int s = 0; s < total_samples; ++s) {
            double linear_progress = static_cast<double>(s) / total_samples;
            
            // Core upgrade: Plosives use an exponential warp factor to simulate a pressure pop release
            double curve_progress = linear_progress;
            if (ph.is_plosive) {
                curve_progress = 1.0 - std::exp(-7.0 * linear_progress);
            } else if (ph.is_fricative) {
                curve_progress = std::pow(linear_progress, 1.5); // Slower, smoother release for friction noise
            }

            // Interpolate center frequencies
            double f1 = ph.start.f1 + curve_progress * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + curve_progress * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + curve_progress * (ph.end.f3 - ph.start.f3);

            // Interpolate mixing volume bounds
            double a1 = ph.start.a1 + linear_progress * (ph.end.a1 - ph.start.a1);
            double a2 = ph.start.a2 + linear_progress * (ph.end.a2 - ph.start.a2);
            double a3 = ph.start.a3 + linear_progress * (ph.end.a3 - ph.start.a3);

            // Audio Upgrade: Fricative Phase Scattering Algorithm
            if (ph.is_fricative && linear_progress < 0.6) {
                // Generate fast pseudorandom noise phase shifts using Linear Congruential Generator logic
                noise_seed = noise_seed * 1103515245 + 12345;
                double noise_offset = static_cast<double>(noise_seed % 1000) / 1000.0 * 2.0 * PI;
                
                p1 += (2.0 * PI * f1) / SAMPLE_RATE + (noise_offset * 0.1);
                p2 += (2.0 * PI * f2) / SAMPLE_RATE + (noise_offset * 0.1);
                p3 += (2.0 * PI * f3) / SAMPLE_RATE + (noise_offset * 0.1);
            } else {
                p1 += (2.0 * PI * f1) / SAMPLE_RATE;
                p2 += (2.0 * PI * f2) / SAMPLE_RATE;
                p3 += (2.0 * PI * f3) / SAMPLE_RATE;
            }

            // Keep phase wrappers clean
            if (p1 > 2.0 * PI) p1 -= 2.0 * PI;
            if (p2 > 2.0 * PI) p2 -= 2.0 * PI;
            if (p3 > 2.0 * PI) p3 -= 2.0 * PI;

            // Generate composite output sample
            double sample_val = (a1 * std::sin(p1)) + (a2 * std::sin(p2)) + (a3 * std::sin(p3));
            sample_val /= (a1 + a2 + a3); // Dynamic scale normalizing factor

            // Apply global windowing tracking boundaries (Prevents popping)
            if (s < 200) sample_val *= (static_cast<double>(s) / 200.0);
            if (s > total_samples - 600) sample_val *= (static_cast<double>(total_samples - s) / 600.0);

            pcm_data.push_back(static_cast<short>(sample_val * 32767));
        }
    }

    std::ofstream file(output_filename, std::ios::binary);
    if (!file) return 1;

    int size = pcm_data.size() * sizeof(short);
    write_wav_header(file, size);
    file.write(reinterpret_cast<const char*>(pcm_data.data()), size);
    file.close();

    std::cout << "[+] Optimized SWS file rendered to: " << output_filename << "\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}

