#include "aurora.h"
#include "daisysp.h"

using namespace daisy;
using namespace aurora;
using namespace daisysp;

const int NUM_OSC = 5;

const int NUM_SCALES = 2; // TODO: make scales a proper enum, prob controled by an input somehow
const int MAJOR = 0;
const int MINOR = 1;

int knob_offset = 0;
int individual_offset = 0;
bool individual_trigger = false;

Hardware hw;
Oscillator osc[NUM_OSC];

// maybe make these the entire scales for more easily understanding the offsets?
float semitone_offsets[NUM_SCALES][NUM_OSC] = {
	{0.0, 4.0, 7.0, 11.0, 14.0},
	{0.0, 3.0, 7.0, 10.0, 14.0}
};

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();

	// TODO: Get the v/oct from an input, use as part of the base freq? (this is mapped in hz)
    float knob_freq = fmap(hw.GetKnobValue(KNOB_WARP), 10.0, 1500.0, Mapping::LOG);
	float warp_input = hw.GetCvValue(CV_WARP);
	float freq = (powf(2.0f, (warp_input * 5.0f)) * knob_freq);
    float volume = hw.GetKnobValue(KNOB_BLUR);

	//  time knob is the chord size for the chord output
	int chord_size = floor(fmap(hw.GetKnobValue(KNOB_TIME), 1.0, 6.0));

	// reflect knob is the offset for the individual output

	int current_knob_offset = floor(fmap(hw.GetKnobValue(KNOB_REFLECT), 1.0, chord_size + 1));
	if (knob_offset != current_knob_offset) {
		knob_offset = current_knob_offset;
		individual_offset = current_knob_offset;
	}

	// the reverse triggers the next individual offset (arpeggio-esque)
	bool current_individual_trigger = hw.GetCvValue(CV_ATMOSPHERE) > 0.5;
	if (current_individual_trigger != individual_trigger) {
		individual_trigger = current_individual_trigger;
		if (individual_trigger) {
			individual_offset += 1.0;
			if (individual_offset == (chord_size+1.0)) {
				individual_offset = 1.0;
			} 
		}
	}

	hw.SetLed(LED_1, individual_offset == 1.0, individual_offset == 1.0, chord_size >= 1.0);
	hw.SetLed(LED_2, individual_offset == 2.0, individual_offset == 2.0, chord_size >= 2.0);
	hw.SetLed(LED_3, individual_offset == 3.0, individual_offset == 3.0, chord_size >= 3.0);
	hw.SetLed(LED_4, individual_offset == 4.0, individual_offset == 4.0, chord_size >= 4.0);
	hw.SetLed(LED_5, individual_offset == 5.0, individual_offset == 5.0, chord_size >= 5.0);
	hw.WriteLeds();
 
    for (size_t i = 0; i < size; i++) {
		float chord_output = 0.0;
		float individual_output = 0.0;
		for (int i = 0; i < chord_size; i++) {
			if (i == 0) {
				osc[i].SetFreq(freq);
			} else {
				osc[i].SetFreq(freq * pow(2.0, (semitone_offsets[MINOR][i]/12.0)));
			}
			float processed_osc = osc[i].Process();
			chord_output += processed_osc;
			if (i == (individual_offset - 1)) {
				individual_output = processed_osc;
			}
		}
		out[0][i] = chord_output * volume;
		out[1][i] = individual_output * volume;
    }
}

int main(void)
{
    /** Initialize the Hardware */
    hw.Init();

	for (int i=0;i < NUM_OSC; i++) {
		/** Initialize the Oscillator we'll use to modulate our signal */
		osc[i].Init(hw.AudioSampleRate());
		osc[i].SetWaveform(Oscillator::WAVE_SAW);
		osc[i].SetAmp(1.0);
	}

	/** Start the audio engine calling the function defined above periodically */
	hw.StartAudio(AudioCallback);

    /** Infinite Loop */
    while (1)
    {
    }
}