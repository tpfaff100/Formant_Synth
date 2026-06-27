//clang++ -std=c++20 -O3 bad_gear_synth.cpp -o bad_gear_synth
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
const double F0_BASE = 115.0; // Slightly dropped pitch baseline for a resonant, speech-accurate weight

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

// Custom phoneme mapping tailored for the exact transition matrix of "bad gear"
std::map<char, PhoneticRule> PHONEMES = {
    // 'b' -> Heavy voiced labial burst transitioning to wide open vocalic 'ae'
    {'b', {.duration = 0.22, .start = {160,  650, 1100, 0.9, 0.4, 0.1}, .end = {750, 1600, 2400, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.012}},
    
    // 'a' -> Formant mapped explicitly to the 'æ' vowel (flat tongue, low-mid front)
    {'a', {.duration = 0.28, .start = {750, 1600, 2400, 1.0, 0.8, 0.4}, .end = {750, 1600, 2400, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // 'd' -> Voiced alveolar stop with an active trailing release window to secure the hard closure
    {'d', {.duration = 0.24, .start = {750, 1600, 2400, 1.0, 0.7, 0.3}, .end = {250, 1750, 2550, 0.8, 0.7, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.015}},
    
    // 'g' -> Voiced velar release backing immediately into the tight high-frequency profile of 'ee'
    {'g', {.duration = 0.20, .start = {280, 1450, 2100, 0.8, 0.5, 0.2}, .end = {280, 2250, 2900, 1.0, 0.9, 0.5}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.018}},
    
    // 'e' -> The 'ee' tense anchor core for "gear"
    {'e', {.duration = 0.26, .start = {280, 2250, 2900, 1.0, 0.9, 0.5}, .end = {280, 2250, 2900, 1.0, 0.9, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // 'r' -> Liquid rhotic tailpiece: Aggressive dive of F3 down to form the characteristic color
    {'r', {.duration = 0.30, .start = {280, 2250, 2900, 1.0, 0.7, 0.4}, .end = {420, 1200, 1500, 0.8, 0.6, 0.6}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}
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
    // Explicit sequence mapping for the target phrase "bad gear"
    std::string target_sequence = "bad ger"; 
    std::string output_filename = "bad_gear.wav";

    std::vector<short> pcm_data;
    double p0 = 0.0, p1 = 0.0, p2 = 0.0, p3 = 0.0;
    unsigned int noise_seed = 54321;

    std::cout << "[*] Compiling specialized dual-source phrase matrix for: \"bad gear\"...\n";

    for (char c : target_sequence) {
        if (c == ' ') {
            // Natural phrase gap cadence separation
            int space_samples = static_cast<int>(0.12 * SAMPLE_RATE);
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
                warp_progress = 1.0 - std::exp(-9.5 * linear_progress);
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
                // Initial explosive transient spike bypass
                double burst_envelope = 1.0 - (static_cast<double>(s) / burst_samples);
                sample_val = (a1 * std::sin(p1) * noise) + (a2 * std::sin(p2) * noise);
                sample_val *= (burst_envelope * 1.1); 
            } else {
                // Clean resonant vocal tract reconstruction
                sample_val = (a1 * std::sin(p1) * (0.35 + 0.65 * glottal_source)) + 
                             (a2 * std::sin(p2) * (0.25 + 0.75 * glottal_source)) + 
                             (a3 * std::sin(p3) * (0.15 + 0.85 * glottal_source));
                sample_val /= (a1 + a2 + a3);
            }

            // Window smoothing
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

    std::cout << "[+] Audio render complete: " << output_filename << "\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}

