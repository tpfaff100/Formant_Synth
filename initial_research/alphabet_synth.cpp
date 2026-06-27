//clang++ -O3 -std=c++20 alphabet_synth.cpp -o alphabet_synth

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

// Structure to define dynamic math rules for a letter
struct LetterFormula {
    double duration; // Total time in seconds
    // Structural target frequencies for up to 3 formants
    double f1_start, f1_end;
    double f2_start, f2_end;
    double f3_start, f3_end;
    // Master amplitudes
    double a1, a2, a3;
    bool is_fricative; // If true, shifts frequencies to simulate noise friction
};

std::map<char, LetterFormula> ALPHABET;

void initialize_alphabet_formulas() {
    // Vowels (Static Formants - Start and End frequencies match)
    ALPHABET['a'] = {0.6,  730,  730, 1090, 1090, 2440, 2440,  0.6, 0.5, 0.3, false};
    ALPHABET['e'] = {0.6,  530,  530, 1840, 1840, 2480, 2480,  0.6, 0.5, 0.3, false};
    ALPHABET['i'] = {0.6,  270,  270, 2290, 2290, 3010, 3010,  0.6, 0.5, 0.3, false};
    ALPHABET['o'] = {0.6,  570,  570,  840,  840, 2410, 2410,  0.6, 0.5, 0.3, false};
    ALPHABET['u'] = {0.6,  300,  300,  870,  870, 2240, 2240,  0.6, 0.4, 0.2, false};

    // Plosives / Stops (Pronounced as Letter + "eh" or "ee". Feature rapid sweeping transitions)
    // 'b' -> Burst sweep into a neutral vowel
    ALPHABET['b'] = {0.4,  150,  500,  600, 1500, 1800, 2500,  0.6, 0.4, 0.2, false};
    // 'c' (si) -> High-frequency friction fading into 'i'
    ALPHABET['c'] = {0.5, 4000,  270, 5000, 2290, 6000, 3010,  0.4, 0.5, 0.3, true};
    // 'd' -> Sharp downward/forward sweep
    ALPHABET['d'] = {0.4,  350,  500, 1800, 1500, 2700, 2500,  0.6, 0.5, 0.3, false};
    // 'g' -> Mid-frequency convergence
    ALPHABET['g'] = {0.4,  300,  500, 1500, 1500, 2000, 2500,  0.6, 0.4, 0.2, false};
    // 'j' -> Complex glide sweep
    ALPHABET['j'] = {0.5,  250,  550, 2000, 1600, 2800, 2400,  0.5, 0.5, 0.3, false};
    // 'k' -> High burst sweep
    ALPHABET['k'] = {0.4,  400,  500, 1800, 1500, 2600, 2500,  0.5, 0.4, 0.2, false};
    // 'p' -> Low burst sweep
    ALPHABET['p'] = {0.4,  200,  500,  700, 1500, 2000, 2500,  0.5, 0.3, 0.1, false};
    // 'q' -> Sweeps low and rounds out
    ALPHABET['q'] = {0.5,  350,  300, 1500,  900, 2500, 2200,  0.5, 0.4, 0.2, false};
    // 't' -> High sharp burst
    ALPHABET['t'] = {0.4,  400,  500, 2000, 1500, 3000, 2500,  0.5, 0.4, 0.3, false};
    // 'v' -> Low friction blending into 'ee'
    ALPHABET['v'] = {0.5,  200,  270, 1000, 2290, 2000, 3010,  0.5, 0.4, 0.2, false};
    // 'x' -> 'e' followed by a high frequency friction burst
    ALPHABET['x'] = {0.6,  530, 4500, 1840, 5500, 2480, 6500,  0.5, 0.5, 0.3, true};
    // 'z' -> Intense friction sweep into 'ee'
    ALPHABET['z'] = {0.5, 3000,  270, 4000, 2290, 5000, 3010,  0.5, 0.4, 0.3, true};

    // Nasals & Liquids & Glides (Smooth continuous adjustments)
    // 'f' -> Vowel into a friction trailing tail
    ALPHABET['f'] = {0.5,  530, 3000, 1840, 4000, 2480, 5000,  0.5, 0.3, 0.2, true};
    // 'h' -> Soft friction opening up to 'e'
    ALPHABET['h'] = {0.5, 1500,  530, 2500, 1840, 3500, 2480,  0.3, 0.4, 0.3, true};
    // 'l' -> Heavy upward liquid move
    ALPHABET['l'] = {0.5,  310,  500, 1050, 1500, 2600, 2500,  0.6, 0.5, 0.3, false};
    // 'm' -> Closed nasal into flat open frame
    ALPHABET['m'] = {0.5,  250,  600, 1000, 1200, 2500, 2400,  0.6, 0.3, 0.1, false};
    // 'n' -> Teeth-closed nasal sweep
    ALPHABET['n'] = {0.5,  250,  600, 1700, 1400, 2500, 2400,  0.6, 0.3, 0.1, false};
    // 'r' -> Extreme low F3 signature drop
    ALPHABET['r'] = {0.5,  400,  530, 1200, 1840, 1500, 2480,  0.5, 0.4, 0.3, false};
    // 's' -> Pure sibilant high friction profile
    ALPHABET['s'] = {0.5, 4500, 5000, 5500, 6000, 6500, 7000,  0.5, 0.5, 0.4, true};
    // 'w' -> Deep back-vowel transition upwards
    ALPHABET['w'] = {0.5,  300,  530,  700, 1840, 2200, 2480,  0.6, 0.4, 0.2, false};
    // 'y' -> High front glide transition
    ALPHABET['y'] = {0.5,  270,  600, 2290, 1200, 3010, 2400,  0.6, 0.5, 0.3, false};
}

void write_wav_header(std::ofstream& file, int data_size) {
    file.write("RIFF", 4);
    int chunk_size = 36 + data_size;
    file.write(reinterpret_cast<const char*>(&chunk_size), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    int subchunk1_size = 16;
    short audio_format = 1; 
    short num_channels = 1; 
    int sample_rate = SAMPLE_RATE;
    int byte_rate = SAMPLE_RATE * 1 * 2; 
    short block_align = 2; 
    short bits_per_sample = 16;
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
        std::cout << "Usage: " << argv[0] << " <string_of_letters> [-o output.wav]\n";
        std::cout << "Example: " << argv[0] << " hello\n";
        return 1;
    }

    initialize_alphabet_formulas();

    std::string input_text = argv[1];
    std::string output_filename = "alphabet.wav";

    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "-o" && i + 1 < argc) {
            output_filename = argv[++i];
        }
    }

    // Process characters to lowercase
    std::transform(input_text.begin(), input_text.end(), input_text.begin(), ::tolower);
    std::vector<short> pcm_data;

    std::cout << "[*] Dynamically morphing sine equations across input string...\n";

    // Keep phases running continuously between letters to eliminate audio pops
    double phase1 = 0.0, phase2 = 0.0, phase3 = 0.0;

    for (size_t i = 0; i < input_text.length(); ++i) {
        char c = input_text[i];
        
        // Handle spacing / pauses
        if (c == ' ') {
            int space_samples = static_cast<int>(0.2 * SAMPLE_RATE);
            for (int s = 0; s < space_samples; ++s) pcm_data.push_back(0);
            continue;
        }

        if (ALPHABET.find(c) == ALPHABET.end()) continue;

        LetterFormula form = ALPHABET[c];
        int letter_samples = static_cast<int>(form.duration * SAMPLE_RATE);

        for (int s = 0; s < letter_samples; ++s) {
            double alpha = static_cast<double>(s) / letter_samples;

            // Mathematical interpolation: F(t) = Start_Freq + alpha * (End_Freq - Start_Freq)
            double f1 = form.f1_start + alpha * (form.f1_end - form.f1_start);
            double f2 = form.f2_start + alpha * (form.f2_end - form.f2_start);
            double f3 = form.f3_start + alpha * (form.f3_end - form.f3_start);

            // Phase tracking loops
            phase1 += (2.0 * PI * f1) / SAMPLE_RATE;
            phase2 += (2.0 * PI * f2) / SAMPLE_RATE;
            phase3 += (2.0 * PI * f3) / SAMPLE_RATE;

            if (phase1 > 2.0 * PI) phase1 -= 2.0 * PI;
            if (phase2 > 2.0 * PI) phase2 -= 2.0 * PI;
            if (phase3 > 2.0 * PI) phase3 -= 2.0 * PI;

            // Generate the dynamic trigonometric composite sample
            double sample_val = (form.a1 * std::sin(phase1)) +
                                (form.a2 * std::sin(phase2)) +
                                (form.a3 * std::sin(phase3));

            // Normalize the output mix
            sample_val = sample_val / (form.a1 + form.a2 + form.a3);

            // Smooth linear volume envelope (fade in/out per letter to prevent clipping)
            if (s < 400) {
                sample_val *= (static_cast<double>(s) / 400.0);
            } else if (s > letter_samples - 400) {
                sample_val *= (static_cast<double>(letter_samples - s) / 400.0);
            }

            short pcm_sample = static_cast<short>(sample_val * 32767);
            pcm_data.push_back(pcm_sample);
        }
    }

    std::ofstream wav_file(output_filename.c_str(), std::ios::binary);
    if (!wav_file.is_open()) {
        std::cerr << "Error writing output file.\n";
        return 1;
    }

    int data_byte_size = pcm_data.size() * sizeof(short);
    write_wav_header(wav_file, data_byte_size);
    wav_file.write(reinterpret_cast<const char*>(pcm_data.data()), data_byte_size);
    wav_file.close();

    std::cout << "[+] Audio synthesized cleanly to: " << output_filename << "\n";
    std::cout << "[*] Executing speaker audio playback stream...\n";
    
    std::string command = "afplay " + output_filename;
    std::system(command.c_str());

    return 0;
}
