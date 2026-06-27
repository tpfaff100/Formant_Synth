//clang++ -std=c++20 -O3 light_my_way.cpp -o light_my_way
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
const double F0_BASE = 120.0; // Balanced vocal cadence pitch baseline

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

// Custom phoneme mapping tailored for the exact transition matrix of "light my way"
std::map<char, PhoneticRule> PHONEMES = {
    // 'l' -> Alveolar liquid opening up into the 'ai' diphthong
    {'l', {.duration = 0.18, .start = {310, 1050, 2600, 0.8, 0.4, 0.2}, .end = {750, 1200, 2400, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // 'i' -> Wide open 'ai' diphthong body sliding towards a tense 't' anchor
    {'i', {.duration = 0.26, .start = {750, 1200, 2400, 1.0, 0.8, 0.4}, .end = {450, 2100, 2750, 0.8, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // 't' -> Unvoiced alveolar release crack concluding "light"
    {'t', {.duration = 0.16, .start = {400, 2400, 3200, 0.0, 0.9, 0.9}, .end = {400, 2400, 3200, 0.0, 0.1, 0.1}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.025}},
    
    // 'm' -> Bilabial nasal resonance block bursting into the upcoming vowel
    {'m', {.duration = 0.20, .start = {250,  900, 2200, 0.7, 0.2, 0.1}, .end = {750, 1250, 2450, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // 'y' -> Trailing diphthong component of "my"
    {'y', {.duration = 0.22, .start = {750, 1250, 2450, 1.0, 0.8, 0.4}, .end = {300, 2200, 2900, 0.8, 0.9, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // 'w' -> Deeply rounded labial semivowel glide starting line
    {'w', {.duration = 0.18, .start = {290,  610, 2150, 0.9, 0.3, 0.1}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // 'e' -> Tense 'ey' vowel core concluding "way"
    {'e', {.duration = 0.28, .start = {530, 1840, 2480, 1.0, 0.7, 0.5}, .end = {350, 2300, 3000, 0.7, 0.9, 0.6}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}
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

int main() {
    // Explicit sequence mapping for the target phrase "light my way"
    std::string target_sequence = "lit my we"; 
    std::string output_filename = "light_my_way.wav";

    std::vector<short> pcm_data;
    double p0 = 0.0, p1 = 0.0, p2 = 0.0, p3 = 0.0;
    unsigned int noise_seed = 98765;

    std::cout << "[*] Compiling specialized dual-source phrase matrix for: \"light my way\"...\n";

    for (char c : target_sequence) {
        if (c == ' ') {
            int space_samples = static_cast<int>(0.10 * SAMPLE_RATE);
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
                warp_progress = 1.0 - std::exp(-11.0 * linear_progress);
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
            } else {
                sample_val = (a1 * std::sin(p1) * (0.35 + 0.65 * glottal_source)) + 
                             (a2 * std::sin(p2) * (0.25 + 0.75 * glottal_source)) + 
                             (a3 * std::sin(p3) * (0.15 + 0.85 * glottal_source));
                sample_val /= (a1 + a2 + a3);
            }

            if (s < 100) sample_val *= (static_cast<double>(s) / 100.0);
            if (s > total_samples - 250) sample_val *= (static_cast<double>(total_samples - s) / 250.0);

            pcm_data.push_back(static_cast<short>(sample_val * 32767));
        }
    }

    std::ofstream file(output_filename, std::ios::binary);
    if (!file) return 1;

    int size = pcm_data.size() * sizeof(short);
    write_wav_header(file, size);
    file.write(reinterpret_cast<const char*>(pcm_data.data()), size);
    file.close();

    std::cout << "[+] Audio render complete: " << output_filename << "\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}

