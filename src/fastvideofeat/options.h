#include <fstream>
#include <cstdio>
#include <string>

#include "../Commons/io_utils.h"

using namespace std;

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

struct Options
{
	string VideoPath;
	bool HogEnabled, HofEnabled, MbhEnabled;
	bool PrintDescriptors;
	bool Dense;
	bool Interpolation;
	bool DumpFrames;
	bool DebugMode;

	bool UseFarnebackFlow;
	bool UseWarping;
	int FlowResample;

	vector<int> GoodPts;
	string OutputDir;

	void Explain()
	{
		if(DebugMode)
			log("#We're in DEBUG mode");
		log("Options:");
		log("Input video: %s", VideoPath.c_str());
		log("HOG's enabled: %s", yesno(HogEnabled));
		log("HOF's enabled: %s", yesno(HofEnabled));
		log("MBH's enabled: %s", yesno(MbhEnabled));

		log("Print descriptors: %s", yesno(PrintDescriptors));
		log("Dense-dense-revolution: %s", yesno(Dense));
		log("Interpolation: %s", yesno(Interpolation));
		log("Output dir: %s", OutputDir.c_str());
		log("Dump frames: %s", yesno(DumpFrames));
		log("Use warping: %s", yesno(UseWarping));
		log("Use Farneback flow: %s", yesno(UseFarnebackFlow));
		log("Flow resample: %d", FlowResample);
		fprintf(stderr, "Good PTS: ");
		for(int i = 0; i < GoodPts.size(); i++)
			fprintf(stderr, "%d, ", GoodPts[i]);
		log(stderr, "");
	}
	
	void ParseCommandLine(int argc, char* argv[])
	{
		for(int i = 1; i < argc-1; i += 2)
		{
			if(strcmp(argv[i], "-i") == 0)
				VideoPath = string(argv[i+1]);
			else if(strcmp(argv[i], "-hog") == 0)
				HogEnabled = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-hof") == 0)
				HofEnabled = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-mbh") == 0)
				MbhEnabled = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-stdout") == 0)
				PrintDescriptors = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-dense") == 0)
				Dense = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-interpolation") == 0)
				Interpolation = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-o") == 0)
				OutputDir = argv[i+1];
			else if(strcmp(argv[i], "-useFarneback") == 0)
				UseFarnebackFlow = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-flowResample") == 0)
				FlowResample = atoi(argv[i+1]);
			else if(strcmp(argv[i], "-warp") == 0)
				UseWarping = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-pts") == 0)
			{
				int b, e;
				sscanf(argv[i+1], "%d-%d", &b, &e);
				for(int i = b; i <= e; i++)
					GoodPts.push_back(i);
			}
			else if(strcmp(argv[i], "-dumpFrames") == 0)
				DumpFrames = strcmp(argv[i+1], yes) == 0;
			else
			{
				printf("Unknown option: %s", argv[i]);
				exit(0);
			}
		}
	}

	void SetDefaults()
	{
		DebugMode = false;		
		HogEnabled = true;
		HofEnabled = true;
		MbhEnabled = true;
		PrintDescriptors = true;
		Dense = false;
		Interpolation = true;
		DumpFrames = false;
		UseFarnebackFlow = false;
		UseWarping = false;
		FlowResample = 1;
	}

	void Check()
	{
		AssertFileExists(VideoPath, "video path");
	}

	void SetDebugDefaults()
	{
		DebugMode = true;
		
		VideoPath = "hwd2/actioncliptest00040.avi";
		//Dense = true;
		//Interpolation = false;
		PrintDescriptors = false;
	}

	Options(int argc, char* argv[])
	{
		SetDefaults();
		if(argc < 2)
			SetDebugDefaults();
		else
			ParseCommandLine(argc, argv);
		Explain();
		Check();
	}
};

#endif
