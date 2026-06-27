//clang++ -std=c++20 -O3 filter_sweep_tts.cpp -o filter_tts
// ./filter_tts --filter-singing "quick execution of the jazz project"
// ./filter_tts --filter-glitch "quick execution of the jazz project"
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
const double F0_STATIC = 135.0; 

enum EngineMode { 
    MODE_HYBRID, MODE_SINE, MODE_ALTERNATE, MODE_SINGING, MODE_ALTERNATE_SINGING, 
    MODE_GLITCH_SINGING, MODE_ALTERNATE_GLITCH,
    MODE_FILTER_SINE, MODE_FILTER_SINGING, MODE_FILTER_GLITCH 
};

struct FormantData { double f1, f2, f3; double b1, b2, b3; };
struct PhoneticRule { double duration; FormantData start; FormantData end; bool is_fricative; bool is_plosive; double burst_duration; };

std::map<std::string, PhoneticRule> PHONEMES = {
    {"EH", {.duration = 0.15, .start = {620, 1950, 2650, 40, 45, 60}, .end = {620, 1950, 2650, 40, 45, 60}}},
    {"AE", {.duration = 0.18, .start = {800, 1750, 2550, 45, 45, 65}, .end = {800, 1750, 2550, 45, 45, 65}}},
    {"AH", {.duration = 0.13, .start = {680, 1350, 2500, 40, 45, 60}, .end = {680, 1350, 2500, 40, 45, 60}}},
    {"AA", {.duration = 0.16, .start = {820, 1220, 2600, 45, 45, 65}, .end = {820, 1220, 2600, 45, 45, 65}}},
    {"AO", {.duration = 0.18, .start = {620,  950, 2500, 40, 45, 60}, .end = {620,  950, 2500, 40, 45, 60}}},
    {"IY", {.duration = 0.16, .start = {320, 2400, 3100, 35, 55, 75}, .end = {320, 2400, 3100, 35, 55, 75}}},
    {"UW", {.duration = 0.18, .start = {350, 1050, 2350, 35, 50, 75}, .end = {350, 1050, 2350, 35, 50, 75}}},
    {"AY", {.duration = 0.24, .start = {820, 1220, 2550, 45, 45, 65}, .end = {340, 2200, 2900, 35, 55, 75}}},
    {"EY", {.duration = 0.22, .start = {620, 1950, 2650, 40, 45, 60}, .end = {340, 2250, 2950, 35, 55, 75}}},
    {"OW", {.duration = 0.22, .start = {620, 1050, 2450, 40, 45, 65}, .end = {400,  880, 2250, 35, 45, 60}}},
    {"AW", {.duration = 0.24, .start = {750, 1350, 2550, 40, 45, 65}, .end = {400,  950, 2250, 35, 45, 60}}},
    {"S",  {.duration = 0.14, .start = {4200, 5200, 6500, 400, 450, 600}, .end = {4000, 5000, 6000, 400, 450, 600}, .is_fricative = true}},
    {"SH", {.duration = 0.15, .start = {2600, 3600, 4600, 300, 400, 500}, .end = {2300, 3300, 4300, 300, 400, 500}, .is_fricative = true}},
    {"J",  {.duration = 0.16, .start = {300,  2300, 3800, 60,  150, 200}, .end = {450,  1800, 2800, 40,  80,  120}, .is_fricative = true, .is_plosive = true, .burst_duration = 0.025}},
    {"Z",  {.duration = 0.12, .start = {3600, 4600, 5600, 250, 350, 450}, .end = {3200, 4200, 5200, 250, 350, 450}, .is_fricative = true}},
    {"F",  {.duration = 0.12, .start = {2100, 3100, 4100, 400, 500, 600}, .end = {1900, 2900, 3900, 400, 500, 600}, .is_fricative = true}},
    {"V",  {.duration = 0.11, .start = {350,  1300, 2300, 70,  110, 180}, .end = {280,  1200, 2200, 70,  110, 180}, .is_fricative = true}},
    {"P",  {.duration = 0.08, .start = {240,  800, 1350, 50, 60, 85},  .end = {550, 1200, 2450, 40, 45, 60}, .is_plosive = true, .burst_duration = 0.015}},
    {"T",  {.duration = 0.08, .start = {450, 2200, 3200, 60, 75, 110}, .end = {550, 1500, 2550, 40, 45, 60}, .is_plosive = true, .burst_duration = 0.018}},
    {"K",  {.duration = 0.10, .start = {400, 1950, 2850, 50, 65, 100}, .end = {600, 1300, 2450, 40, 45, 60}, .is_plosive = true, .burst_duration = 0.022}},
    {"B",  {.duration = 0.10, .start = {200,  750, 1300, 40, 55, 80},  .end = {650, 1200, 2450, 40, 45, 60}, .is_plosive = true, .burst_duration = 0.010}},
    {"D",  {.duration = 0.10, .start = {280, 1750, 2650, 40, 55, 85},  .end = {550, 1500, 2550, 40, 45, 60}, .is_plosive = true, .burst_duration = 0.010}},
    {"G",  {.duration = 0.10, .start = {280, 1450, 2150, 40, 55, 80},  .end = {600, 1200, 2450, 40, 45, 60}, .is_plosive = true, .burst_duration = 0.010}},
    {"M",  {.duration = 0.11, .start = {280,  900, 2200, 35, 50, 90},  .end = {280,  900, 2200, 35, 50, 90}}},
    {"N",  {.duration = 0.11, .start = {280, 1650, 2300, 35, 50, 90},  .end = {280, 1650, 2300, 35, 50, 90}}},
    {"L",  {.duration = 0.11, .start = {340, 1100, 2600, 35, 50, 80},  .end = {440, 1400, 2500, 35, 50, 80}}},
    {"R",  {.duration = 0.12, .start = {340, 1000, 1500, 35, 45, 70},  .end = {440, 1300, 1700, 35, 45, 70}}},
    {"W",  {.duration = 0.09, .start = {310,  700, 2200, 35, 45, 80},  .end = {440, 1050, 2300, 35, 45, 80}}},
    {"Y",  {.duration = 0.09, .start = {280, 2200, 3000, 35, 55, 85},  .end = {380, 1800, 2600, 35, 55, 85}}}
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
    {"four", {"F", "AO", "R"}}, {"jazz", {"J", "AE", "Z"}}, {"just", {"J", "AH", "S", "T"}}, 
    {"project", {"P", "R", "AA", "J", "EH", "K", "T"}}, {"executive", {"EH", "G", "Z", "EH", "K", "Y", "UW", "T", "EH", "V"}},
    {"quick", {"K", "W", "IY", "K"}}, {"six", {"S", "IY", "K", "S"}}
};

struct ScheduledPhoneme {
    std::string token;
    bool use_sine_engine; 
    bool is_singing_word; 
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
        if (c == 'q') { tokens.push_back("K"); tokens.push_back("W"); }
        else if (c == 'x') { tokens.push_back("K"); tokens.push_back("S"); }
        else if (c == 'j') tokens.push_back("J");
        else if (c == 'a') tokens.push_back("AE");
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

class Resonator {
private:
    double a, b, c, y1, y2;
public:
    Resonator() : a(0), b(0), c(0), y1(0), y2(0) {}
    void set_parameters(double freq, double bw) {
        double r = std::exp(-PI * bw / SAMPLE_RATE);
        c = -r * r; b = 2.0 * r * std::cos(2.0 * PI * freq / SAMPLE_RATE); a = 1.0 - b - c;
    }
    double process(double in) {
        double out = a * in + b * y1 + c * y2;
        y2 = y1; y1 = out; return out;
    }
};

// Resonant State Variable Filter (SVF) for dynamic sweeping morphs
class StateVariableFilter {
private:
    double lowState, bandState;
public:
    StateVariableFilter() : lowState(0), bandState(0) {}
    
    // Processes the input sample and extracts standard LP and HP bands simultaneously
    void process(double input, double cutoffHz, double resonance, double& lpOut, double& hpOut) {
        // Safe parameter transformation constraints
        double f = 2.0 * std::sin(PI * cutoffHz / SAMPLE_RATE);
        f = std::clamp(f, 0.0, 1.0);
        double q = 1.0 / resonance;

        hpOut = input - lowState - q * bandState;
        bandState += f * hpOut;
        lpOut = lowState + f * bandState;
        lowState = lpOut;
    }
};

double get_glottal_pulse(double phase) {
    double np = phase / (2.0 * PI);
    if (np < 0.22) return 3.0 * std::pow(np / 0.22, 2) - 2.0 * std::pow(np / 0.22, 3);
    else if (np < 0.30) return 1.0 - std::pow((np - 0.22) / 0.08, 2);
    return 0.0; 
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
        std::cout << "Usage: ./filter_tts [--filter-sine | --filter-singing | --filter-glitch] \"<phrase>\"\n";
        return 1;
    }

    EngineMode selected_mode = MODE_FILTER_SINE;
    std::string raw_string = "";

    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--hybrid") selected_mode = MODE_HYBRID;
        else if (arg == "--sine") selected_mode = MODE_SINE;
        else if (arg == "--singing") selected_mode = MODE_SINGING;
        else if (arg == "--alternate") selected_mode = MODE_ALTERNATE;
        else if (arg == "--alternate-glitch") selected_mode = MODE_ALTERNATE_GLITCH;
        else if (arg == "--filter-sine") selected_mode = MODE_FILTER_SINE;
        else if (arg == "--filter-singing") selected_mode = MODE_FILTER_SINGING;
        else if (arg == "--filter-glitch") selected_mode = MODE_FILTER_GLITCH;
        else if (raw_string.empty()) raw_string = arg;
    }

    if (raw_string.empty()) {
        std::cout << "[-] Missing string content literal.\n";
        return 1;
    }

    std::vector<ScheduledPhoneme> master_stream;
    std::stringstream ss(raw_string);
    std::string word;

    while (ss >> word) {
        std::string clean = clean_str(word);
        if (clean.empty()) continue;

        bool target_to_sine = false;
        bool is_singing = false;

        if (selected_mode == MODE_SINE || selected_mode == MODE_FILTER_SINE) {
            target_to_sine = true;
        } else if (selected_mode == MODE_SINGING || selected_mode == MODE_FILTER_SINGING || 
                   selected_mode == MODE_GLITCH_SINGING || selected_mode == MODE_FILTER_GLITCH) {
            is_singing = true;
        }

        std::vector<std::string> targeted_phonemes = LEXICON.contains(clean) ? LEXICON[clean] : rule_based_g2p(clean);
        for (const auto& ph : targeted_phonemes) {
            master_stream.push_back({ph, target_to_sine, is_singing});
        }
        master_stream.push_back({" ", target_to_sine, is_singing}); 
    }

    std::string filename = "filter_sweep_output.wav";
    std::vector<short> pcm;

    double phase_f0 = 0.0, phase_f1 = 0.0, phase_f2 = 0.0, phase_f3 = 0.0;
    double phase_fric_a = 0.0, phase_fric_b = 0.0;
    double hybrid_p0 = 0.0, noise_lpf_state = 0.0;
    unsigned int noise_seed = 54321;
    
    Resonator R1, R2, R3;
    StateVariableFilter morphFilter;

    long long global_sample_index = 0;

    for (const auto& scheduled : master_stream) {
        if (scheduled.token == " ") {
            int gap = static_cast<int>(0.06 * SAMPLE_RATE);
            for (int s = 0; s < gap; ++s) { pcm.push_back(0); global_sample_index++; }
            continue;
        }
        
        if (!PHONEMES.contains(scheduled.token)) continue;
        const auto& ph = PHONEMES[scheduled.token];
        int len = static_cast<int>(ph.duration * SAMPLE_RATE);
        int burst = static_cast<int>(ph.burst_duration * SAMPLE_RATE);

        for (int s = 0; s < len; ++s) {
            double r = static_cast<double>(s) / len;
            double f1 = ph.start.f1 + r * (ph.end.f1 - ph.start.f1);
            double f2 = ph.start.f2 + r * (ph.end.f2 - ph.start.f2);
            double f3 = ph.start.f3 + r * (ph.end.f3 - ph.start.f3);

            double current_f0 = F0_STATIC;
            double currentTime = (double)global_sample_index / SAMPLE_RATE;

            if (scheduled.is_singing_word) {
                double base_melody = 165.0 + 45.0 * std::sin(2.0 * PI * 0.85 * currentTime);
                double vibrato = 8.0 * std::sin(2.0 * PI * 6.2 * currentTime);
                current_f0 = base_melody + vibrato;

                if (selected_mode == MODE_FILTER_GLITCH) {
                    double glitch_window = std::sin(2.0 * PI * 0.4 * currentTime);
                    if (glitch_window > 0.3) {
                        current_f0 += std::tan(2.0 * PI * 18.0 * currentTime) * 85.0;
                    }
                }
            }

            if (std::isnan(current_f0) || std::isinf(current_f0) || current_f0 < 10.0) current_f0 = 40.0; 
            else if (current_f0 > 4500.0) current_f0 = 4500.0;

            double raw_sample = 0.0;

            if (scheduled.use_sine_engine) {
                phase_f0 += (2.0 * PI * current_f0) / SAMPLE_RATE;
                phase_f1 += (2.0 * PI * f1) / SAMPLE_RATE;
                phase_f2 += (2.0 * PI * f2) / SAMPLE_RATE;
                phase_f3 += (2.0 * PI * f3) / SAMPLE_RATE;

                if (phase_f0 > 2.0 * PI) phase_f0 -= 2.0 * PI;
                if (phase_f1 > 2.0 * PI) phase_f1 -= 2.0 * PI;
                if (phase_f2 > 2.0 * PI) phase_f2 -= 2.0 * PI;
                if (phase_f3 > 2.0 * PI) phase_f3 -= 2.0 * PI;

                if (ph.is_plosive && s < burst) {
                    raw_sample = std::sin(phase_f1 * 2.8) * (1.0 - ((double)s / burst)) * 0.55;
                } else if (ph.is_fricative) {
                    phase_fric_a += (2.0 * PI * f2) / SAMPLE_RATE;
                    phase_fric_b += (2.0 * PI * (f3 * 1.25)) / SAMPLE_RATE;
                    if (phase_fric_a > 2.0 * PI) phase_fric_a -= 2.0 * PI;
                    if (phase_fric_b > 2.0 * PI) phase_fric_b -= 2.0 * PI;
                    raw_sample = std::sin(phase_fric_a) * std::sin(phase_fric_b * 1.5) * 0.22;
                } else {
                    raw_sample = (std::sin(phase_f0) * 0.4) + (std::sin(phase_f1) * 0.35) + 
                                 (std::sin(phase_f2) * 0.20) + (std::sin(phase_f3) * 0.10);
                }
                raw_sample *= 0.85;
            } else {
                double b1 = ph.start.b1 + r * (ph.end.b1 - ph.start.b1);
                double b2 = ph.start.b2 + r * (ph.end.b2 - ph.start.b2);
                double b3 = ph.start.b3 + r * (ph.end.b3 - ph.start.b3);

                R1.set_parameters(f1, b1); R2.set_parameters(f2, b2); R3.set_parameters(f3, b3);

                hybrid_p0 += (2.0 * PI * current_f0) / SAMPLE_RATE;
                if (hybrid_p0 > 2.0 * PI) hybrid_p0 -= 2.0 * PI;
                
                noise_seed = noise_seed * 1103515245 + 12345;
                double n = ((double)(noise_seed % 2000) / 1000.0) - 1.0;
                noise_lpf_state = 0.45 * n + 0.55 * noise_lpf_state;

                double src = 0.0;
                if (ph.is_plosive && ph.is_fricative) {
                    if (s < burst) src = noise_lpf_state * (1.2 - ((double)s / burst)) * 1.8;
                    else src = noise_lpf_state * 0.65;
                } else if (s < burst) {
                    src = noise_lpf_state * (1.0 - ((double)s / burst)) * 1.5;
                } else if (ph.is_fricative) {
                    src = noise_lpf_state * 0.70;
                } else {
                    double p = get_glottal_pulse(hybrid_p0) * 1.6; src = p - 0.15 * std::pow(p, 3);
                }

                raw_sample = R3.process(R2.process(R1.process(src))) * (ph.is_fricative ? 0.16 : 0.45);
            }

            // --- Real-time LFO Filter Morph Processing Engine ---
            // Triangle wave oscillator tracking between 0.0 and 1.0 at 0.5 Hz
            double filterLfo = 2.0 * std::abs(2.0 * ((currentTime * 0.5) - std::floor((currentTime * 0.5) + 0.5)));
            
            // Map the LFO path to a cutoff frequency sweep between 250Hz and 3500Hz
            double targetCutoff = 250.0 + filterLfo * 3250.0;
            double resonance = 2.2; // Pronounced accentuation at cutoff center

            double lpSample = 0.0, hpSample = 0.0;
            morphFilter.process(raw_sample, targetCutoff, resonance, lpSample, hpSample);

            // Crossfade mix based on the LFO state: 
            // When cutoff is low -> extract the warm Low-Pass signal
            // When cutoff sweeps high -> blend toward the glassy High-Pass signal
            double processed_sample = (lpSample * (1.0 - filterLfo)) + (hpSample * filterLfo);

            if (s < 80) processed_sample *= ((double)s / 80.0);
            if (s > len - 150) processed_sample *= ((double)(len - s) / 150.0);

            processed_sample = std::clamp(processed_sample, -1.0, 1.0);
            pcm.push_back(static_cast<short>(processed_sample * 32767));
            global_sample_index++;
        }
    }

    std::ofstream file(filename, std::ios::binary);
    int d_size = pcm.size() * sizeof(short);
    write_wav_header(file, d_size);
    file.write(reinterpret_cast<const char*>(pcm.data()), d_size);
    file.close();

    std::cout << "[+] Dynamic SVF Sweep complete.\n";
    std::system(("afplay " + filename).c_str());
    return 0;
}
