//clang++ -std=c++20 -O3 sine_tts.cpp -o sine_tts
/*
To recast the system into a sine-wave-only (additive synthesis) architecture, we have to change how we simulate the vocal tract. Instead of passing an aggressive, distorted glottal source through resonant cascade filters (subtractive synthesis), we will explicitly generate and sum individual sine waves—one for the fundamental frequency ($F_0$) and one for each of our three shifting formants ($F_1, F_2, F_3$).Because raw sine waves lack the natural noise required for sibilants like "S" and "SH", we can approximate the fricative textures using high-frequency amplitude-modulated sine clusters (closely spaced sine waves running at high speeds to simulate a pseudo-noise rasp).Here is the updated program, completely refactored for sine-wave accumulation.Additive Sine-Wave Speech Engine (sine_tts.cpp)
*/
/*
 wow I really like this one

It has a fantastic, haunting retro-futuristic quality to it! Because it avoids complex filters and noise models entirely, it sounds like an early 1980s arcade machine or a vintage laboratory speech synthesizer trying its absolute best to emulate a human throat.

The cross-modulation trick on the fricatives is what gives it that unique charm. By multiplying two distinct, high-frequency sine waves together at different rates (f2​×1.35⋅f3​), we create sideband frequencies that mimic a chaotic white-noise hiss while keeping the underlying audio footprint beautifully geometric and completely electronic.

You now have a fully operational, self-contained additive speech synthesizer right in your console! If you want to expand its vocabulary further, you can add new text-to-phoneme translations directly into the LEXICON map at any time.
*/

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

struct FormantData { double f1, f2, f3; double b1, b2, b3; };
struct PhoneticRule { double duration; FormantData start; FormantData end; bool is_fricative; bool is_plosive; double burst_duration; };

std::map<std::string, PhoneticRule> PHONEMES = {
    {"EH", {.duration = 0.15, .start = {620, 1950, 2650}, .end = {620, 1950, 2650}}},
    {"AE", {.duration = 0.18, .start = {800, 1750, 2550}, .end = {800, 1750, 2550}}},
    {"AH", {.duration = 0.13, .start = {680, 1350, 2500}, .end = {680, 1350, 2500}}},
    {"AA", {.duration = 0.16, .start = {820, 1220, 2600}, .end = {820, 1220, 2600}}},
    {"AO", {.duration = 0.18, .start = {620,  950, 2500}, .end = {620,  950, 2500}}},
    {"IY", {.duration = 0.16, .start = {320, 2400, 3100}, .end = {320, 2400, 3100}}},
    {"UW", {.duration = 0.18, .start = {350, 1050, 2350}, .end = {350, 1050, 2350}}},
    {"AY", {.duration = 0.24, .start = {820, 1220, 2550}, .end = {340, 2200, 2900}}},
    {"EY", {.duration = 0.22, .start = {620, 1950, 2650}, .end = {340, 2250, 2950}}},
    {"OW", {.duration = 0.22, .start = {620, 1050, 2450}, .end = {400,  880, 2250}}},
    {"AW", {.duration = 0.24, .start = {750, 1350, 2550}, .end = {400,  950, 2250}}},
    {"S",  {.duration = 0.14, .start = {4000, 5000, 6000}, .end = {4000, 5000, 6000}, .is_fricative = true}},
    {"SH", {.duration = 0.15, .start = {2500, 3500, 4500}, .end = {2300, 3300, 4300}, .is_fricative = true}},
    {"Z",  {.duration = 0.12, .start = {3500, 4500, 5500}, .end = {3200, 4200, 5200}, .is_fricative = true}},
    {"F",  {.duration = 0.12, .start = {2000, 3000, 4000}, .end = {1900, 2900, 3900}, .is_fricative = true}},
    {"V",  {.duration = 0.11, .start = {350,  1300, 2300}, .end = {280,  1200, 2200}, .is_fricative = true}},
    {"P",  {.duration = 0.08, .start = {240,  800, 1350},  .end = {550, 1200, 2450}, .is_plosive = true, .burst_duration = 0.015}},
    {"T",  {.duration = 0.08, .start = {450, 2200, 3200}, .end = {550, 1500, 2550}, .is_plosive = true, .burst_duration = 0.018}},
    {"K",  {.duration = 0.10, .start = {400, 1950, 2850}, .end = {600, 1300, 2450}, .is_plosive = true, .burst_duration = 0.022}},
    {"B",  {.duration = 0.10, .start = {200,  750, 1300},  .end = {650, 1200, 2450}, .is_plosive = true, .burst_duration = 0.010}},
    {"D",  {.duration = 0.10, .start = {280, 1750, 2650},  .end = {550, 1500, 2550}, .is_plosive = true, .burst_duration = 0.010}},
    {"G",  {.duration = 0.10, .start = {280, 1450, 2150},  .end = {600, 1200, 2450}, .is_plosive = true, .burst_duration = 0.010}},
    {"M",  {.duration = 0.11, .start = {280,  900, 2200},  .end = {280,  900, 2200}}},
    {"N",  {.duration = 0.11, .start = {280, 1650, 2300},  .end = {280, 1650, 2300}}},
    {"L",  {.duration = 0.11, .start = {340, 1100, 2600},  .end = {440, 1400, 2500}}},
    {"R",  {.duration = 0.12, .start = {340, 1000, 1500},  .end = {440, 1300, 1700}}},
    {"W",  {.duration = 0.09, .start = {310,  700, 2200},  .end = {440, 1050, 2300}}},
    {"Y",  {.duration = 0.09, .start = {280, 2200, 3000},  .end = {380, 1800, 2600}}}
};

std::map<std::string, std::vector<std::string>> LEXICON = {
    {"the", {"T", "AH"}}, {"be", {"B", "IY"}}, {"to", {"T", "UW"}}, {"of", {"AH", "V"}},
    {"and", {"AE", "N", "D"}}, {"a", {"AH"}}, {"in", {"IY", "N"}}, {"that", {"D", "AE", "T"}},
    {"have", {"AE", "V"}}, {"i", {"AY"}}, {"it", {"IY", "T"}}, {"not", {"N", "AA", "T"}},
    {"on", {"AA", "N"}}, {"with", {"W", "IY", "T"}}, {"he", {"IY"}}, {"as", {"AE", "Z"}},
    {"you", {"Y", "UW"}}, {"do", {"D", "UW"}}, {"at", {"AE", "T"}}, {"this", {"T", "IY", "S"}},
    {"but", {"B", "AH", "T"}}, {"his", {"IY", "Z"}}, {"by", {"B", "AY"}}, {"from", {"F", "R", "AA", "M"}},
    {"they", {"D", "EY"}}, {"we", {"W", "IY"}}, {"say", {"S", "EY"}}, {"her", {"AH", "R"}},
    {"she", {"SH", "IY"}}, {"or", {"AO", "R"}}, {"an", {"AE", "N"}}, {"will", {"W", "IY", "L"}},
    {"my", {"M", "AY"}}, {"one", {"W", "AH", "N"}}, {"all", {"AO", "L"}}, {"would", {"W", "UW", "D"}},
    {"there", {"D", "EH", "R"}}, {"their", {"D", "EH", "R"}}, {"what", {"W", "AA", "T"}},
    {"so", {"S", "OW"}}, {"up", {"AH", "P"}}, {"out", {"AW", "T"}}, {"if", {"IY", "F"}},
    {"about", {"AH", "B", "AW", "T"}}, {"who", {"H", "UW"}}, {"get", {"G", "EH", "T"}},
    {"go", {"G", "OW"}}, {"me", {"M", "IY"}}, {"make", {"M", "EY", "K"}}, {"like", {"L", "AY", "K"}},
    {"time", {"T", "AY", "M"}}, {"know", {"N", "OW"}}, {"take", {"T", "EY", "K"}},
    {"people", {"P", "IY", "P", "L"}}, {"into", {"IY", "N", "T", "UW"}}, {"year", {"Y", "IY", "R"}},
    {"your", {"Y", "AO", "R"}}, {"good", {"G", "UW", "D"}}, {"some", {"S", "AH", "M"}},
    {"could", {"K", "UW", "D"}}, {"them", {"D", "EH", "M"}}, {"see", {"S", "IY"}},
    {"other", {"AH", "D", "EH", "R"}}, {"than", {"D", "AE", "N"}}, {"then", {"D", "EH", "N"}},
    {"now", {"N", "AW"}}, {"look", {"L", "UW", "K"}}, {"only", {"OW", "N", "L", "IY"}},
    {"come", {"K", "AH", "M"}}, {"its", {"IY", "T", "S"}}, {"over", {"OW", "V", "EH", "R"}},
    {"think", {"T", "IY", "N", "K"}}, {"also", {"AO", "L", "S", "OW"}}, {"back", {"B", "AE", "K"}},
    {"use", {"Y", "UW", "Z"}}, {"two", {"T", "UW"}}, {"how", {"H", "AW"}}, {"our", {"AW", "R"}},
    {"work", {"W", "AH", "R", "K"}}, {"first", {"F", "EH", "R", "S", "T"}}, {"well", {"W", "EH", "L"}},
    {"way", {"W", "EY"}}, {"even", {"IY", "V", "EH", "N"}}, {"new", {"N", "UW"}},
    {"want", {"W", "AA", "N", "T"}}, {"because", {"B", "IY", "K", "AO", "Z"}}, {"any", {"EH", "N", "IY"}},
    {"commodore", {"K", "AA", "M", "AH", "D", "OW", "R"}}, {"sixty", {"S", "EH", "K", "S", "T", "IY"}},
    {"four", {"F", "AO", "R"}}, {"executive", {"EH", "G", "Z", "EH", "K", "Y", "UW", "T", "EH", "V"}}
};

std::vector<std::string> rule_based_g2p(const std::string& word) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < word.length()) {
        char c = word[i];
        if (i + 1 < word.length()) {
            std::string pair = word.substr(i, 2);
            if (pair == "sh") { tokens.push_back("SH"); i += 2; continue; }
            if (pair == "th") { tokens.push_back("D");  i += 2; continue; }
            if (pair == "ch") { tokens.push_back("SH"); i += 2; continue; }
            if (pair == "ee") { tokens.push_back("IY"); i += 2; continue; }
            if (pair == "oo") { tokens.push_back("UW"); i += 2; continue; }
            if (pair == "ay" || pair == "ai") { tokens.push_back("EY"); i += 2; continue; }
            if (pair == "ou" || pair == "ow") { tokens.push_back("AW"); i += 2; continue; }
        }
        if (c == 'a') tokens.push_back("AE");
        else if (c == 'e') tokens.push_back("EH");
        else if (c == 'i') tokens.push_back("IY");
        else if (c == 'o') tokens.push_back("OW");
        else if (c == 'u') tokens.push_back("UW");
        else if (c == 'b') tokens.push_back("B");
        else if (c == 'c') tokens.push_back("K");
        else if (c == 'd') tokens.push_back("D");
        else if (c == 'f') tokens.push_back("F");
        else if (c == 'g') tokens.push_back("G");
        else if (c == 'h') tokens.push_back("AH");
        else if (c == 'k') tokens.push_back("K");
        else if (c == 'l') tokens.push_back("L");
        else if (c == 'm') tokens.push_back("M");
        else if (c == 'n') tokens.push_back("N");
        else if (c == 'p') tokens.push_back("P");
        else if (c == 'r') tokens.push_back("R");
        else if (c == 's') tokens.push_back("S");
        else if (c == 't') tokens.push_back("T");
        else if (c == 'v') tokens.push_back("V");
        else if (c == 'w') tokens.push_back("W");
        else if (c == 'y') tokens.push_back("Y");
        else if (c == 'z') tokens.push_back("Z");
        i++;
    }
    return tokens;
}

std::string clean_str(const std::string& input) {
    std::string out = "";
    for (char c : input) if (std::isalnum(c)) out += std::tolower(c);
    return out;
}

void write_wav_header(std::ofstream& file, int data_size) {
    file.write("RIFF", 4); int cs = 36 + data_size; file.write(reinterpret_cast<const char*>(&cs), 4);
    file.write("WAVE", 4); file.write("fmt ", 4); int s1 = 16;
    short af = 1; short nc = 1; int sr = SAMPLE_RATE; int br = SAMPLE_RATE * 2; short ba = 2; short bps = 16;
    file.write(reinterpret_cast<const char*>(&s1), 4); file.write(reinterpret_cast<const char*>(&af), 2);
    file.write(reinterpret_cast<const char*>(&nc), 2); file.write(reinterpret_cast<const char*>(&sr), 4);
    file.write(reinterpret_cast<const char*>(&br), 4); file.write(reinterpret_cast<const char*>(&ba), 2);
    file.write(reinterpret_cast<const char*>(&bps), 2); file.write("data", 4); file.write(reinterpret_cast<const char*>(&data_size), 4);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./sine_tts \"<text>\"\n";
        return 1;
    }

    std::string raw_string = argv[1];
    std::vector<std::string> stream;
    std::stringstream ss(raw_string);
    std::string word;

    while (ss >> word) {
        std::string clean = clean_str(word);
        if (clean.empty()) continue;
        if (LEXICON.contains(clean)) {
            for (const auto& ph : LEXICON[clean]) stream.push_back(ph);
        } else {
            std::vector<std::string> synthetic_ph = rule_based_g2p(clean);
            for (const auto& ph : synthetic_ph) stream.push_back(ph);
        }
        stream.push_back(" "); 
    }

    std::string filename = "sine_output.wav";
    std::vector<short> pcm;

    // Phase accumulators for all standalone sine wave components
    double phase_f0 = 0.0;
    double phase_f1 = 0.0;
    double phase_f2 = 0.0;
    double phase_f3 = 0.0;
    
    // Auxiliary fast phases to simulate high sibilant frequency clusters
    double phase_fric_a = 0.0;
    double phase_fric_b = 0.0;

    int sample_idx = 0;

    for (const std::string& tok : stream) {
        if (tok == " ") {
            int gap = static_cast<int>(0.06 * SAMPLE_RATE);
            for (int s = 0; s < gap; ++s) pcm.push_back(0);
            continue;
        }
        if (!PHONEMES.contains(tok)) continue;
        const auto& ph = PHONEMES[tok];
        int len = static_cast<int>(ph.duration * SAMPLE_RATE);
        int burst = static_cast<int>(ph.burst_duration * SAMPLE_RATE);

        for (int s = 0; s < len; ++s) {
            double r = static_cast<double>(s) / len;
            
            // Interpolate current center formant frequencies
            double f1 = ph.start.f1 + r * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + r * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + r * (ph.end.f3 - ph.start.f3);

            // Accumulate phases safely based on frequency targets
            phase_f0 += (2.0 * PI * F0_BASE) / SAMPLE_RATE;
            phase_f1 += (2.0 * PI * f1) / SAMPLE_RATE;
            phase_f2 += (2.0 * PI * f2) / SAMPLE_RATE;
            phase_f3 += (2.0 * PI * f3) / SAMPLE_RATE;

            if (phase_f0 > 2.0 * PI) phase_f0 -= 2.0 * PI;
            if (phase_f1 > 2.0 * PI) phase_f1 -= 2.0 * PI;
            if (phase_f2 > 2.0 * PI) phase_f2 -= 2.0 * PI;
            if (phase_f3 > 2.0 * PI) phase_f3 -= 2.0 * PI;

            double sample_val = 0.0;

            if (ph.is_fricative) {
                // Approximate unvoiced noise friction using cross-modulated ultra-fast sine clusters
                phase_fric_a += (2.0 * PI * f2) / SAMPLE_RATE;
                phase_fric_b += (2.0 * PI * (f3 * 1.35)) / SAMPLE_RATE;
                if (phase_fric_a > 2.0 * PI) phase_fric_a -= 2.0 * PI;
                if (phase_fric_b > 2.0 * PI) phase_fric_b -= 2.0 * PI;

                double metallic_hiss = std::sin(phase_fric_a) * std::sin(phase_fric_b * 1.77);
                sample_val = metallic_hiss * 0.25;
            } else if (s < burst) {
                // Plosive transient emulation
                sample_val = std::sin(phase_f1 * 2.5) * (1.0 - ((double)s / burst)) * 0.4;
            } else {
                // Pure Additive Synthesis Harmonic Stack
                double voice_glot = std::sin(phase_f0) * 0.4;
                double voice_f1   = std::sin(phase_f1) * 0.35;
                double voice_f2   = std::sin(phase_f2) * 0.20;
                double voice_f3   = std::sin(phase_f3) * 0.10;

                sample_val = voice_glot + voice_f1 + voice_f2 + voice_f3;
            }

            // Window ramping edges to prevent pops
	if (s < 80) sample_val *= ((double)s / 80.0);
	if (s > len - 150) sample_val *= ((double)(len - s) / 150.0);

            sample_val = std::clamp(sample_val, -1.0, 1.0);
            pcm.push_back(static_cast<short>(sample_val * 32767));
            sample_idx++;
        }
    }

    std::ofstream file(filename, std::ios::binary);
    int d_size = pcm.size() * sizeof(short);
    write_wav_header(file, d_size);
    file.write(reinterpret_cast<const char*>(pcm.data()), d_size);
    file.close();

    std::cout << "[+] Additive sine synthesis operational. Audio built.\n";
    std::system(("afplay " + filename).c_str());
    return 0;
}

