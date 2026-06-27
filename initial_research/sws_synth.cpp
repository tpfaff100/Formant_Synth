// clang++ -std=c++17 -O3 sws_synth.cpp -o sws_synth

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <map>
#include <algorithm>
#include <cstdlib> // Necessary for std::system playback hooks

const int SAMPLE_RATE = 22050;
const double PI = 3.14159265358979323846;

struct Formant {
    double freq;
    double amp;
};

struct FormantFrame {
    int duration_ms;
    std::vector<Formant> formants;
};

std::map<std::string, std::vector<FormantFrame> > PHONEMES;

// Safely populates the dictionary avoiding modern recursive brace-initialization layout quirks
void initialize_phoneme_dictionary() {
    FormantFrame f;
    
    // --- Vowels ---
    // 'a'
    f.duration_ms = 300; f.formants.clear();
    f.formants.push_back({730, 0.6}); f.formants.push_back({1090, 0.5}); f.formants.push_back({2440, 0.3});
    PHONEMES["a"].push_back(f);

    // 'i'
    f.duration_ms = 300; f.formants.clear();
    f.formants.push_back({270, 0.6}); f.formants.push_back({2290, 0.5}); f.formants.push_back({3010, 0.3});
    PHONEMES["i"].push_back(f);

    // 'u'
    f.duration_ms = 300; f.formants.clear();
    f.formants.push_back({300, 0.6}); f.formants.push_back({870, 0.4}); f.formants.push_back({2240, 0.2});
    PHONEMES["u"].push_back(f);

    // --- Nasals ---
    // 'm'
    f.duration_ms = 120; f.formants.clear();
    f.formants.push_back({250, 0.5}); f.formants.push_back({1000, 0.1}); f.formants.push_back({2500, 0.05});
    PHONEMES["m"].push_back(f);

    // 'n'
    f.duration_ms = 120; f.formants.clear();
    f.formants.push_back({250, 0.5}); f.formants.push_back({1700, 0.1}); f.formants.push_back({2500, 0.05});
    PHONEMES["n"].push_back(f);

    // --- Glides ---
    // 'w'
    f.duration_ms = 60; f.formants.clear();
    f.formants.push_back({300, 0.5}); f.formants.push_back({700, 0.3}); f.formants.push_back({2200, 0.1});
    PHONEMES["w"].push_back(f);
    f.duration_ms = 60; f.formants.clear();
    f.formants.push_back({500, 0.5}); f.formants.push_back({900, 0.4}); f.formants.push_back({2300, 0.2});
    PHONEMES["w"].push_back(f);

    // 'l'
    f.duration_ms = 80; f.formants.clear();
    f.formants.push_back({310, 0.5}); f.formants.push_back({1050, 0.3}); f.formants.push_back({2600, 0.2});
    PHONEMES["l"].push_back(f);
    f.duration_ms = 40; f.formants.clear();
    f.formants.push_back({500, 0.5}); f.formants.push_back({1500, 0.4}); f.formants.push_back({2500, 0.2});
    PHONEMES["l"].push_back(f);

    // --- Plosives ---
    // 'b'
    f.duration_ms = 40; f.formants.clear();
    f.formants.push_back({0, 0.0}); f.formants.push_back({0, 0.0}); f.formants.push_back({0, 0.0});
    PHONEMES["b"].push_back(f);
    f.duration_ms = 30; f.formants.clear();
    f.formants.push_back({200, 0.6}); f.formants.push_back({700, 0.4}); f.formants.push_back({2000, 0.2});
    PHONEMES["b"].push_back(f);

    // 'd'
    f.duration_ms = 40; f.formants.clear();
    f.formants.push_back({0, 0.0}); f.formants.push_back({0, 0.0}); f.formants.push_back({0, 0.0});
    PHONEMES["d"].push_back(f);
    f.duration_ms = 30; f.formants.clear();
    f.formants.push_back({350, 0.6}); f.formants.push_back({1600, 0.5}); f.formants.push_back({2600, 0.3});
    PHONEMES["d"].push_back(f);

    // --- Fricatives ---
    // 's'
    f.duration_ms = 150; f.formants.clear();
    f.formants.push_back({4500, 0.4}); f.formants.push_back({5500, 0.5}); f.formants.push_back({6500, 0.4});
    PHONEMES["s"].push_back(f);

    // 'sh'
    f.duration_ms = 150; f.formants.clear();
    f.formants.push_back({2500, 0.4}); f.formants.push_back({3500, 0.5}); f.formants.push_back({4500, 0.4});
    PHONEMES["sh"].push_back(f);

    // 'f'
    f.duration_ms = 120; f.formants.clear();
    f.formants.push_back({3000, 0.2}); f.formants.push_back({4000, 0.2}); f.formants.push_back({5000, 0.1});
    PHONEMES["f"].push_back(f);
}

std::vector<Formant> interpolate_frames(const std::vector<Formant>& start, const std::vector<Formant>& end, double alpha) {
    std::vector<Formant> result;
    for (size_t i = 0; i < start.size(); ++i) {
        double f = start[i].freq + (end[i].freq - start[i].freq) * alpha;
        double a = start[i].amp + (end[i].amp - start[i].amp) * alpha;
        Formant interpolated_formant = {f, a};
        result.push_back(interpolated_formant);
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
        std::cout << "Usage: " << argv[0] << " <phoneme1> <phoneme2> ... [-o output.wav]\n";
        return 1;
    }

    initialize_phoneme_dictionary();

    std::vector<std::string> sequence;
    std::string output_filename = "output.wav";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            output_filename = argv[++i];
        } else {
            sequence.push_back(arg);
        }
    }

    std::vector<FormantFrame> all_frames;
    for (size_t k = 0; k < sequence.size(); ++k) {
        std::string ph = sequence[k];
        if (PHONEMES.find(ph) != PHONEMES.end()) {
            for (size_t f_idx = 0; f_idx < PHONEMES[ph].size(); ++f_idx) {
                all_frames.push_back(PHONEMES[ph][f_idx]);
            }
        } else {
            std::cerr << "Warning: Phoneme '" << ph << "' not recognized. Skipping.\n";
        }
    }

    if (all_frames.empty()) {
        std::cerr << "Error: No valid phonetic tokens provided.\n";
        return 1;
    }

    std::vector<short> pcm_data;
    std::vector<double> master_phases;
    master_phases.push_back(0.0); master_phases.push_back(0.0); master_phases.push_back(0.0);

    std::cout << "[*] Synthesizing SWS audio profile via C++ engine...\n";

    for (size_t i = 0; i < all_frames.size(); ++i) {
        int duration_ms = all_frames[i].duration_ms;
        std::vector<Formant> current_targets = all_frames[i].formants;
        std::vector<Formant> next_targets = (i + 1 < all_frames.size()) ? all_frames[i + 1].formants : current_targets;
        
        int total_samples = static_cast<int>((duration_ms / 1000.0) * SAMPLE_RATE);
        
        for (int s = 0; s < total_samples; ++s) {
            double alpha = static_cast<double>(s) / total_samples;
            std::vector<Formant> current_formants = interpolate_frames(current_targets, next_targets, alpha);
            
            double sample_val = 0.0;
            
            for (size_t idx = 0; idx < current_formants.size(); ++idx) {
                double freq = current_formants[idx].freq;
                double amp = current_formants[idx].amp;
                
                if (freq > 0.0) {
                    master_phases[idx] += (2.0 * PI * freq) / SAMPLE_RATE;
                    if (master_phases[idx] > 2.0 * PI) master_phases[idx] -= 2.0 * PI;
                    
                    sample_val += amp * std::sin(master_phases[idx]);
                }
            }
            
            sample_val = sample_val / 3.0; 
            if (sample_val > 1.0) sample_val = 1.0;
            if (sample_val < -1.0) sample_val = -1.0;
            
            short pcm_sample = static_cast<short>(sample_val * 32767);
            pcm_data.push_back(pcm_sample);
        }
    }

    std::ofstream wav_file(output_filename.c_str(), std::ios::binary);
    if (!wav_file.is_open()) {
        std::cerr << "Error: Could not open output file " << output_filename << "\n";
        return 1;
    }

    int data_byte_size = pcm_data.size() * sizeof(short);
    write_wav_header(wav_file, data_byte_size);
    wav_file.write(reinterpret_cast<const char*>(pcm_data.data()), data_byte_size);
    wav_file.close();

    std::cout << "[+] Sound package completed successfully: " << output_filename << "\n";
    
    // --- NATIVE SPEAKER PLAYBACK EXECUTION ---
    std::cout << "[*] Sending SWS waveform data directly to speakers...\n";
    std::string command = "afplay " + output_filename;
    std::system(command.c_str());
    
    return 0;
}
