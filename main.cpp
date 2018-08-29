#include <list>
#include <iostream>
#include <algorithm>

#define FTYPE double
#include "olcNoiseMaker.h"

namespace synth
{
	FTYPE w(FTYPE dHerz)		//Converts frequency(Hz) to angular velocity
	{
		return dHerz * 2.0 * PI;
	}

	struct note			//A basic note
	{
		int id;			//Position in scale
		FTYPE on;		//Time note was activated
		FTYPE off;		//Time note was deactivated
		bool active;
		int channel;

		note()
		{
			id = 0;
			on = 0.0;
			off = 0.0;
			active = false;
			channel = 0;
		}
	};

	const int OSC_SINE = 0;
	const int OSC_SQUARE = 1;
	const int OSC_TRIANGLE = 2;
	const int OSC_SAW_AN = 3;
	const int OSC_SAW_OP = 4;
	const int OSC_NOISE = 5;

	FTYPE osc(FTYPE dHertz, FTYPE dTime, int Type = OSC_SINE, FTYPE dLFOHertz = 0.0, FTYPE dLFOAmplitude = 0.0)
	{
		FTYPE dFreq = w(dHertz) * dTime + dLFOAmplitude * dHertz * sin(w(dLFOHertz) * dTime);			//base frequency

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
			FTYPE dOutput = 0.0;

			for (FTYPE n = 1.0; n < 10.0; n++)
				dOutput += (sin(n * dFreq)) / n;

			return dOutput * (2.0 / PI);
		}

		case OSC_SAW_OP:		//Saw wave (optimized / harsh / fast)
			return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

		case OSC_NOISE:		//Pseudo Random Noise
			return 2.0 * ((FTYPE)rand() / (FTYPE)RAND_MAX) - 1.0;

		default:
			return 0.0;
		}
	}

	const int SCALE_DEFAULT = 0;

	FTYPE scale(const int nNoteID, const int nScaleID = SCALE_DEFAULT)
	{
		switch (nScaleID)
		{
		case SCALE_DEFAULT: default:
			return 256 * pow(1.0594630943592952645618252949463, nNoteID);
		}
	}

	struct envelope
	{
		virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) = 0;
	};

	struct envelope_adsr : public envelope
	{
		FTYPE dAttackTime;
		FTYPE dDecayTime;
		FTYPE dReleaseTime;
		FTYPE dSustainAmplitude;
		FTYPE dStartAmplitude;

		envelope_adsr()
		{
			dAttackTime = 0.001;
			dDecayTime = 1.0;
			dStartAmplitude = 1.0;
			dSustainAmplitude = 0.0;
			dReleaseTime = 1.0;
		}

		virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff)
		{
			FTYPE dAmplitude = 0.0;
			FTYPE dReleaseAmplitude = 0.0;

			if (dTimeOn > dTimeOff)			//Note is on
			{
				FTYPE dLifeTime = dTime - dTimeOn;

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

			else		//Note is off
			{
				FTYPE dLifeTime = dTimeOff - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dReleaseAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dReleaseAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dReleaseAmplitude = dSustainAmplitude;

				//Release
				dAmplitude = ((dTime - dTimeOff) / dReleaseTime) * (0.0 - dReleaseAmplitude) + dReleaseAmplitude;
			}

			if (dAmplitude <= 0.000)
				dAmplitude = 0.0;


			return dAmplitude;
		}
	};

	FTYPE env(const FTYPE dTime, envelope &env, const FTYPE dTimeOn, const FTYPE dTimeOff)
	{
		return env.amplitude(dTime, dTimeOn, dTimeOff);
	}

	struct instrument_base
	{
		FTYPE dVolume;
		synth::envelope_adsr env;
		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool&bNoteFinished) = 0;
	};

	struct bell : public instrument_base
	{
		bell()
		{
			env.dAttackTime = 0.001;
			env.dDecayTime = 1.0;
			//env.dStartAmplitude = 1.0;
			env.dSustainAmplitude = 0.0;
			env.dReleaseTime = 1.0;

			dVolume = 1.0;
		}

		FTYPE sound(const FTYPE dTime, synth::note n, bool&bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);

			if (dAmplitude <= 0.0)
				bNoteFinished = true;

			FTYPE dSound =
				+1.0 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SINE, 5.0, 0.001)
				+ 0.5 * synth::osc(n.on - dTime, synth::scale(n.id + 12))
				+ 0.25 * synth::osc(n.on - dTime, synth::scale(n.id + 24));

			return dAmplitude * dSound * dVolume;
		}
	};

	struct harmonica : public instrument_base
	{
		harmonica()
		{
			env.dAttackTime = 0.05;
			env.dDecayTime = 1.0;
			env.dReleaseTime = 0.1;
			env.dSustainAmplitude = 0.95;
			//env.dStartAmplitude = 0.200;

			dVolume = 1.0;
		}

		FTYPE sound(const FTYPE dTime, synth::note n, bool&bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);

			if (dAmplitude <= 0.0)
				bNoteFinished = true;

			FTYPE dSound =
				+1.0 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.5 * synth::osc(n.on - dTime, synth::scale(n.id + 12), synth::OSC_SQUARE)
				+ 0.25 * synth::osc(n.on - dTime, synth::scale(n.id + 24), synth::OSC_SQUARE)
				+ 0.05 * synth::osc(n.on - dTime, dTime, synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}
	};

	struct piano : public instrument_base
	{
		piano()
		{
			env.dAttackTime = 0.100;
			env.dDecayTime = 0.01;
			env.dReleaseTime = 0.01;
			env.dSustainAmplitude = 0.8;
			//env.dStartAmplitude = 0.200;

			dVolume = 1.0;
		}

		FTYPE sound(const FTYPE dTime, synth::note n, bool&bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);

			if (dAmplitude <= 0.0)
				bNoteFinished = true;

			FTYPE dSound =
				+1.0 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SINE, 5.0, 0.001)
				+ 0.5 * synth::osc(n.on - dTime, synth::scale(n.id + 12), synth::OSC_SINE);

			return dAmplitude * dSound * dVolume;
		}
	};
}

vector<synth::note> vecNotes;
mutex muxNotes;
synth::bell instrBell;
synth::harmonica instrHarm;
synth::piano instrPiano;

//synth::instrument_base *voice = nullptr;

typedef bool(*lambda)(synth::note const& item);
template<class T>
void safe_remove(T &v, lambda f)
{
	auto n = v.begin();
	while (n != v.end())
		if (!f(*n))
			n = v.erase(n);
		else
			++n;
}

FTYPE MakeNoise(FTYPE dTime)
{
	unique_lock<mutex> lm(muxNotes);
	FTYPE dMixedOutput = 0.0;

	for (auto &n : vecNotes)
	{
		bool bNoteFinished = false;
		FTYPE dSound = 0;

		if (n.channel == 2)
			dSound = instrBell.sound(dTime, n, bNoteFinished);
		if (n.channel == 1)
			dSound = instrHarm.sound(dTime, n, bNoteFinished);
		if (n.channel == 0)
			dSound = instrPiano.sound(dTime, n, bNoteFinished);

		dMixedOutput += dSound;

		if (bNoteFinished && n.off > n.on)
			n.active = false;
	}

	safe_remove<vector<synth::note>>(vecNotes, [](synth::note const& item) { return item.active; });

	return dMixedOutput * 0.05;		//Master volume
}

int main()
{
	wcout << "Synthesizer" << endl;

	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

	wcout << endl <<
		"|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl <<
		"|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl <<
		"|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__" << endl <<
		"|     |     |     |     |     |     |     |     |     |     |" << endl <<
		"|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |" << endl <<
		"|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;
		//"\nPress '1' for harmonica(default),'2' for Bell and '3' for Piano" << endl << endl;


	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

	//voice = new synth::harmonica();

	sound.SetUserFunction(MakeNoise);

	char keyboard[129];
	memset(keyboard, ' ', 127);
	keyboard[128] = '\0';

	auto clock_old_time = chrono::high_resolution_clock::now();
	auto clock_real_time = chrono::high_resolution_clock::now();
	FTYPE dElapsedTime = 0.0;

	while (1)
	{
		for (int k = 0; k < 16; k++)
		{
			short nKeyState = GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k]));

			FTYPE dTimeNow = sound.GetTime();

			//Check if note already exists in currently playing notes
			muxNotes.lock();
			auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&k](synth::note const& item) { return item.id == k; });
			if (noteFound == vecNotes.end())
			{
				if (nKeyState & 0x8000)
				{
					synth::note n;
					n.id = k;
					n.on = dTimeNow;
					n.channel = 0;
					n.active = true;

					vecNotes.emplace_back(n);		//Adding note to vector
				}
				else
				{
					//do nothing
				}
			}

			else
			{
				if (nKeyState & 0x8000)
				{
					if (noteFound->off > noteFound->on)
					{
						//Key has been pressed again during the release state
						noteFound->on = dTimeNow;
						noteFound->active = true;
					}
				}

				else
				{
					if (noteFound->off < noteFound->on)
					{
						//Key has been released so switch off
						noteFound->off = dTimeNow;
					}
				}
			}

			muxNotes.unlock();
		}

		wcout << "\rNotes:" << vecNotes.size() << "			";
	}

	return 0;
}