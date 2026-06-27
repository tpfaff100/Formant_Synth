//clang++ -std=c++20 -O3 perfect_knc.cpp -o perfect_knc
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
const double F0_BASE = 125.0; 

struct FormantData {
    double f1, f2, f3, fn; 
    double a1, a2, a3, an; 
};

struct PhoneticRule {
    double duration;
    FormantData start;
    FormantData end;
    bool is_fricative;
    bool is_plosive;
    bool is_nasal;         
    double vot_delay;      // Voice Onset Time: % of duration that remains completely voiceless/aspirated
};

std::map<char, PhoneticRule> PHONEMES = {
    // --- Vowels (VOT is 0, instant voicing) ---
    {'a', {.duration = 0.32, .start = {730, 1090, 2440, 250, 1.0, 0.8, 0.4, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4, 0.0}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'e', {.duration = 0.32, .start = {530, 1840, 2480, 250, 1.0, 0.7, 0.5, 0.0}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5, 0.0}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'i', {.duration = 0.32, .start = {270, 2290, 3010, 250, 1.0, 0.9, 0.6, 0.0}, .end = {270, 2290, 3010, 250, 1.0, 0.9, 0.6, 0.0}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'o', {.duration = 0.32, .start = {570,  840, 2410, 250, 1.0, 0.6, 0.3, 0.0}, .end = {570,  840, 2410, 250, 1.0, 0.6, 0.3, 0.0}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'u', {.duration = 0.32, .start = {300,  870, 2240, 250, 1.0, 0.5, 0.2, 0.0}, .end = {300,  870, 2240, 250, 1.0, 0.5, 0.2, 0.0}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},

    // --- Fixed 'C' and 'K' (High initial pinch, zero voicing for the first 18% of the sound) ---

// 'c' -> High-Pass Energy Strike: High initial F2/F3 pinch, F1 volume suppressed to 0.05 to eliminate the 'D/G' thud
    {'c', {.duration = 0.22, .start = {350, 2400, 2900, 250, 0.05, 1.0, 0.9, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true,  .is_nasal = false, .vot_delay = 0.0}},
    
    // 'k' -> Sharpened snap transition matching 'c' but with a compressed execution window
    {'k', {.duration = 0.22, .start = {350, 2400, 2900, 250, 0.05, 1.0, 0.9, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true,  .is_nasal = false, .vot_delay = 0.0}},

    // --- Other Consonants ---
    {'b', {.duration = 0.28, .start = {150,  700, 1300, 250, 0.8, 0.3, 0.1, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true,  .is_nasal = false, .vot_delay = 0.0}},
    {'d', {.duration = 0.28, .start = {250, 1800, 2600, 250, 0.9, 0.8, 0.5, 0.0}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = true,  .is_nasal = false, .vot_delay = 0.0}},
    {'g', {.duration = 0.28, .start = {300, 1500, 2100, 250, 0.8, 0.5, 0.3, 0.0}, .end = {570,  840, 2410, 250, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = true,  .is_nasal = false, .vot_delay = 0.0}},
    {'j', {.duration = 0.32, .start = {250, 2100, 2900, 250, 0.8, 0.8, 0.5, 0.0}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5}, .is_fricative = true,  .is_plosive = true,  .is_nasal = false, .vot_delay = 0.0}},
    {'p', {.duration = 0.26, .start = {150,  600, 1100, 250, 0.1, 0.4, 0.2, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true,  .is_nasal = false, .vot_delay = 0.15}},
    {'q', {.duration = 0.35, .start = {300, 1400, 2400, 250, 0.6, 0.4, 0.2, 0.0}, .end = {300,  870, 2240, 250, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = true,  .is_nasal = false, .vot_delay = 0.10}},
    {'t', {.duration = 0.26, .start = {400, 2400, 3100, 250, 0.1, 0.6, 0.5, 0.0}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = true,  .is_nasal = false, .vot_delay = 0.15}},
    {'f', {.duration = 0.32, .start = {2600, 3600, 4800, 250, 0.3, 0.3, 0.2, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'h', {.duration = 0.28, .start = {1000, 2000, 3000, 250, 0.3, 0.3, 0.2, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'s', {.duration = 0.36, .start = {5200, 6500, 7800, 250, 0.6, 0.9, 0.8, 0.0}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5}, .is_fricative = true,  .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'v', {.duration = 0.32, .start = {300,  1300, 2500, 250, 0.6, 0.5, 0.3, 0.0}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5}, .is_fricative = true,  .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'x', {.duration = 0.40, .start = {530,  1840, 2480, 250, 0.9, 0.6, 0.4, 0.0}, .end = {5500, 6800, 7800, 250, 0.4, 0.8, 0.8}, .is_fricative = true,  .is_plosive = true,  .is_nasal = false, .vot_delay = 0.0}},
    {'z', {.duration = 0.36, .start = {4200, 5200, 6200, 250, 0.6, 0.7, 0.6, 0.0}, .end = {270, 2290, 3010, 250, 1.0, 0.9, 0.6}, .is_fricative = true,  .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'m', {.duration = 0.28, .start = {280,  900, 2200, 250, 0.7, 0.2, 0.1, 0.8}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4, 0.0}, .is_fricative = false, .is_plosive = false, .is_nasal = true,  .vot_delay = 0.0}},
    {'n', {.duration = 0.28, .start = {280, 1700, 2300, 250, 0.7, 0.2, 0.1, 0.8}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5, 0.0}, .is_fricative = false, .is_plosive = false, .is_nasal = true,  .vot_delay = 0.0}},
    {'l', {.duration = 0.32, .start = {310, 1050, 2600, 250, 0.8, 0.4, 0.2, 0.0}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'r', {.duration = 0.32, .start = {320,  950, 1300, 250, 0.8, 0.6, 0.5, 0.0}, .end = {530, 1840, 2480, 250, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'w', {.duration = 0.32, .start = {290,  610, 2150, 250, 0.9, 0.3, 0.1, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}},
    {'y', {.duration = 0.32, .start = {260, 2300, 3050, 250, 1.0, 0.9, 0.5, 0.0}, .end = {730, 1090, 2440, 250, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .is_nasal = false, .vot_delay = 0.0}}
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <string>\n";
        return 1;
    }

    std::string input_text = argv[1];
    std::string output_filename = "perfect_speech.wav";
    std::transform(input_text.begin(), input_text.end(), input_text.begin(), ::tolower);

    std::vector<short> pcm_data;
    double p0 = 0.0, p1 = 0.0, p2 = 0.0, p3 = 0.0, pn = 0.0;
    unsigned int noise_seed = 77777;

    std::cout << "[*] Processing with Voice Onset Time (VOT) Controls...\n";

    for (char c : input_text) {
        if (c == ' ') {
            int space_samples = static_cast<int>(0.18 * SAMPLE_RATE);
            for (int s = 0; s < space_samples; ++s) pcm_data.push_back(0);
            continue;
        }

        if (!PHONEMES.contains(c)) continue;
        const auto& ph = PHONEMES[c];
        int total_samples = static_cast<int>(ph.duration * SAMPLE_RATE);

        for (int s = 0; s < total_samples; ++s) {
            double linear_progress = static_cast<double>(s) / total_samples;
            
            // Check if we are currently inside the unvoiced/aspirated delay window
            bool is_currently_unvoiced = (linear_progress < ph.vot_delay);

            double warp_progress = linear_progress;
            if (ph.is_plosive) {
                warp_progress = 1.0 - std::exp(-12.0 * linear_progress);
            } else if (ph.is_fricative) {
                warp_progress = std::pow(linear_progress, 1.8);
            }

            double f1 = ph.start.f1 + warp_progress * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + warp_progress * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + warp_progress * (ph.end.f3 - ph.start.f3);
            double fn = ph.start.fn + warp_progress * (ph.end.fn - ph.start.fn);

            double a1 = ph.start.a1 + linear_progress * (ph.end.a1 - ph.start.a1);
            double a2 = ph.start.a2 + linear_progress * (ph.end.a2 - ph.start.a2);
            double a3 = ph.start.a3 + linear_progress * (ph.end.a3 - ph.start.a3);
            double an = ph.start.an + linear_progress * (ph.end.an - ph.start.an);

            // Vocal cord step logic
            p0 += (2.0 * PI * F0_BASE) / SAMPLE_RATE;
            if (p0 > 2.0 * PI) p0 -= 2.0 * PI;
            
            // If the voice onset delay hasn't finished, glottal source is forced to 0
            double glottal_source = is_currently_unvoiced ? 0.0 : get_glottal_pulse(p0);

            // Generate a rapid random white noise sample for aspiration/frication checks
            noise_seed = noise_seed * 1103515245 + 12345;
            double noise_sample = (static_cast<double>(noise_seed % 2000) / 1000.0) - 1.0; 
            double noise_phase_dev = noise_sample * PI;

            if (ph.is_fricative || is_currently_unvoiced) {
                p1 += (2.0 * PI * f1) / SAMPLE_RATE + (noise_phase_dev * 0.3);
                p2 += (2.0 * PI * f2) / SAMPLE_RATE + (noise_phase_dev * 0.3);
                p3 += (2.0 * PI * f3) / SAMPLE_RATE + (noise_phase_dev * 0.3);
            } else {
                p1 += (2.0 * PI * f1) / SAMPLE_RATE;
                p2 += (2.0 * PI * f2) / SAMPLE_RATE;
                p3 += (2.0 * PI * f3) / SAMPLE_RATE;
            }
            pn += (2.0 * PI * fn) / SAMPLE_RATE;

            if (p1 > 2.0 * PI) p1 -= 2.0 * PI;
            if (p2 > 2.0 * PI) p2 -= 2.0 * PI;
            if (p3 > 2.0 * PI) p3 -= 2.0 * PI;
            if (pn > 2.0 * PI) pn -= 2.0 * PI;

            double sample_val = 0.0;
            if (ph.is_nasal && linear_progress < 0.4) {
                sample_val = an * std::sin(pn) * glottal_source;
            } else if (is_currently_unvoiced) {
                // TRUE VOICE ONSET TIME CORES: Pure unvoiced air burst passing through high-frequency resonators
                sample_val = (0.1 * std::sin(p1) * noise_sample) + 
                             (a2  * std::sin(p2) * noise_sample) + 
                             (a3  * std::sin(p3) * noise_sample);
                sample_val /= (0.1 + a2 + a3);
                sample_val *= (1.0 - linear_progress / ph.vot_delay); // Burst decay curve
            } else {
                // Post-VOT: The vocal tract stabilizes and turns into a fully voiced harmonic sound
                sample_val = (a1 * std::sin(p1) * (0.3 + 0.7 * glottal_source)) + 
                             (a2 * std::sin(p2) * (0.2 + 0.8 * glottal_source)) + 
                             (a3 * std::sin(p3) * (0.1 + 0.9 * glottal_source));
                sample_val /= (a1 + a2 + a3);
            }

            if (s < 200) sample_val *= (static_cast<double>(s) / 200.0);
            if (s > total_samples - 400) sample_val *= (static_cast<double>(total_samples - s) / 400.0);

            pcm_data.push_back(static_cast<short>(sample_val * 32767));
        }
    }

    std::ofstream file(output_filename, std::ios::binary);
    if (!file) return 1;

    int size = pcm_data.size() * sizeof(short);
    write_wav_header(file, size);
    file.write(reinterpret_cast<const char*>(pcm_data.data()), size);
    file.close();

    std::cout << "[+] Rendering complete!\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}
