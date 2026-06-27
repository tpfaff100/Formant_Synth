//clang++ -std=c++20 -O3 phoneme_synth.cpp -o phoneme_synth
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <cstdlib>
#include <algorithm>

const int SAMPLE_RATE = 22050;
const double PI = 3.14159265358979323846;
const double F0_BASE = 115.0; // Dropped to a more natural masculine/neutral speech weight

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

// True ARPAbet-inspired Phoneme Dictionary Matrix
std::map<std::string, PhoneticRule> PHONEMES = {
    // --- VOWELS & DIPHTHONGS ---
    {"EH", {.duration = 0.20, .start = {530, 1840, 2480, 1.0, 0.7, 0.5}, .end = {530, 1840, 2480, 1.0, 0.7, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // pEt, nEk
    {"AE", {.duration = 0.24, .start = {750, 1600, 2400, 1.0, 0.8, 0.4}, .end = {750, 1600, 2400, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // mAc, App-le
    {"AH", {.duration = 0.18, .start = {600, 1170, 2300, 1.0, 0.7, 0.3}, .end = {600, 1170, 2300, 1.0, 0.7, 0.3}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // kam-Ador, han-dred
    {"AA", {.duration = 0.22, .start = {730, 1090, 2440, 1.0, 0.8, 0.4}, .end = {730, 1090, 2440, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // mAd-el, tAs
    {"AO", {.duration = 0.24, .start = {570,  840, 2410, 1.0, 0.6, 0.3}, .end = {570,  840, 2410, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // fOr, kam-Ador
    {"IY", {.duration = 0.22, .start = {270, 2290, 3010, 1.0, 0.9, 0.6}, .end = {270, 2290, 3010, 1.0, 0.9, 0.6}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // tI, nI, twI
    {"UW", {.duration = 0.24, .start = {300,  870, 2240, 1.0, 0.5, 0.2}, .end = {300,  870, 2240, 1.0, 0.5, 0.2}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // tUW (two), pyUW-ter
    {"AY", {.duration = 0.32, .start = {750, 1200, 2400, 1.0, 0.8, 0.4}, .end = {300, 2100, 2900, 0.8, 0.9, 0.5}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // nAYn, fAYv
    {"EY", {.duration = 0.28, .start = {530, 1840, 2480, 1.0, 0.7, 0.5}, .end = {350, 2300, 3000, 0.7, 0.9, 0.6}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // EYt, stEY-sen
    {"OW", {.duration = 0.28, .start = {500,  850, 2300, 1.0, 0.6, 0.2}, .end = {350,  750, 2100, 0.8, 0.4, 0.1}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // zer-OW, taused-OW
    {"AW", {.duration = 0.32, .start = {700, 1200, 2400, 1.0, 0.7, 0.3}, .end = {350,  850, 2100, 0.8, 0.5, 0.2}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}, // tAW-sed (thousand)

    // --- CONSONANTS, PLOSIVES & LIQUIDS ---
    {"P",  {.duration = 0.12, .start = {150,  600, 1100, 0.0, 0.4, 0.2}, .end = {500, 1200, 2400, 0.8, 0.6, 0.3}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.025}},
    {"T",  {.duration = 0.12, .start = {400, 2400, 3200, 0.0, 0.9, 0.9}, .end = {500, 1500, 2500, 0.8, 0.7, 0.4}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.028}},
    {"K",  {.duration = 0.14, .start = {350, 2200, 3000, 0.0, 1.0, 0.9}, .end = {600, 1400, 2400, 0.9, 0.7, 0.4}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.035}},
    {"B",  {.duration = 0.14, .start = {160,  650, 1100, 0.8, 0.3, 0.1}, .end = {600, 1100, 2300, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.015}},
    {"D",  {.duration = 0.14, .start = {250, 1800, 2600, 0.8, 0.8, 0.5}, .end = {500, 1500, 2500, 1.0, 0.7, 0.4}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.018}},
    {"G",  {.duration = 0.14, .start = {250, 1400, 2000, 0.7, 0.5, 0.2}, .end = {550, 1100, 2300, 1.0, 0.6, 0.3}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.018}},
    {"S",  {.duration = 0.22, .start = {5200, 6500, 7800, 0.1, 0.9, 0.9}, .end = {4500, 6000, 7500, 0.1, 0.8, 0.8}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"SH", {.duration = 0.24, .start = {2800, 3800, 4800, 0.2, 0.9, 0.8}, .end = {2500, 3500, 4500, 0.2, 0.8, 0.7}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}}, // maki-n-taSH, stei-SH-en
    {"Z",  {.duration = 0.20, .start = {4200, 5200, 6200, 0.6, 0.7, 0.6}, .end = {3500, 4800, 5800, 0.7, 0.6, 0.5}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}}, // eg-Z-ekutive
    {"F",  {.duration = 0.20, .start = {2600, 3600, 4800, 0.2, 0.3, 0.2}, .end = {2000, 3000, 4000, 0.3, 0.3, 0.2}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}}, // fOr, fAYv
    {"V",  {.duration = 0.18, .start = {300,  1300, 2500, 0.6, 0.5, 0.3}, .end = {250,  1100, 2200, 0.7, 0.4, 0.2}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}}, // fiv-V, egzekuti-V
    {"M",  {.duration = 0.16, .start = {280,  900, 2200, 0.8, 0.2, 0.1}, .end = {280,  900, 2200, 0.8, 0.2, 0.1}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"N",  {.duration = 0.16, .start = {280, 1700, 2300, 0.8, 0.2, 0.1}, .end = {280, 1700, 2300, 0.8, 0.2, 0.1}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"L",  {.duration = 0.16, .start = {310, 1050, 2600, 0.8, 0.4, 0.2}, .end = {400, 1400, 2400, 0.9, 0.5, 0.3}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"R",  {.duration = 0.18, .start = {320,  950, 1300, 0.8, 0.6, 0.5}, .end = {420, 1200, 1500, 0.8, 0.6, 0.6}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"W",  {.duration = 0.14, .start = {290,  610, 2150, 0.9, 0.3, 0.1}, .end = {400,  900, 2300, 1.0, 0.6, 0.2}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"Y",  {.duration = 0.14, .start = {260, 2300, 3050, 1.0, 0.9, 0.5}, .end = {350, 1800, 2600, 1.0, 0.8, 0.4}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}
};

// Menu definitions completely translated to structured token vectors
std::map<int, std::pair<std::string, std::vector<std::string>>> MENU = {
    {1,  {"Pet 2001", {"P", "EH", "T", " ", "T", "UW", " ", "T", "AW", "S", "EH", "N", "D", " ", "W", "AH", "N"}}},
    {2,  {"t-r-s-80", {"T", "R", "S", " ", "EY", "T", "IY"}}},
    {3,  {"apple two", {"AE", "P", "L", " ", "T", "UW"}}},
    {4,  {"NEC PC8201a", {"N", "EH", "K", " ", "P", "IY", "S", "IY", " ", "EY", "T", " ", "T", "UW", " ", "Z", "IY", "R", "OW", " ", "W", "AH", "N", " ", "EY"}}},
    {5,  {"Model 100", {"M", "AA", "D", "EH", "L", " ", "W", "AH", "N", " ", "H", "AH", "N", "D", "R", "EH", "D"}}},
    {6,  {"Sparcstation", {"S", "P", "AA", "R", "K", " ", "S", "T", "EY", "SH", "EH", "N"}}},
    {7,  {"NeXT Computer", {"N", "EH", "K", "S", "T", " ", "K", "AH", "M", "P", "Y", "UW", "T", "EH", "R"}}},
    {8,  {"Macintosh", {"M", "AE", "K", "IH", "N", "T", "AA", "SH"}}}, // Remapped 'ih' approximation to 'eh' smoothly
    {9,  {"Commodore 64", {"K", "AA", "M", "AH", "D", "OW", "R", " ", "S", "IH", "K", "S", "T", "IY", " ", "F", "AO", "R"}}},
    {10, {"Commodore 64 executive", {"K", "AA", "M", "AH", "D", "OW", "R", " ", "S", "IH", "K", "S", "T", "IY", " ", "F", "AO", "R", " ", "EH", "G", "Z", "EH", "K", "Y", "UW", "T", "IH", "V"}}},
    {11, {"HP 9825", {"EH", "T", "S", " ", "P", "IY", " ", "N", "AY", "N", " ", "EY", "T", " ", "T", "UW", " ", "F", "AY", "V"}}},
    {12, {"HP 9000-712", {"EH", "T", "S", " ", "P", "IY", " ", "N", "AY", "N", " ", "T", "AW", "S", "EH", "N", "D", " ", "S", "EH", "V", "EH", "N", " ", "W", "AH", "N", " ", "T", "UW"}}},
    {13, {"HP2000", {"EH", "T", "S", " ", "P", "IY", " ", "T", "UW", " ", "T", "AW", "S", "EH", "N", "D"}}},
    {14, {"HP3000", {"EH", "T", "S", " ", "P", "IY", " ", "T", "R", "IY", " ", "T", "AW", "S", "EH", "N", "D"}}}
};

// Fallback handling if a sub-token alias is routed
std::string resolve_token(std::string t) {
    if (t == "IH") return "EH"; // Route short 'i' to efficient resonant 'eh' boundary
    return t;
}

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
    std::cout << "=== True Phonemic Retro Hardware Speech Engine ===\n";
    for (const auto& [id, target] : MENU) {
        std::cout << "  [" << id << "] " << target.first << "\n";
    }
    std::cout << "Usage: ./phoneme_synth <number>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_menu();
        return 1;
    }

    int choice = std::atoi(argv[1]);
    if (!MENU.contains(choice)) {
        std::cout << "[-] Invalid selection.\n";
        print_menu();
        return 1;
    }

    auto [label, phoneme_vector] = MENU[choice];
    std::cout << "[*] Synthesizing complex phonetic tokens for: \"" << label << "\"\n";

    std::string output_filename = "phoneme_output.wav";
    std::vector<short> pcm_data;
    double p0 = 0.0, p1 = 0.0, p2 = 0.0, p3 = 0.0;
    unsigned int noise_seed = 424242;

    for (std::string raw_token : phoneme_vector) {
        if (raw_token == " ") {
            int space_samples = static_cast<int>(0.06 * SAMPLE_RATE); // Tight phoneme gap compression
            for (int s = 0; s < space_samples; ++s) pcm_data.push_back(0);
            continue;
        }

        std::string tok = resolve_token(raw_token);
        if (!PHONEMES.contains(tok)) continue;
        const auto& ph = PHONEMES[tok];
        
        int total_samples = static_cast<int>(ph.duration * SAMPLE_RATE);
        int burst_samples = static_cast<int>(ph.burst_duration * SAMPLE_RATE);

        for (int s = 0; s < total_samples; ++s) {
            double linear_progress = static_cast<double>(s) / total_samples;
            
            double warp_progress = linear_progress;
            if (ph.is_plosive) {
                warp_progress = 1.0 - std::exp(-12.0 * linear_progress);
            } else if (ph.is_fricative) {
                warp_progress = std::pow(linear_progress, 1.5);
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
                sample_val *= (burst_envelope * 1.4);
            } else if (ph.is_fricative) {
                sample_val = (a1 * std::sin(p1) * 0.2) + 
                             (a2 * std::sin(p2) * (0.4 * noise + 0.6)) + 
                             (a3 * std::sin(p3) * (0.7 * noise + 0.3));
                sample_val /= (a1 + a2 + a3);
            } else {
                sample_val = (a1 * std::sin(p1) * (0.35 + 0.65 * glottal_source)) + 
                             (a2 * std::sin(p2) * (0.25 + 0.75 * glottal_source)) + 
                             (a3 * std::sin(p3) * (0.15 + 0.85 * glottal_source));
                sample_val /= (a1 + a2 + a3);
            }

            // Smoother boundary damping to keep syllables locked together tightly
            if (s < 80) sample_val *= (static_cast<double>(s) / 80.0);
            if (s > total_samples - 200) sample_val *= (static_cast<double>(total_samples - s) / 200.0);

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
