//clang++ -std=c++20 -O3 cascade_synth.cpp -o cascade_synth

/*
 please improve the pronunciation so that it sounds like recognizable english

The reason the synthesis still sounds a bit like an alien transmission rather than recognizable English comes down to articulation filters.

Right now, the code uses pure sine waves for formants (F1​, F2​, F3​). In a real human vocal tract, formants aren't pristine laser beams; they are broad, resonant bands of acoustic energy with specific bandwidths. Without these bandwidths, vowels sound metallic, and consonants lose their identity entirely. Furthermore, a real vocal tract acts as a series of cascading filters rather than a simple mixer.

To bridge the gap to recognizable English, we need to replace the primitive oscillator mixer with a Klatt-style Formant Cascade Filter. This architecture passes a raw source signal (either a rich glottal pulse train or white noise) through three resonant bandpass filters in series.

Here is the fully upgraded program. It introduces proper resonators, explicit bandwidth constants, and re-tuned phoneme targets.
The Resonant Cascade Speech Engine (cascade_synth.cpp)
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
const double F0_BASE = 110.0; // Steady, low speech register

struct FormantData {
    double f1, f2, f3;
    double b1, b2, b3; // Bandwidths in Hz (Crucial for natural resonance)
};

struct PhoneticRule {
    double duration;
    FormantData start;
    FormantData end;
    bool is_fricative;
    bool is_plosive;
    double burst_duration; 
};

// Formant and Bandwidth definitions tuned for a 3-filter cascade
std::map<std::string, PhoneticRule> PHONEMES = {
    // VOWELS (Frequencies & Bandwidths)
    {"EH", {.duration = 0.18, .start = {550, 1800, 2500, 60, 90, 120}, .end = {550, 1800, 2500, 60, 90, 120}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AE", {.duration = 0.22, .start = {700, 1600, 2400, 60, 80, 120}, .end = {700, 1600, 2400, 60, 80, 120}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AH", {.duration = 0.16, .start = {600, 1200, 2400, 60, 70, 110}, .end = {600, 1200, 2400, 60, 70, 110}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AA", {.duration = 0.20, .start = {750, 1100, 2450, 70, 70, 120}, .end = {750, 1100, 2450, 70, 70, 120}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AO", {.duration = 0.22, .start = {550,  850, 2400, 60, 70, 110}, .end = {550,  850, 2400, 60, 70, 110}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"IY", {.duration = 0.20, .start = {280, 2250, 2900, 50, 100, 150}, .end = {280, 2250, 2900, 50, 100, 150}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"UW", {.duration = 0.22, .start = {300,  900, 2200, 50, 80, 140},  .end = {300,  900, 2200, 50, 80, 140},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    
    // DIPHTHONGS (Dynamic targets)
    {"AY", {.duration = 0.28, .start = {750, 1100, 2400, 70, 70, 120}, .end = {300, 2000, 2700, 50, 100, 140}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"EY", {.duration = 0.26, .start = {550, 1800, 2500, 60, 90, 120}, .end = {300, 2100, 2800, 50, 100, 140}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"OW", {.duration = 0.26, .start = {550,  900, 2300, 60, 80, 120}, .end = {350,  750, 2100, 50, 70, 110},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AW", {.duration = 0.28, .start = {700, 1200, 2400, 60, 80, 120}, .end = {350,  850, 2100, 50, 70, 110},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},

    // CONSONANTS & TRANSITIONS
    {"P",  {.duration = 0.10, .start = {200,  700, 1200, 80, 100, 150}, .end = {500, 1100, 2300, 60, 80, 120}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.020}},
    {"T",  {.duration = 0.10, .start = {400, 2000, 3000, 90, 120, 200}, .end = {500, 1400, 2400, 60, 80, 120}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.025}},
    {"K",  {.duration = 0.12, .start = {350, 1800, 2700, 80, 110, 180}, .end = {550, 1200, 2300, 60, 80, 120}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.030}},
    {"B",  {.duration = 0.12, .start = {180,  700, 1200, 60, 90, 130}, .end = {600, 1100, 2300, 60, 80, 120}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.012}},
    {"D",  {.duration = 0.12, .start = {250, 1600, 2500, 60, 90, 140}, .end = {500, 1400, 2400, 60, 80, 120}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.012}},
    {"G",  {.duration = 0.12, .start = {250, 1300, 2000, 60, 90, 130}, .end = {550, 1100, 2300, 60, 80, 120}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.012}},
    {"S",  {.duration = 0.18, .start = {4500, 5500, 7000, 200, 300, 400}, .end = {4000, 5000, 6500, 200, 300, 400}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"SH", {.duration = 0.20, .start = {2500, 3500, 4500, 150, 250, 350}, .end = {2200, 3200, 4200, 150, 250, 350}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"Z",  {.duration = 0.16, .start = {3500, 4500, 5500, 150, 250, 350}, .end = {3000, 4000, 5000, 150, 250, 350}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"F",  {.duration = 0.16, .start = {2000, 3000, 4000, 300, 400, 500}, .end = {1800, 2800, 3800, 300, 400, 500}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"V",  {.duration = 0.14, .start = {300,  1200, 2200, 80, 150, 250}, .end = {250,  1100, 2100, 80, 150, 250}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"M",  {.duration = 0.14, .start = {250,  850, 2100, 50, 80, 150}, .end = {250,  850, 2100, 50, 80, 150}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"N",  {.duration = 0.14, .start = {250, 1550, 2200, 50, 80, 150}, .end = {250, 1550, 2200, 50, 80, 150}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"L",  {.duration = 0.14, .start = {300, 1000, 2500, 50, 80, 120}, .end = {400, 1300, 2400, 50, 80, 120}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"R",  {.duration = 0.15, .start = {310,  950, 1400, 50, 70, 100}, .end = {400, 1200, 1600, 50, 70, 100}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"W",  {.duration = 0.12, .start = {290,  650, 2100, 50, 70, 130}, .end = {400,  950, 2200, 50, 70, 130}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"Y",  {.duration = 0.12, .start = {260, 2100, 2900, 50, 90, 140}, .end = {350, 1700, 2500, 50, 90, 140}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}
};

std::map<int, std::pair<std::string, std::vector<std::string>>> MENU = {
    {1,  {"Pet 2001", {"P", "EH", "T", " ", "T", "UW", " ", "T", "AW", "S", "EH", "N", "D", " ", "W", "AH", "N"}}},
    {2,  {"t-r-s-80", {"T", "R", "S", " ", "EY", "T", "IY"}}},
    {3,  {"apple two", {"AE", "P", "L", " ", "T", "UW"}}},
    {4,  {"NEC PC8201a", {"N", "EH", "K", " ", "P", "IY", "S", "IY", " ", "EY", "T", " ", "T", "UW", " ", "Z", "IY", "R", "OW", " ", "W", "AH", "N", " ", "EY"}}},
    {5,  {"Model 100", {"M", "AA", "D", "EH", "L", " ", "W", "AH", "N", " ", "H", "AH", "N", "D", "R", "EH", "D"}}},
    {6,  {"Sparcstation", {"S", "P", "AA", "R", "K", " ", "S", "T", "EY", "SH", "EH", "N"}}},
    {7,  {"NeXT Computer", {"N", "EH", "K", "S", "T", " ", "K", "AH", "M", "P", "Y", "UW", "T", "EH", "R"}}},
    {8,  {"Macintosh", {"M", "AE", "K", "EH", "N", "T", "AA", "SH"}}}, 
    {9,  {"Commodore 64", {"K", "AA", "M", "AH", "D", "OW", "R", " ", "S", "EH", "K", "S", "T", "IY", " ", "F", "AO", "R"}}},
    {10, {"Commodore 64 executive", {"K", "AA", "M", "AH", "D", "OW", "R", " ", "S", "EH", "K", "S", "T", "IY", " ", "F", "AO", "R", " ", "EH", "G", "Z", "EH", "K", "Y", "UW", "T", "EH", "V"}}},
    {11, {"HP 9825", {"EH", "T", "S", " ", "P", "IY", " ", "N", "AY", "N", " ", "EY", "T", " ", "T", "UW", " ", "F", "AY", "V"}}},
    {12, {"HP 9000-712", {"EH", "T", "S", " ", "P", "IY", " ", "N", "AY", "N", " ", "T", "AW", "S", "EH", "N", "D", " ", "S", "EH", "V", "EH", "N", " ", "W", "AH", "N", " ", "T", "UW"}}},
    {13, {"HP2000", {"EH", "T", "S", " ", "P", "IY", " ", "T", "UW", " ", "T", "AW", "S", "EH", "N", "D"}}},
    {14, {"HP3000", {"EH", "T", "S", " ", "P", "IY", " ", "T", "R", "IY", " ", "T", "AW", "S", "EH", "N", "D"}}}
};

// Two-pole Resonator Filter Class (Klatt Formant Architecture)
class Resonator {
private:
    double a, b, c;
    double y1, y2;
public:
    Resonator() : a(0), b(0), c(0), y1(0), y2(0) {}
    
    void set_parameters(double frequency, double bandwidth) {
        double r = std::exp(-PI * bandwidth / SAMPLE_RATE);
        c = -r * r;
        b = 2.0 * r * std::cos(2.0 * PI * frequency / SAMPLE_RATE);
        a = 1.0 - b - c;
    }
    
    double process(double input) {
        double output = a * input + b * y1 + c * y2;
        y2 = y1;
        y1 = output;
        return output;
    }
};

double get_glottal_pulse(double phase) {
    double normalized_phase = phase / (2.0 * PI);
    if (normalized_phase < 0.25) {
        return 3.0 * std::pow(normalized_phase / 0.25, 2) - 2.0 * std::pow(normalized_phase / 0.25, 3);
    } else if (normalized_phase < 0.35) {
        return 1.0 - std::pow((normalized_phase - 0.25) / 0.10, 2);
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
        std::cout << "Usage: ./cascade_synth <number (1-14)>\n";
        return 1;
    }

    int choice = std::atoi(argv[1]);
    if (!MENU.contains(choice)) {
        std::cout << "[-] Selection must be between 1 and 14.\n";
        return 1;
    }

    auto [label, phoneme_vector] = MENU[choice];
    std::cout << "[*] Running cascade articulation for: \"" << label << "\"\n";

    std::string output_filename = "cascade_output.wav";
    std::vector<short> pcm_data;
    
    double p0 = 0.0;
    unsigned int noise_seed = 12345;

    // Instantiate 3 cascade formants
    Resonator R1, R2, R3;

    for (const std::string& tok : phoneme_vector) {
        if (tok == " ") {
            int space_samples = static_cast<int>(0.05 * SAMPLE_RATE);
            for (int s = 0; s < space_samples; ++s) pcm_data.push_back(0);
            continue;
        }

        if (!PHONEMES.contains(tok)) continue;
        const auto& ph = PHONEMES[tok];
        int total_samples = static_cast<int>(ph.duration * SAMPLE_RATE);
        int burst_samples = static_cast<int>(ph.burst_duration * SAMPLE_RATE);

        for (int s = 0; s < total_samples; ++s) {
            double progress = static_cast<double>(s) / total_samples;
            
            // Linear target interpolation for frequency and bandwidth
            double f1 = ph.start.f1 + progress * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + progress * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + progress * (ph.end.f3 - ph.start.f3);
            
            double b1 = ph.start.b1 + progress * (ph.end.b1 - ph.start.b1);
            double b2 = ph.start.b2 + progress * (ph.end.b2 - ph.start.b2);
            double b3 = ph.start.b3 + progress * (ph.end.b3 - ph.start.b3);

            // Update resonant poles dynamically
            R1.set_parameters(f1, b1);
            R2.set_parameters(f2, b2);
            R3.set_parameters(f3, b3);

            // Source calculation
            p0 += (2.0 * PI * F0_BASE) / SAMPLE_RATE;
            if (p0 > 2.0 * PI) p0 -= 2.0 * PI;
            
            noise_seed = noise_seed * 1103515245 + 12345;
            double noise = (static_cast<double>(noise_seed % 2000) / 1000.0) - 1.0;

            double source_signal = 0.0;

            if (s < burst_samples) {
                // Plosive release: High amplitude unvoiced impulse noise
                source_signal = noise * (1.0 - (static_cast<double>(s) / burst_samples));
            } else if (ph.is_fricative) {
                // Sibilant hiss source
                source_signal = noise * 0.60;
            } else {
                // Vocalic source: Glottal pulse train
                source_signal = get_glottal_pulse(p0);
            }

            // Execute Cascade Path: Source -> R1 -> R2 -> R3
            double filtered = R1.process(source_signal);
            filtered = R2.process(filtered);
            filtered = R3.process(filtered);

            // Automatic gain adaptation to balance fricatives vs voiced segments
            double gain = ph.is_fricative ? 0.08 : 0.25;
            double sample_val = filtered * gain;

            // syllabic smoothing envelopes
            if (s < 100) sample_val *= (static_cast<double>(s) / 100.0);
            if (s > total_samples - 200) sample_val *= (static_cast<double>(total_samples - s) / 200.0);

            // Clip prevention guard
            if (sample_val > 1.0) sample_val = 1.0;
            if (sample_val < -1.0) sample_val = -1.0;

            pcm_data.push_back(static_cast<short>(sample_val * 32767));
        }
    }

    std::ofstream file(output_filename, std::ios::binary);
    if (!file) return 1;

    int size = pcm_data.size() * sizeof(short);
    write_wav_header(file, size);
    file.write(reinterpret_cast<const char*>(pcm_data.data()), size);
    file.close();

    std::cout << "[+] Render Complete. Running playback...\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}

