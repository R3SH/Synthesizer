#include <iostream>
#include "olcNoiseMaker.h"

#define OSC_SINE 0
#define OSC_SQUARE 1
#define OSC_TRIANGLE 2
#define OSC_SAW_AN 3
#define OSC_SAW_OP 4
#define OSC_NOISE 5

double w(double dHerz)		//Converts frequency(Hz) to angular velocity
{
	return dHerz * 2.0 * PI;
}

double osc(double dHertz, double dTime, int Type = OSC_SINE, double dLFOHertz = 0.0, double dLFOAmplitude = 0.0)
{
	double dFreq = w(dHertz) * dTime + dLFOAmplitude * dHertz * sin(w(dLFOHertz) * dTime);			//base frequency

	switch (Type)
	{
	case OSC_SINE:		//Sine wave
		return sin(dFreq);

	case OSC_SQUARE:		//Sqare wave
		return sin(dFreq) > 0.0 ? 1.0 : -1.0;

	case OSC_TRIANGLE:		//Triangle wave
		return asin(sin(dFreq)) * (2.0 / PI);

	case OSC_SAW_AN:		//Saw wave (analogue / warm / slow)
	{
		double dOutput = 0.0;

		for (double n = 1.0; n < 10.0; n++)
			dOutput += (sin(n * dFreq)) / n;

		return dOutput * (2.0 / PI);
	}

	case OSC_SAW_OP:		//Saw wave (optimized / harsh / fast)
		return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

	case OSC_NOISE:		//Pseudo Random Noise
		return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

	default:
		return 0.0;
	}
}

struct sEnvelopeADSR
{
	double dAttackTime;
	double dDecayTime;
	double dReleaseTime;
	double dSustainAmplitude;
	double dStartAmplitude;

	double dTriggerOnTime;
	double dTriggerOffTime;

	bool bNoteOn;

	sEnvelopeADSR()
	{
		dAttackTime = 0.001;
		dDecayTime = 1.0;
		dStartAmplitude = 1.0;
		dSustainAmplitude = 0.0;
		dReleaseTime = 1.0;
		bNoteOn = false;
		dTriggerOnTime = 0.0;
		dTriggerOffTime = 0.0;
	}
	double GetAmplitude(double dTime)
	{
		double dAmplitude = 0.0;
		double dLifeTime = dTime - dTriggerOnTime;

		if (bNoteOn)
		{
			// ASD

			//Attack
			if (dLifeTime <= dAttackTime)
				dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

			//Decay
			if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
				dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;
		
			//Sustain
			if (dLifeTime > (dAttackTime + dDecayTime))
				dAmplitude = dSustainAmplitude;
		}
		else
		{
			//Release
			dAmplitude = ((dTime - dTriggerOffTime) * (0.0 - dSustainAmplitude) + dSustainAmplitude);
		}

		if (dAmplitude <= 0.0001)
		{
			dAmplitude = 0;
		}


		return dAmplitude;
	}

	void NoteOn(double dTimeOn)
	{
		dTriggerOnTime = dTimeOn;
		bNoteOn = true;
	}

	void NoteOff(double dTimeOff)
	{
		dTriggerOffTime = dTimeOff;
		bNoteOn = false;
	}
};

atomic<double> dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 110.0;
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);
sEnvelopeADSR envelope;

struct instrument
{
	double dVolume;
	sEnvelopeADSR env;
	virtual double sound(double dTime, double dFrequency) = 0;
};


struct bell : public instrument
{
	bell()
	{
		env.dAttackTime = 0.001;
		env.dDecayTime = 1.0;
		env.dStartAmplitude = 1.0;
		env.dSustainAmplitude = 0.0;
		env.dReleaseTime = 1.0;

		dVolume = 1.0;
	}

	double sound(double dTime, double dFrequency)
	{
		double dOutput = envelope.GetAmplitude(dTime) *
			(
			+1.0 * osc(dFrequencyOutput * 2.0, dTime, OSC_SINE, 5.0, 0.001)
			+ 0.5 * osc(dFrequencyOutput * 3.0, dTime, OSC_SINE)
			+ 0.25 * osc(dFrequencyOutput * 4.0, dTime, OSC_SINE)
			);

		return dOutput;
	}
};

instrument *voice = nullptr;

double MakeNoise( double dTime)
{
	//double dOutput = envelope.GetAmplitude(dTime) *
	//	(
	//		+ 1.0 * osc(dFrequencyOutput, dTime, OSC_SQUARE, 5.0, 0.001)
	//		+ 0.5 * osc(dFrequencyOutput * 1.5, dTime, OSC_SQUARE)
	//		+ 0.25 * osc(dFrequencyOutput * 2.0, dTime, OSC_SQUARE)
	//		+ 0.05 * osc(0, dTime, OSC_NOISE)		//Harmonica

	//		+1.0 * osc(dFrequencyOutput * 2.0, dTime, OSC_SINE, 5.0, 0.001)
	//		+ 0.5 * osc(dFrequencyOutput * 3.0, dTime, OSC_SINE)
	//		+ 0.25 * osc(dFrequencyOutput * 4.0, dTime, OSC_SINE)
	//	);

	double dOutput = voice->sound(dTime, dFrequencyOutput);

	return dOutput * 0.05;		//Master volume
}

int main()
{
	wcout << "Synthesizer" << std::endl;

	std::vector<std::wstring> devices = olcNoiseMaker<short>::Enumerate();

	wcout << endl <<
		"|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl <<
		"|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl <<
		"|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__" << endl <<
		"|     |     |     |     |     |     |     |     |     |     |" << endl <<
		"|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |" << endl <<
		"|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;


	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

	voice = new bell();

	sound.SetUserFunction(MakeNoise);


	int nCurrentKey = -1;
	bool bKeyPressed = false;

	while (1)
	{
		//Add a keyboard like piano
		bKeyPressed = false;

		for (int k = 0; k < 16; k++)
		{
			if (GetAsyncKeyState((unsigned char)"ZSXCFVGBNJMK\xbcL\xbe\xbf"[k]) & 0x8000)
			{
				if (nCurrentKey != k)
				{
					dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
					voice->env.NoteOn(sound.GetTime());
					wcout << "\rNoteOn : " << sound.GetTime() << "s; Frequency: " << dFrequencyOutput << "Hz";
					nCurrentKey = k;
				}

				bKeyPressed = true;
			}
		}

		if (!bKeyPressed)
		{
			if (nCurrentKey != -1)
			{
				wcout << "\rNoteOff: " << sound.GetTime() << "s					";
				voice->env.NoteOff(sound.GetTime());
				nCurrentKey = -1;
			}
		}
	}

	return 0;
}

