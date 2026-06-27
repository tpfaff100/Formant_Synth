//clang++ -std=c++20 -O3 dual_source_speech.cpp -o dual_source_speech

/*By using smooth sine waves or a glottal pulse, we are trying to make a hard "C" or "K" out of a continuous hum. But in human anatomy, a "K" is a transient explosion. When your tongue drops from the soft palate, it releases a high-pressure burst of chaotic, turbulent air. If you don't have that chaotic burst, it will always sound like a voiced vowel transition ("daw").

To fix this once and for all, we must implement a Dual-Source Engine. When a plosive like 'c' or 'k' hits, the engine must completely decouple from the harmonic voice oscillator for the first 30 milliseconds and fire a high-pass filtered white noise burst.

Here is the ultimate rewrite of the synthesis loop. It splits the excitation source into two distinct physical models: a noise-burst transient generator for consonants and a glottal pulse train for vowels.
The Dual-Source Production Engine (dual_source_speech.cpp)
*/

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
    double f1, f2, f3;
    double a1, a2, a3;
};

struct PhoneticRule {
    double duration;
    FormantData start;
    FormantData end;
    bool is_fricative;
    bool is_plosive;
    double burst_duration; // Dynamic window (in seconds) for raw noise transient injection
};

std::map<char, PhoneticRule> PHONEMES = {
    // Vowels
    {'a', {.duration = 0.32, .start = {730, 1090, 2440, 1.0, 0.8, 0.4}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'e', {.duration = 0.32, .start = {530, 1840, 2480, 1.0, 0.7, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'i', {.duration = 0.32, .start = {270, 2290, 3010, 1.0, 0.9, 0.6}, .end = {270, 2290, 3010, 1.0, 0.9, 0.6}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'o', {.duration = 0.32, .start = {570,  840, 2410, 1.0, 0.6, 0.3}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'u', {.duration = 0.32, .start = {300,  870, 2240, 1.0, 0.5, 0.2}, .end = {300,  870, 2240, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},

// 'q' -> Labialized Velar Burst: Fires a 30ms explosion that drops into a tight, rounded vowel glide target
    {'q', {.duration = 0.38, .start = {300, 1800, 2700, 0.0, 1.0, 0.9}, .end = {300,  870, 2240, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.030}},
    
    // 'w' -> Standard vocalic glide frame to support the transition matrix
    {'w', {.duration = 0.35, .start = {290,  610, 2150, 0.9, 0.3, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},


    // --- REBUILT PLOSIVES WITH EXPLICIT NOISE TRANSIENT BURSTS ---
    // Instantly fires a 35ms high-frequency crack before bleeding into the vowel matrix
    {'c', {.duration = 0.25, .start = {350, 2200, 3000, 0.0, 1.0, 0.9}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.035}},
    {'k', {.duration = 0.25, .start = {350, 2200, 3000, 0.0, 1.0, 0.9}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.035}},
    
    {'b', {.duration = 0.25, .start = {180,  700, 1200, 0.8, 0.3, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.010}},
    {'d', {.duration = 0.25, .start = {250, 1800, 2600, 0.8, 0.8, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.015}},
    {'g', {.duration = 0.25, .start = {250, 1400, 2000, 0.7, 0.5, 0.2}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.015}},
    {'t', {.duration = 0.25, .start = {400, 2400, 3200, 0.0, 0.9, 0.9}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.030}},
    {'p', {.duration = 0.25, .start = {150,  600, 1100, 0.0, 0.4, 0.2}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = true, .burst_duration = 0.025}},

    // Fricatives & Liquids
    {'s', {.duration = 0.35, .start = {5000, 6500, 7800, 0.2, 0.9, 0.9}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'f', {.duration = 0.32, .start = {2600, 3600, 4800, 0.3, 0.3, 0.2}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'h', {.duration = 0.28, .start = {1000, 2000, 3000, 0.3, 0.3, 0.2}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'v', {.duration = 0.32, .start = {300,  1300, 2500, 0.6, 0.5, 0.3}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {'m', {.duration = 0.28, .start = {280,  900, 2200, 0.8, 0.2, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'n', {.duration = 0.28, .start = {280, 1700, 2300, 0.8, 0.2, 0.1}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'l', {.duration = 0.32, .start = {310, 1050, 2600, 0.8, 0.4, 0.2}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'r', {.duration = 0.32, .start = {320,  950, 1300, 0.8, 0.6, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'w', {.duration = 0.32, .start = {290,  610, 2150, 0.9, 0.3, 0.1}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {'y', {.duration = 0.32, .start = {260, 2300, 3050, 1.0, 0.9, 0.5}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},

// --- Digits (0 - 9) ---
    // '0' ("zero") -> High-frequency sibilant 'z' sliding into an 'ee-ro' vowel continuum
    {'0', {.duration = 0.42, .start = {350, 2100, 2900, 0.7, 0.6, 0.4}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    
    // '1' ("one") -> Labialized 'w' glide sweeping rapidly into a heavy nasal resonance 'n'
    {'1', {.duration = 0.38, .start = {290,  610, 2150, 0.8, 0.3, 0.1}, .end = {280, 1700, 2300, 0.8, 0.2, 0.1}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // '2' ("two") -> Sharp alveolar unvoiced burst 't' gliding into a deeply rounded 'oo' vowel
    {'2', {.duration = 0.35, .start = {400, 2400, 3200, 0.0, 0.9, 0.9}, .end = {300,  870, 2240, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.025}},
    
    // '3' ("three") -> Low-frequency dental fricative 'th' dropping into an 'r' liquid and an open 'ee' target
    {'3', {.duration = 0.40, .start = {1500, 2500, 3500, 0.4, 0.4, 0.3}, .end = {270, 2290, 3010, 1.0, 0.9, 0.6}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    
    // '4' ("four") -> Labiodental friction hiss 'f' sweeping cleanly up into a dark 'o-r' vowel space
    {'4', {.duration = 0.38, .start = {2600, 3600, 4800, 0.3, 0.3, 0.2}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    
    // '5' ("five") -> Clear 'f' friction leading to an open 'ai' diphthong and closing on a voiced 'v' buzz
    {'5', {.duration = 0.45, .start = {2600, 3600, 4800, 0.4, 0.3, 0.2}, .end = {300, 1300, 2500, 0.7, 0.6, 0.4}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    
    // '6' ("six") -> High-intensity sibilant 's', sharp mid-vowel drop, ending in a hard unvoiced 'ks' cluster
    {'6', {.duration = 0.46, .start = {5200, 6500, 7800, 0.7, 0.9, 0.8}, .end = {5500, 6800, 7800, 0.5, 0.8, 0.8}, .is_fricative = true,  .is_plosive = true,  .burst_duration = 0.030}},
    
    // '7' ("seven") -> Fricative 's' followed by an 'eh' transition that locks straight into a nasalized endpoint
    {'7', {.duration = 0.48, .start = {5200, 6500, 7800, 0.6, 0.8, 0.8}, .end = {280, 1700, 2300, 0.8, 0.2, 0.1}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    
    // '8' ("eight") -> Rapid vowel glide starting with 'e' and concluding with a 25ms unvoiced plosive release snap
    {'8', {.duration = 0.32, .start = {530, 1840, 2480, 1.0, 0.7, 0.5}, .end = {400, 2400, 3200, 0.0, 0.8, 0.8}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.025}},
    
    // '9' ("nine") -> Double nasal sandwich: Starts nasal 'n', sweeps through wide diphthong, falls back to 'n' cavity
    {'9', {.duration = 0.44, .start = {280, 1700, 2300, 0.8, 0.2, 0.1}, .end = {280, 1700, 2300, 0.8, 0.2, 0.1}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
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
    std::string output_filename = "dual_source_speech.wav";
    std::transform(input_text.begin(), input_text.end(), input_text.begin(), ::tolower);

    std::vector<short> pcm_data;
    double p0 = 0.0, p1 = 0.0, p2 = 0.0, p3 = 0.0;
    unsigned int noise_seed = 12345;

    std::cout << "[*] Executing Dual-Source Explosive Synthesis Engine...\n";

    for (char c : input_text) {
        if (c == ' ') {
            int space_samples = static_cast<int>(0.15 * SAMPLE_RATE);
            for (int s = 0; s < space_samples; ++s) pcm_data.push_back(0);
            continue;
        }

        if (!PHONEMES.contains(c)) continue;
        const auto& ph = PHONEMES[c];
        int total_samples = static_cast<int>(ph.duration * SAMPLE_RATE);
        int burst_samples = static_cast<int>(ph.burst_duration * SAMPLE_RATE);

        for (int s = 0; s < total_samples; ++s) {
            double linear_progress = static_cast<double>(s) / total_samples;
            
            // Warp rules for smooth transitions post-burst
            double warp_progress = linear_progress;
            if (ph.is_plosive) {
                warp_progress = 1.0 - std::exp(-10.0 * linear_progress);
            } else if (ph.is_fricative) {
                warp_progress = std::pow(linear_progress, 1.8);
            }

            double f1 = ph.start.f1 + warp_progress * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + warp_progress * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + warp_progress * (ph.end.f3 - ph.start.f3);

            double a1 = ph.start.a1 + linear_progress * (ph.end.a1 - ph.start.a1);
            double a2 = ph.start.a2 + linear_progress * (ph.end.a2 - ph.start.a2);
            double a3 = ph.start.a3 + linear_progress * (ph.end.a3 - ph.start.a3);

            // Maintain glottal tracking
            p0 += (2.0 * PI * F0_BASE) / SAMPLE_RATE;
            if (p0 > 2.0 * PI) p0 -= 2.0 * PI;
            double glottal_source = get_glottal_pulse(p0);

            // Generate White Noise
            noise_seed = noise_seed * 1103515245 + 12345;
            double noise = (static_cast<double>(noise_seed % 2000) / 1000.0) - 1.0;

            // Phase accumulation
            p1 += (2.0 * PI * f1) / SAMPLE_RATE;
            p2 += (2.0 * PI * f2) / SAMPLE_RATE;
            p3 += (2.0 * PI * f3) / SAMPLE_RATE;
            if (p1 > 2.0 * PI) p1 -= 2.0 * PI;
            if (p2 > 2.0 * PI) p2 -= 2.0 * PI;
            if (p3 > 2.0 * PI) p3 -= 2.0 * PI;

            double sample_val = 0.0;

            if (s < burst_samples) {
                // EXCITING TRANSITION: Complete bypass of glottal cords.
                // Replaces vocalic energy with a high-pass noise burst through the velar pinch frequencies.
                double burst_envelope = 1.0 - (static_cast<double>(s) / burst_samples);
                sample_val = (a2 * std::sin(p2) * noise) + (a3 * std::sin(p3) * noise);
                sample_val *= (burst_envelope * 1.2); // Amplified impact pop
            } else if (ph.is_fricative) {
                // Continuous friction modeling
                sample_val = (a1 * std::sin(p1)) + 
                             (a2 * std::sin(p2) * (0.3 * noise + 0.7)) + 
                             (a3 * std::sin(p3) * (0.6 * noise + 0.4));
                sample_val /= (a1 + a2 + a3);
            } else {
                // Fully Voiced harmonic tracking
                sample_val = (a1 * std::sin(p1) * (0.3 + 0.7 * glottal_source)) + 
                             (a2 * std::sin(p2) * (0.2 + 0.8 * glottal_source)) + 
                             (a3 * std::sin(p3) * (0.1 + 0.9 * glottal_source));
                sample_val /= (a1 + a2 + a3);
            }

            if (s < 150) sample_val *= (static_cast<double>(s) / 150.0);
            if (s > total_samples - 350) sample_val *= (static_cast<double>(total_samples - s) / 350.0);

            pcm_data.push_back(static_cast<short>(sample_val * 32767));
        }
    }

    std::ofstream file(output_filename, std::ios::binary);
    if (!file) return 1;

    int size = pcm_data.size() * sizeof(short);
    write_wav_header(file, size);
    file.write(reinterpret_cast<const char*>(pcm_data.data()), size);
    file.close();

    std::cout << "[+] Rendering complete via dual-excitation matrix!\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}

