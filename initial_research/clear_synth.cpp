// clang++ -std=c++20 -O3 clear_synth.cpp -o clear_synth
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

// Base voicing pitch (Fundamental Frequency F0) in Hz
// 120.0 Hz gives a clear, natural resonance anchor for the formants
const double F0_BASE = 120.0; 

struct FormantProperties {
    double f1, f2, f3;
    double a1, a2, a3;
};

struct DynamicPhoneme {
    double duration;        // Duration in seconds
    FormantProperties start;
    FormantProperties end;
    bool is_fricative;      // Triggers high frequency noise-scattering
    bool is_plosive;        // Triggers sharp burst curves
};

// Refined acoustic mapping for maximum phonetic separation
std::map<char, DynamicPhoneme> DICTIONARY = {
    // Vowels (Sustained, mathematically pure formant structures)
    {'a', {.duration = 0.35, .start = {730, 1090, 2440, 1.0, 0.8, 0.4}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false}},
    {'e', {.duration = 0.35, .start = {530, 1840, 2480, 1.0, 0.7, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false}},
    {'i', {.duration = 0.35, .start = {270, 2290, 3010, 1.0, 0.9, 0.6}, .end = {270, 2290, 3010, 1.0, 0.9, 0.6}, .is_fricative = false, .is_plosive = false}},
    {'o', {.duration = 0.35, .start = {570,  840, 2410, 1.0, 0.6, 0.3}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = false}},
    {'u', {.duration = 0.35, .start = {300,  870, 2240, 1.0, 0.5, 0.2}, .end = {300,  870, 2240, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = false}},

    // Plosives (Frequencies morph rapidly outward from zero-points)
    {'b', {.duration = 0.30, .start = {150,  500, 1200, 0.9, 0.4, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true}},
    {'d', {.duration = 0.30, .start = {300, 2400, 3000, 0.9, 0.8, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = true}},
    {'g', {.duration = 0.30, .start = {200, 1600, 2200, 0.9, 0.6, 0.3}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = true}},
    {'p', {.duration = 0.30, .start = {100,  600, 1000, 0.4, 0.2, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true}},
    {'t', {.duration = 0.30, .start = {400, 2600, 3300, 0.5, 0.7, 0.6}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = true}},
    {'k', {.duration = 0.30, .start = {300, 1900, 2800, 0.6, 0.6, 0.4}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true}},

    // Fricatives & Sibilants
    {'s', {.duration = 0.40, .start = {5000, 6200, 7500, 0.7, 0.9, 0.8}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = true,  .is_plosive = false}},
    {'f', {.duration = 0.35, .start = {3200, 4200, 5200, 0.4, 0.4, 0.3}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false}},
    {'h', {.duration = 0.30, .start = {1400, 2400, 3400, 0.3, 0.3, 0.2}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false}},
    {'z', {.duration = 0.40, .start = {4000, 5000, 6000, 0.6, 0.8, 0.7}, .end = {270, 2290, 3010, 1.0, 0.9, 0.6}, .is_fricative = true,  .is_plosive = false}},

    // Liquids & Glides (Continuous transitions)
    {'l', {.duration = 0.35, .start = {310, 1050, 2600, 0.8, 0.5, 0.3},  .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false}},
    {'m', {.duration = 0.30, .start = {250,  950, 2300, 0.8, 0.3, 0.1},  .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false}},
    {'n', {.duration = 0.30, .start = {250, 1650, 2300, 0.8, 0.3, 0.1},  .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false}},
    {'r', {.duration = 0.35, .start = {350, 1000, 1400, 0.8, 0.7, 0.6},  .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false}},
    {'w', {.duration = 0.35, .start = {280,  650, 2200, 0.9, 0.4, 0.2},  .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false}}
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
        std::cout << "Usage: " << argv[0] << " <string>\n";
        return 1;
    }

    std::string input_text = argv[1];
    std::string output_filename = "clear_speech.wav";
    std::transform(input_text.begin(), input_text.end(), input_text.begin(), ::tolower);

    std::vector<short> pcm_data;
    
    // Core Phase Accumulators
    double p0 = 0.0, p1 = 0.0, p2 = 0.0, p3 = 0.0; 
    unsigned int noise_seed = 54321;

    std::cout << "[*] Processing speech via Unified Voicing Carrier Engine...\n";

    for (char c : input_text) {
        if (c == ' ') {
            int space_samples = static_cast<int>(0.15 * SAMPLE_RATE);
            for (int s = 0; s < space_samples; ++s) pcm_data.push_back(0);
            continue;
        }

        if (!DICTIONARY.contains(c)) continue;
        const auto& ph = DICTIONARY[c];
        int total_samples = static_cast<int>(ph.duration * SAMPLE_RATE);

        for (int s = 0; s < total_samples; ++s) {
            double linear_progress = static_cast<double>(s) / total_samples;
            
            // Logarithmic and Exponential mapping layers for distinct articulation boundaries
            double formant_progress = linear_progress;
            if (ph.is_plosive) {
                formant_progress = 1.0 - std::exp(-9.0 * linear_progress); // Accelerated burst snap
            } else if (ph.is_fricative) {
                formant_progress = std::pow(linear_progress, 2.0);        // Extended friction trail
            }

            // Interpolate target frequencies
            double f1 = ph.start.f1 + formant_progress * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + formant_progress * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + formant_progress * (ph.end.f3 - ph.start.f3);

            // Interpolate dynamic mixing weights
            double a1 = ph.start.a1 + linear_progress * (ph.end.a1 - ph.start.a1);
            double a2 = ph.start.a2 + linear_progress * (ph.end.a2 - ph.start.a2);
            double a3 = ph.start.a3 + linear_progress * (ph.end.a3 - ph.start.a3);

            // Phase integrations tracking
            p0 += (2.0 * PI * F0_BASE) / SAMPLE_RATE; // Continuous fundamental core tracker
            if (p0 > 2.0 * PI) p0 -= 2.0 * PI;

            // Handle high-frequency chaotic updates for fricatives
            if (ph.is_fricative && linear_progress < 0.5) {
                noise_seed = noise_seed * 1103515245 + 12345;
                double noise_mod = static_cast<double>(noise_seed % 1000) / 1000.0 * 2.0 * PI;
                
                p1 += (2.0 * PI * f1) / SAMPLE_RATE + (noise_mod * 0.2);
                p2 += (2.0 * PI * f2) / SAMPLE_RATE + (noise_mod * 0.2);
                p3 += (2.0 * PI * f3) / SAMPLE_RATE + (noise_mod * 0.2);
            } else {
                p1 += (2.0 * PI * f1) / SAMPLE_RATE;
                p2 += (2.0 * PI * f2) / SAMPLE_RATE;
                p3 += (2.0 * PI * f3) / SAMPLE_RATE;
            }

            if (p1 > 2.0 * PI) p1 -= 2.0 * PI;
            if (p2 > 2.0 * PI) p2 -= 2.0 * PI;
            if (p3 > 2.0 * PI) p3 -= 2.0 * PI;

            // Synthesis core: Modulate formants with the fundamental voicing frequency (p0)
            // This tricks the brain into hearing a cohesive voice instead of disjointed musical pitches
            double carrier = std::sin(p0);
            
            double sample_val = (a1 * std::sin(p1) * (0.4 + 0.6 * carrier)) + 
                                (a2 * std::sin(p2) * (0.3 + 0.7 * carrier)) + 
                                (a3 * std::sin(p3) * (0.2 + 0.8 * carrier));

            // Master normalization boundary
            sample_val /= (a1 + a2 + a3);

            // Clean edge window smoothing structures
            if (s < 300) sample_val *= (static_cast<double>(s) / 300.0);
            if (s > total_samples - 500) sample_val *= (static_cast<double>(total_samples - s) / 500.0);

            pcm_data.push_back(static_cast<short>(sample_val * 32767));
        }
    }

    std::ofstream file(output_filename, std::ios::binary);
    if (!file) return 1;

    int size = pcm_data.size() * sizeof(short);
    write_wav_header(file, size);
    file.write(reinterpret_cast<const char*>(pcm_data.data()), size);
    file.close();

    std::cout << "[+] High-clarity SWS file generated successfully: " << output_filename << "\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}
