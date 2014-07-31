#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <string>
#include <vector>
#include <stdexcept>
#include "grid_part.h"

#include "io_utils.h"
#include "config.h"

using namespace std;

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

struct Options
{
	vector<Part> Parts;
	vector<Grid> Grids;

	int XnPos, YnPos, TnPos;
	float XnTotal, YnTotal, TnTotal;
	int GmmK, GmmNiter;
	float Lambda;
	bool BuildGmmIndex;
	int K_nn;
	int FlannKdTrees;
	int FlannChecks;
	bool DoSigma;
	int BinaryLineSize;
	bool UseBinary;
	string OutputFilePath;
	string FV_CONFIG;

	void Explain()
	{
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "Indices are zero-based\n");
		for(int i = 0; i < Parts.size(); i++)
		{
			fprintf(stderr, "%d-%d\t'%s'\n", Parts[i].FeatStart, Parts[i].FeatEnd, Parts[i].VocabPath.c_str()); 
		}
		fprintf(stderr, "Build GMM index: %s\n", BuildGmmIndex ? "yes" : "no");
		fprintf(stderr, "GMM components: %d\n", GmmK);
		fprintf(stderr, "GMM niter: %d\n", GmmNiter);
		fprintf(stderr, "Lambda reg: %f\n", Lambda);
		fprintf(stderr, "Output file path: %s\n", OutputFilePath.c_str());
		fprintf(stderr, "K_nn: %d\n", K_nn);
		fprintf(stderr, "FLANN trees: %d\n", FlannKdTrees);
		fprintf(stderr, "FLANN checks: %d\n", FlannChecks);
		fprintf(stderr, "Do sigma params: %s\n", DoSigma ? "yes" : "no");
		fprintf(stderr, "FV_CONFIG path: %s\n", FV_CONFIG == "" ? "none" : FV_CONFIG.c_str());
		fprintf(stderr, "XnPos: %d, YnPos: %d, TnPos: %d\n", XnPos, YnPos, TnPos);
		fprintf(stderr, "Xtot: %f, Ytot: %f, Ttot: %f\n", XnTotal, YnTotal, TnTotal);
		fprintf(stderr, "Grids: ");
		for(int i = 0; i < Grids.size(); i++)
		{
			fprintf(stderr, "%s ", Grids[i].ToString);
		}
		fprintf(stderr, "\nUse binary: %s, line size: %d\n", UseBinary ? "yes" : "no", BinaryLineSize);
		fprintf(stderr, "\n");
	}	
	
	void ParseCommandLine(int argc, char* argv[])
	{
		const int BUF = 1000;
		for(int i = 1; i < argc; )
		{
			if(strcmp(argv[i], "--buildGmmIndex") == 0)
			{
				BuildGmmIndex = true;
				i++;
			}
			else if(strcmp(argv[i], "--gmm_k") == 0)
			{
				GmmK = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--gmm_niter") == 0)
			{
				GmmNiter = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--lambda") == 0)
			{
				sscanf(argv[i+1], "%f", &Lambda);
				i += 2;
			}
			else if(strcmp(argv[i], "--knn") == 0)
			{
				K_nn = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--outputFile") == 0)
			{
				OutputFilePath = string(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--trees") == 0)
			{
				FlannKdTrees = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--checks") == 0)
			{
				FlannChecks = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--sigma") == 0)
			{
				DoSigma = strcmp(argv[i+1], "yes") == 0;
				i += 2;
			}
			else if(strcmp(argv[i], "--xtot") == 0)
			{
				XnTotal = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--ytot") == 0)
			{
				YnTotal = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--ttot") == 0)
			{
				TnTotal = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--xnpos") == 0)
			{
				XnPos = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--ynpos") == 0)
			{
				YnPos = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--tnpos") == 0)
			{
				TnPos = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--grid") == 0)
			{
				int x, y, t;
				char c1, c2, c3;
				sscanf(argv[i+1], "%d%c%d%c%d%c", &x, &c1, &y, &c2, &t, &c3);
				Grid g(x, y, t, c1 == 'o', c2 == 'o', c3 == 'o');
				Grids.push_back(g);
				i += 2;
			}
			else if(strcmp(argv[i], "--vocab") == 0)
			{
				Part p;
				sscanf(argv[i+1], "%d-%d", &p.FeatStart, &p.FeatEnd);
				p.VocabPath = argv[i+2];
				Parts.push_back(p);
				i += 3;
			}
			else if(strcmp(argv[i], "--binary") == 0)
			{
				UseBinary = true;
				BinaryLineSize = atoi(argv[i+1]);
				i += 2;
			}
			else
			{
				FV_CONFIG = argv[i];
				i++;
			}
		}
		
		FILE* f = FV_CONFIG != "" ? fopen(FV_CONFIG.c_str(), "r") : NULL;
		if(f != NULL)
		{
			char S[BUF];
			while(fgets(S, BUF-1, f))
			{
				char* SS = S;
				if(SS[0] == '#')
					continue;
				
				Part p;
				int res;

				if(string(SS).substr(0, 4) == "grid")
				{
					int x, y, t;
					char c1, c2, c3;
					sscanf(SS, "grid %d%c%d%c%d%c", &x, &c1, &y, &c2, &t, &c3);
					Grid g(x, y, t, c1 == 'o', c2 == 'o', c3 == 'o');
					Grids.push_back(g);
					continue;
				}

				if(sscanf(SS, "%d-%d\t", &p.FeatStart, &p.FeatEnd) < 1)
					continue;
				SS += strspn(SS, "0123456789-");
				SS += strspn(SS, " \t");
				p.VocabPath = SS;

				if(isspace(p.VocabPath[p.VocabPath.length()-1]))
					p.VocabPath = p.VocabPath.substr(0, p.VocabPath.length()-1);
				if(p.VocabPath == "xnpos")
					XnPos = p.FeatStart;
				else if(p.VocabPath == "ynpos")
					YnPos = p.FeatStart;
				else if(p.VocabPath == "tnpos")
					TnPos = p.FeatStart;
				else			
					Parts.push_back(p);
			}
			fclose(f);
		}
	}

	void SetDefaults()
	{
		BuildGmmIndex = false;
		GmmK = 256;
		GmmNiter = 50;
		Lambda = 1e-4;
		K_nn = 5;
		DoSigma = false;
		FlannKdTrees = 4;
		FlannChecks = 32;
		XnPos = YnPos = TnPos = -1;
		XnTotal = YnTotal = TnTotal = 1.0;

		UseBinary = false;
		BinaryLineSize = -1;
	}

	void Check()
	{
		if(!OutputFilePath.empty())
		{
			string suffix = GetFileExtension(OutputFilePath);
			assert(suffix == ".txt" || suffix == ".h5");
			if(suffix == ".h5" && !HAVE_HDF5)
			{
				fprintf(stderr, "FATAL: --outputFile '%s' cannot be used without the HDF5 library\n", OutputFilePath.c_str());
				exit(1);
			}
		}

		if(!BuildGmmIndex)
		{
			assert(XnPos != -1);
			assert(YnPos != -1);
			assert(TnPos != -1);
			assert(Grids.size() > 0);
		}
		else
		{
			if(!HAVE_YAEL)
			{
				fprintf(stderr, "FATAL: --buildGmmIndex cannot be used without the YAEL library\n");
				exit(1);
			}
		}

		assert(Parts.size() > 0);
		assert(GmmK > 0);
		for(int i = 0; i < Parts.size(); i++)
		{
			if(!BuildGmmIndex)
			{
				AssertFileExists(Parts[i].VocabPath, "gmm mu");
				AssertFileExists(GmmVocab::GetCovariancesFilePath(Parts[i].VocabPath), "gmm sigma");
				AssertFileExists(GmmVocab::GetWeightsFilePath(Parts[i].VocabPath), "gmm weights");
			}
		}
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
