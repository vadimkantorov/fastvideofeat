#include <fstream>
#include <cstdio>
#include <string>

using namespace std;

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

bool FileExists(string file)
{
	ifstream f(file.c_str());
	return f.good();
}

void AssertFileExists(string file, string comment = "")
{
	if(!FileExists(file))
		throw std::runtime_error("File (" + comment + ") doesn't exist: '" + file +"'");
}

static const char* yes = "yes";
static const char* no = "no";

const char* yesno(bool b)
{
	return b ? yes : no;
}

struct Options
{
	string VideoPath;
	bool HogEnabled, HofEnabled, MbhEnabled;
	bool Dense;
	bool Interpolation;

	vector<int> GoodPts;

	void Explain()
	{
		log("Options:");
		log("Input video: %s", VideoPath.c_str());
		log("HOG's enabled: %s", yesno(HogEnabled));
		log("HOF's enabled: %s", yesno(HofEnabled));
		log("MBH's enabled: %s", yesno(MbhEnabled));

		log("Dense-dense-revolution: %s", yesno(Dense));
		log("Interpolation: %s", yesno(Interpolation));
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
			else if(strcmp(argv[i], "-dense") == 0)
				Dense = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-interpolation") == 0)
				Interpolation = strcmp(argv[i+1], yes) == 0;
			else if(strcmp(argv[i], "-f") == 0)
			{
				int b, e;
				sscanf(argv[i+1], "%d-%d", &b, &e);
				for(int i = b; i <= e; i++)
					GoodPts.push_back(i);
			}
			else
			{
				printf("Unknown option: %s", argv[i]);
				exit(0);
			}
		}
	}

	void SetDefaults()
	{
		HogEnabled = true;
		HofEnabled = true;
		MbhEnabled = true;
		Dense = false;
		Interpolation = true;
	}

	void Check()
	{
		AssertFileExists(VideoPath, "video path");
	}

	void SetDebugDefaults()
	{
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
