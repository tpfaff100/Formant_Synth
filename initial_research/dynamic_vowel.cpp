//clang++ -O3 dynamic_vowel.cpp -o dynamic_vowel
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <cstdlib>

const int SAMPLE_RATE = 22050;
const double PI = 3.14159265358979323846;

// Formant definitions for the letter 'a' (frequencies in Hz and relative amplitudes)
const double F1_FREQ = 730.0;  const double F1_AMP = 0.6;
const double F2_FREQ = 1090.0; const double F2_AMP = 0.5;
const double F3_FREQ = 2440.0; const double F3_AMP = 0.3;

// RIFF WAVE Header writer
void write_wav_header(std::ofstream& file, int data_size) {
    file.write("RIFF", 4);
    int chunk_size = 36 + data_size;
    file.write(reinterpret_cast<const char*>(&chunk_size), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    
    int subchunk1_size = 16;
    short audio_format = 1; // Uncompressed PCM
    short num_channels = 1; // Mono
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

int main() {
    double duration_seconds = 1.0;
    int total_samples = static_cast<int>(duration_seconds * SAMPLE_RATE);
    std::vector<short> pcm_data;

    std::cout << "[*] Dynamically compounding trigonometric formants for 'a'...\n";

    for (int t = 0; t < total_samples; ++t) {
        // Convert the current sample index into a precise time variable (seconds)
        double time = static_cast<double>(t) / SAMPLE_RATE;

        // The Sin/Cos mathematical core: 
        // Signal = A1*sin(2*pi*F1*t) + A2*sin(2*pi*F2*t) + A3*sin(2*pi*F3*t)
        double sample_val = (F1_AMP * std::sin(2.0 * PI * F1_FREQ * time)) +
                            (F2_AMP * std::sin(2.0 * PI * F2_FREQ * time)) +
                            (F3_AMP * std::sin(2.0 * PI * F3_FREQ * time));

        // Normalize the mixed signal so it sits perfectly within a -1.0 to 1.0 boundary
        sample_val = sample_val / (F1_AMP + F2_AMP + F3_AMP);

        // Optional: Apply a very slight fade-out envelope at the end to prevent an abrupt audio pop
        if (t > total_samples - 1000) {
            double fade = static_cast<double>(total_samples - t) / 1000.0;
            sample_val *= fade;
        }

        // Quantize floating-point spectrum directly to signed 16-bit PCM integer boundaries
        short pcm_sample = static_cast<short>(sample_val * 32767);
        pcm_data.push_back(pcm_sample);
    }

    std::string filename = "dynamic_a.wav";
    std::ofstream wav_file(filename.c_str(), std::ios::binary);
    if (!wav_file.is_open()) {
        std::cerr << "Error opening file for write sequence.\n";
        return 1;
    }

    int data_byte_size = pcm_data.size() * sizeof(short);
    write_wav_header(wav_file, data_byte_size);
    wav_file.write(reinterpret_cast<const char*>(pcm_data.data()), data_byte_size);
    wav_file.close();

    std::cout << "[+] 'a' vowel saved to: " << filename << "\n";
    std::cout << "[*] Streaming directly to speakers via macOS system pipeline...\n";
    
    std::string command = "afplay " + filename;
    std::system(command.c_str());

    return 0;
}

