#include <iostream>
#include "olcNoiseMaker.h"

double w(double dHerz)		//Converts frequency(Hz) to angular velocity
{
	return dHerz * 2.0 * PI;
}

double osc(double dHerz, double dTime, int Type)
{
	switch (Type)
	{
	case 0:		//Sine wave
		return sin(w(dHerz) * dTime);

	case 1:		//Sqare wave
		return sin(w(dHerz) * dTime) > 0.0 ? 1.0 : -1.0;

	case 2:		//Triangle wave
		return asin(sin(w(dHerz) * dTime)) * 2 / PI;

	case 3:		//Saw wave (analogue / warm / slow)
	{
		double dOutput = 0.0;

		for (double n = 1.0; n < 10.0; n++)
			dOutput += (sin(n * w(dHerz) * dTime)) / n;

		return dOutput * (2.0 / PI);
	}

	case 4:		//Saw wave (optimized / harsh / fast)
		return (2.0 / PI) * (dHerz * PI * fmod(dTime, 1.0 / dHerz) - (PI / 2.0));

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
		dAttackTime = 0.100;
		dDecayTime = 0.01;
		dReleaseTime = 1.0;
		dSustainAmplitude = 0.8;
		dStartAmplitude = 0.200;
		dTriggerOnTime = 0.0;
		dTriggerOffTime = 0.0;
		bNoteOn = false;
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

double MakeNoise(double dTime)
{
	double dOutput = envelope.GetAmplitude(dTime) *
		(
			+osc(dFrequencyOutput * 0.5, dTime, 3)
			+ osc(dFrequencyOutput * 1.0, dTime, 1)
		);
		

	return dOutput * 0.1;		//Master volume
}

int main()
{
	std::wcout << "Synthesizer" << std::endl;

	vector<std::wstring> devices = olcNoiseMaker<short>::Enumerate();

	for (auto d : devices) wcout << "Found output device: " << d << std::endl;

	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

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
					envelope.NoteOn(sound.GetTime());
					//wcout << "r\NoteOn : " << sound.GetTime() << "s; Frequency: " << dFrequencyOutput << std::endl;
					nCurrentKey = k;
				}

				bKeyPressed = true;
			}
		}

		if (!bKeyPressed)
		{
			if (nCurrentKey != -1)
			{
				//wcout << "\rNoteOff: " << sound.GetTime << "s" << std::endl;
				envelope.NoteOff(sound.GetTime());
				nCurrentKey = -1;
			}
		}
	}

	return 0;
}
