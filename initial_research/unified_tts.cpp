//clang++ -std=c++20 -O3 unified_tts.cpp -o unified_tts
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <cctype>

const int SAMPLE_RATE = 22050;
const double PI = 3.14159265358979323846;
const double F0_BASE = 135.0; 

struct FormantData {
    double f1, f2, f3;
    double b1, b2, b3; 
};

struct PhoneticRule {
    double duration;
    FormantData start;
    FormantData end;
    bool is_fricative;
    bool is_plosive;
    double burst_duration; 
};

// Internal Master Phoneme Matrix
std::map<std::string, PhoneticRule> PHONEMES = {
    {"EH", {.duration = 0.16, .start = {620, 1950, 2650, 40, 45, 60}, .end = {620, 1950, 2650, 40, 45, 60}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AE", {.duration = 0.20, .start = {800, 1750, 2550, 45, 45, 65}, .end = {800, 1750, 2550, 45, 45, 65}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AH", {.duration = 0.14, .start = {680, 1350, 2500, 40, 45, 60}, .end = {680, 1350, 2500, 40, 45, 60}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AA", {.duration = 0.18, .start = {820, 1220, 2600, 45, 45, 65}, .end = {820, 1220, 2600, 45, 45, 65}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AO", {.duration = 0.20, .start = {620,  950, 2500, 40, 45, 60}, .end = {620,  950, 2500, 40, 45, 60}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"IY", {.duration = 0.18, .start = {320, 2400, 3100, 35, 55, 75}, .end = {320, 2400, 3100, 35, 55, 75}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"UW", {.duration = 0.20, .start = {350, 1050, 2350, 35, 50, 75}, .end = {350, 1050, 2350, 35, 50, 75}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AY", {.duration = 0.26, .start = {820, 1220, 2550, 45, 45, 65}, .end = {340, 2200, 2900, 35, 55, 75}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"EY", {.duration = 0.24, .start = {620, 1950, 2650, 40, 45, 60}, .end = {340, 2250, 2950, 35, 55, 75}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"OW", {.duration = 0.24, .start = {620, 1050, 2450, 40, 45, 65}, .end = {400,  880, 2250, 35, 45, 60}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"AW", {.duration = 0.26, .start = {750, 1350, 2550, 40, 45, 65}, .end = {400,  950, 2250, 35, 45, 60}, .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"S",  {.duration = 0.15, .start = {4500, 5500, 6800, 400, 450, 600}, .end = {4200, 5200, 6500, 400, 450, 600}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"SH", {.duration = 0.17, .start = {2600, 3600, 4600, 300, 400, 500}, .end = {2300, 3300, 4300, 300, 400, 500}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"Z",  {.duration = 0.13, .start = {3600, 4600, 5600, 250, 350, 450}, .end = {3200, 4200, 5200, 250, 350, 450}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"F",  {.duration = 0.13, .start = {2100, 3100, 4100, 400, 500, 600}, .end = {1900, 2900, 3900, 400, 500, 600}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"V",  {.duration = 0.11, .start = {350,  1300, 2300, 70,  110, 180}, .end = {280,  1200, 2200, 70,  110, 180}, .is_fricative = true,  .is_plosive = false, .burst_duration = 0.0}},
    {"P",  {.duration = 0.09, .start = {240,  800, 1350, 50, 60, 85},  .end = {550, 1200, 2450, 40, 45, 60}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.015}},
    {"T",  {.duration = 0.09, .start = {450, 2200, 3200, 60, 75, 110}, .end = {550, 1500, 2550, 40, 45, 60}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.018}},
    {"K",  {.duration = 0.11, .start = {400, 1950, 2850, 50, 65, 100}, .end = {600, 1300, 2450, 40, 45, 60}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.022}},
    {"B",  {.duration = 0.11, .start = {200,  750, 1300, 40, 55, 80},  .end = {650, 1200, 2450, 40, 45, 60}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.010}},
    {"D",  {.duration = 0.11, .start = {280, 1750, 2650, 40, 55, 85},  .end = {550, 1500, 2550, 40, 45, 60}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.010}},
    {"G",  {.duration = 0.11, .start = {280, 1450, 2150, 40, 55, 80},  .end = {600, 1200, 2450, 40, 45, 60}, .is_fricative = false, .is_plosive = true,  .burst_duration = 0.010}},
    {"M",  {.duration = 0.12, .start = {280,  900, 2200, 35, 50, 90},  .end = {280,  900, 2200, 35, 50, 90},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"N",  {.duration = 0.12, .start = {280, 1650, 2300, 35, 50, 90},  .end = {280, 1650, 2300, 35, 50, 90},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"L",  {.duration = 0.12, .start = {340, 1100, 2600, 35, 50, 80},  .end = {440, 1400, 2500, 35, 50, 80},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"R",  {.duration = 0.13, .start = {340, 1000, 1500, 35, 45, 70},  .end = {440, 1300, 1700, 35, 45, 70},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"W",  {.duration = 0.10, .start = {310,  700, 2200, 35, 45, 80},  .end = {440, 1050, 2300, 35, 45, 80},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}},
    {"Y",  {.duration = 0.10, .start = {280, 2200, 3000, 35, 55, 85},  .end = {380, 1800, 2600, 35, 55, 85},  .is_fricative = false, .is_plosive = false, .burst_duration = 0.0}}
};

// Front-End Pronunciation Lexicon Matrix (Maps text tokens directly to internal phonemes)
std::map<std::string, std::vector<std::string>> PRONUNCIATION_LEXICON = {
    {"pet", {"P", "EH", "T"}},
    {"two", {"T", "UW"}},
    {"thousand", {"T", "AW", "S", "EH", "N", "D"}},
    {"one", {"W", "AH", "N"}},
    {"trs", {"T", "R", "S"}},
    {"eighty", {"EY", "T", "IY"}},
    {"apple", {"AE", "P", "L"}},
    {"nec", {"N", "EH", "K"}},
    {"pc", {"P", "IY", "S", "IY"}},
    {"eight", {"EY", "T"}},
    {"zero", {"Z", "IY", "R", "OW"}},
    {"model", {"M", "AA", "D", "EH", "L"}},
    {"hundred", {"H", "AH", "N", "D", "R", "EH", "D"}},
    {"sparcstation", {"S", "P", "AA", "R", "K", " ", "S", "T", "EY", "SH", "EH", "N"}},
    {"next", {"N", "EH", "K", "S", "T"}},
    {"computer", {"K", "AH", "M", "P", "Y", "UW", "T", "EH", "R"}},
    {"macintosh", {"M", "AE", "K", "EH", "N", "T", "AA", "SH"}},
    {"commodore", {"K", "AA", "M", "AH", "D", "OW", "R"}},
    {"sixty", {"S", "EH", "K", "S", "T", "IY"}},
    {"four", {"F", "AO", "R"}},
    {"executive", {"EH", "G", "Z", "EH", "K", "Y", "UW", "T", "EH", "V"}},
    {"hp", {"EH", "T", "S", " ", "P", "IY"}},
    {"nine", {"N", "AY", "N"}},
    {"five", {"F", "AY", "V"}},
    {"seven", {"S", "EH", "V", "EH", "N"}},
    {"three", {"T", "R", "IY"}}
};

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
        y2 = y1; y1 = output;
        return output;
    }
};

double get_glottal_pulse(double phase) {
    double normalized_phase = phase / (2.0 * PI);
    if (normalized_phase < 0.22) {
        return 3.0 * std::pow(normalized_phase / 0.22, 2) - 2.0 * std::pow(normalized_phase / 0.22, 3);
    } else if (normalized_phase < 0.30) {
        return 1.0 - std::pow((normalized_phase - 0.22) / 0.08, 2);
    }
    return 0.0; 
}

std::string clean_word(const std::string& input) {
    std::string result = "";
    for (char c : input) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
        }
    }
    return result;
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
        std::cout << "Usage: ./unified_tts \"<raw english sentence text>\"\n";
        std::cout << "Example: ./unified_tts \"commodore sixty four executive\"\n";
        return 1;
    }

    std::string raw_input_string = argv[1];
    std::vector<std::string> synthesized_phoneme_stream;

    // --- FRONT-END PARSER LOOP ---
    std::stringstream ss(raw_input_string);
    std::string current_raw_word;
    std::cout << "[*] Front-End Text Parsing:\n";

    while (ss >> current_raw_word) {
        std::string processed_word = clean_word(current_raw_word);
        if (processed_word.empty()) continue;

        if (PRONUNCIATION_LEXICON.contains(processed_word)) {
            std::cout << "  -> \"" << current_raw_word << "\" translated to lexicon string: ";
            for (const auto& phoneme : PRONUNCIATION_LEXICON[processed_word]) {
                std::cout << phoneme << " ";
                synthesized_phoneme_stream.push_back(phoneme);
            }
            std::cout << "\n";
            // Append a space phoneme marker for explicit word isolation gaps
            synthesized_phoneme_stream.push_back(" ");
        } else {
            std::cout << "  [-] Warning: Word \"" << current_raw_word << "\" not covered in internal lexicon map.\n";
        }
    }

    if (synthesized_phoneme_stream.empty()) {
        std::cout << "[-] Output generation halted: No recognizable vocabulary found.\n";
        return 1;
    }

    // --- BACK-END SYNTHESIS PATH ---
    std::string output_filename = "unified_output.wav";
    std::vector<short> pcm_data;
    double p0 = 0.0;
    unsigned int noise_seed = 54321;
    double noise_lpf_state = 0.0;

    Resonator R1, R2, R3;

    for (const std::string& tok : synthesized_phoneme_stream) {
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
            
            double f1 = ph.start.f1 + progress * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + progress * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + progress * (ph.end.f3 - ph.start.f3);
            
            double b1 = ph.start.b1 + progress * (ph.end.b1 - ph.start.b1);
            double b2 = ph.start.b2 + progress * (ph.end.b2 - ph.start.b2);
            double b3 = ph.start.b3 + progress * (ph.end.b3 - ph.start.b3);

            R1.set_parameters(f1, b1);
            R2.set_parameters(f2, b2);
            R3.set_parameters(f3, b3);

            p0 += (2.0 * PI * F0_BASE) / SAMPLE_RATE;
            if (p0 > 2.0 * PI) p0 -= 2.0 * PI;
            
            noise_seed = noise_seed * 1103515245 + 12345;
            double noise = (static_cast<double>(noise_seed % 2000) / 1000.0) - 1.0;
            noise_lpf_state = 0.45 * noise + 0.55 * noise_lpf_state; // De-esser

            double source_signal = 0.0;
            if (s < burst_samples) {
                source_signal = noise_lpf_state * (1.0 - (static_cast<double>(s) / burst_samples)) * 1.5;
            } else if (ph.is_fricative) {
                source_signal = noise_lpf_state * 0.70;
            } else {
                double raw_pulse = get_glottal_pulse(p0) * 1.6; 
                source_signal = raw_pulse - 0.15 * std::pow(raw_pulse, 3);
            }

            double filtered = R1.process(source_signal);
            filtered = R2.process(filtered);
            filtered = R3.process(filtered);

            double gain = ph.is_fricative ? 0.14 : 0.45;
            double sample_val = filtered * gain;

            if (s < 60) sample_val *= (static_cast<double>(s) / 60.0);
            if (s > total_samples - 150) sample_val *= (static_cast<double>(total_samples - s) / 150.0);

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

    std::cout << "[+] Audio pipeline output compiled to: " << output_filename << "\n";
    std::system(("afplay " + output_filename).c_str());
    return 0;
}

