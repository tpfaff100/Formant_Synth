//clang++ -std=c++20 -O3 retro_words_synth.cpp -o retro_words_synth
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
const double F0_BASE = 120.0; 

struct FormantData {
    double f1, f2, f3;
    double a1, a2, a3;
};

struct PhoneticRule {
    double duration;
    FormantData start;
    FormantData end;
    bool is_fricative;
    bool is_plosive;
    double burst_duration; 
};

// Global Phoneme Dictionary Matrix
std::map<char, PhoneticRule> PHONEMES = {
    {'a', {.duration = 0.28, .start = {730, 1090, 2440, 1.0, 0.8, 0.4}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'e', {.duration = 0.26, .start = {530, 1840, 2480, 1.0, 0.7, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'i', {.duration = 0.26, .start = {270, 2290, 3010, 1.0, 0.9, 0.6}, .end = {270, 2290, 3010, 1.0, 0.9, 0.6}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'o', {.duration = 0.28, .start = {570,  840, 2410, 1.0, 0.6, 0.3}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'u', {.duration = 0.28, .start = {300,  870, 2240, 1.0, 0.5, 0.2}, .end = {300,  870, 2240, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'w', {.duration = 0.24, .start = {290,  610, 2150, 0.9, 0.3, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'y', {.duration = 0.24, .start = {260, 2300, 3050, 1.0, 0.9, 0.5}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    {'c', {.duration = 0.22, .start = {350, 2200, 3000, 0.0, 1.0, 0.9}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.035}},
    {'k', {.duration = 0.22, .start = {350, 2200, 3000, 0.0, 1.0, 0.9}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.035}},
    {'p', {.duration = 0.22, .start = {150,  600, 1100, 0.0, 0.4, 0.2}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.025}},
    {'t', {.duration = 0.22, .start = {400, 2400, 3200, 0.0, 0.9, 0.9}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.030}},
    {'b', {.duration = 0.24, .start = {180,  700, 1200, 0.8, 0.3, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.012}},
    {'d', {.duration = 0.24, .start = {250, 1800, 2600, 0.8, 0.8, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.015}},
    {'g', {.duration = 0.22, .start = {250, 1400, 2000, 0.7, 0.5, 0.2}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.015}},
    {'q', {.duration = 0.35, .start = {300, 1400, 2400, 0.0, 1.0, 0.9}, .end = {300,  870, 2240, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.030}},

    {'s', {.duration = 0.32, .start = {5200, 6500, 7800, 0.2, 0.9, 0.8}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'f', {.duration = 0.30, .start = {2600, 3600, 4800, 0.3, 0.3, 0.2}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'h', {.duration = 0.26, .start = {1000, 2000, 3000, 0.3, 0.3, 0.2}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'v', {.duration = 0.30, .start = {300,  1300, 2500, 0.6, 0.5, 0.3}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'x', {.duration = 0.38, .start = {530,  1840, 2480, 0.9, 0.6, 0.4}, .end = {5500, 6800, 7800, 0.4, 0.8, 0.8}, .is_fricative = true,  .is_plosive = true,  .burst_duration = 0.030}},
    {'z', {.duration = 0.34, .start = {4200, 5200, 6200, 0.6, 0.7, 0.6}, .end = {270, 2290, 3010, 1.0, 0.9, 0.6}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'m', {.duration = 0.24, .start = {280,  900, 2200, 0.8, 0.2, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'n', {.duration = 0.24, .start = {280, 1700, 2300, 0.8, 0.2, 0.1}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'l', {.duration = 0.26, .start = {310, 1050, 2600, 0.8, 0.4, 0.2}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'r', {.duration = 0.26, .start = {320,  950, 1300, 0.8, 0.6, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}
};

// MENU Definitions updated to express acronyms and names as full spoken words
std::map<int, std::pair<std::string, std::string>> MENU = {
    {1,  {"Pet 2001", "pet tu taused wan"}},
    {2,  {"t-r-s-80", "trs eiti"}},                              // Spoken smoothly as "trs eighty"
    {3,  {"apple two", "apl tu"}},
    {4,  {"NEC PC8201a", "nek pisi eit tu zi ro wan ei"}},        // "NEC" as "nek"
    {5,  {"Model 100", "madl wan han dred"}},
    {6,  {"Sparcstation", "spark steisen"}},
    {7,  {"NeXT Computer", "nekst kam pyu ter"}},
    {8,  {"Macintosh", "makin tas"}},
    {9,  {"Commodore 64", "kam ador siksti for"}},
    {10, {"Commodore 64 executive", "kam ador siksti for eg zek yutiv"}},
    {11, {"HP 9825", "ets pi nin eit tu fiv"}},                  // "HP" blended as "ets-pi"
    {12, {"HP 9000-712", "ets pi nin taused sevn wan tu"}},
    {13, {"HP2000", "ets pi tu taused"}},
    {14, {"HP3000", "ets pi tri taused"}}
};

double get_glottal_pulse(double phase) {
    double normalized_phase = phase / (2.0 * PI);
    if (normalized_phase < 0.30) {
        return 3.0 * std::pow(normalized_phase / 0.30, 2) - 2.0 * std::pow(normalized_phase / 0.30, 3);
    } else if (normalized_phase < 0.40) {
        return 1.0 - std::pow((normalized_phase - 0.30) / 0.10, 2);
    }
    return 0.0; 
}

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

void print_menu() {
    std::cout << "=== Word-Optimized Retro Hardware Speech Engine ===\n";
    for (const auto& [id, target] : MENU) {
        std::cout << "  [" << id << "] " << target.first << "\n";
    }
    std::cout << "Usage: ./retro_words_synth <number>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_menu();
        return 1;
    }

    int choice = std::atoi(argv[1]);
    if (!MENU.contains(choice)) {
        std::cout << "[-] Invalid hardware option.\n";
        print_menu();
        return 1;
    }

    auto [label, phonetic_sequence] = MENU[choice];
    std::cout << "[*] Synthesizing continuous word phrases for: \"" << label << "\"\n";

    std::string output_filename = "retro_words.wav";
    std::vector<short> pcm_data;
    double p0 = 0.0, p1 = 0.0, p2 = 0.0, p3 = 0.0;
    unsigned int noise_seed = 13579;

    for (char c : phonetic_sequence) {
        if (c == ' ') {
            int space_samples = static_cast<int>(0.07 * SAMPLE_RATE);
            for (int s = 0; s < space_samples; ++s) pcm_data.push_back(0);
            continue;
        }

        if (!PHONEMES.contains(c)) continue;
        const auto& ph = PHONEMES[c];
        int total_samples = static_cast<int>(ph.duration * SAMPLE_RATE);
        int burst_samples = static_cast<int>(ph.burst_duration * SAMPLE_RATE);

        for (int s = 0; s < total_samples; ++s) {
            double linear_progress = static_cast<double>(s) / total_samples;
            
            double warp_progress = linear_progress;
            if (ph.is_plosive) {
                warp_progress = 1.0 - std::exp(-11.5 * linear_progress);
            } else if (ph.is_fricative) {
                warp_progress = std::pow(linear_progress, 1.6);
            }

            double f1 = ph.start.f1 + warp_progress * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + warp_progress * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + warp_progress * (ph.end.f3 - ph.start.f3);

            double a1 = ph.start.a1 + linear_progress * (ph.end.a1 - ph.start.a1);
            double a2 = ph.start.a2 + linear_progress * (ph.end.a2 - ph.start.a2);
            double a3 = ph.start.a3 + linear_progress * (ph.end.a3 - ph.start.a3);

            p0 += (2.0 * PI * F0_BASE) / SAMPLE_RATE;
            if (p0 > 2.0 * PI) p0 -= 2.0 * PI;
            double glottal_source = get_glottal_pulse(p0);

            noise_seed = noise_seed * 1103515245 + 12345;
            double noise = (static_cast<double>(noise_seed % 2000) / 1000.0) - 1.0;

            p1 += (2.0 * PI * f1) / SAMPLE_RATE;
            p2 += (2.0 * PI * f2) / SAMPLE_RATE;
            p3 += (2.0 * PI * f3) / SAMPLE_RATE;
            if (p1 > 2.0 * PI) p1 -= 2.0 * PI;
            if (p2 > 2.0 * PI) p2 -= 2.0 * PI;
            if (p3 > 2.0 * PI) p3 -= 2.0 * PI;

            double sample_val = 0.0;

            if (s < burst_samples) {
                double burst_envelope = 1.0 - (static_cast<double>(s) / burst_samples);
                sample_val = (a2 * std::sin(p2) * noise) + (a3 * std::sin(p3) * noise);
                sample_val *= (burst_envelope * 1.3);
            } else if (ph.is_fricative) {
                sample_val = (a1 * std::sin(p1)) + 
                             (a2 * std::sin(p2) * (0.3 * noise + 0.7)) + 
                             (a3 * std::sin(p3) * (0.6 * noise + 0.4));
                sample_val /= (a1 + a2 + a3);
            } else {
                sample_val = (a1 * std::sin(p1) * (0.35 + 0.65 * glottal_source)) + 
                             (a2 * std::sin(p2) * (0.25 + 0.75 * glottal_source)) + 
                             (a3 * std::sin(p3) * (0.15 + 0.85 * glottal_source));
                sample_val /= (a1 + a2 + a3);
            }

            if (s < 120) sample_val *= (static_cast<double>(s) / 120.0);
            if (s > total_samples - 300) sample_val *= (static_cast<double>(total_samples - s) / 300.0);

            pcm_data.push_back(static_cast<short>(sample_val * 32767));
        }
    }

    std::ofstream file(output_filename, std::ios::binary);
    if (!file) return 1;

    int size = pcm_data.size() * sizeof(short);
    write_wav_header(file, size);
    file.write(reinterpret_cast<const char*>(pcm_data.data()), size);
    file.close();

    std::cout << "[+] Rendering complete.\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}

