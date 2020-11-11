#include <cstdlib>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include <cstring>
#include <cstdio>

#include "Utils.h"

// Uncomment to show debug messages
// #define DEBUG
// Though some cost to check duplicates, yet the unique number of mutants tried in fixed time is larger during measurement
#define AVOID_DUPLICATE_MUTANT

std::vector<std::string> SeedInputs;
std::vector<std::string> PastCoverage;
#include <unordered_map>
#define REPORT_PERIOD 1000
std::unordered_map<std::string, int> PastMutantMemo;
int NumDuplicateMutant = 0;
int NumTriedMutant = 0;

/**
 * Select a seed from SeedInputs
 */
std::string selectInput() {
  // Select a random seed from SeedInputs as the candidate
  // Note, for this algorithm, once a seed is added, we never remove it so we can't just use the back
  int Index = rand()%SeedInputs.size();
  return SeedInputs[Index];
}

/*********************************************/
/*  Mutation algorithms	 */
/*********************************************/

#define NUM_MUTATION_METHODS 5 
#define ASCII_NUM 256

/**
 * 1: Replace bytes with random values
 */
std::string mutateReplace(std::string Origin) {
  // For more effective mutation, replace all bytes (rather than just a single random byte)
  for(int Index=0; Index<Origin.size(); Index++) {
    // Randomly generate a char different from the original
    int RndChar = rand()%ASCII_NUM;
    while(RndChar==Origin[Index]) {
      RndChar = rand()%ASCII_NUM;
    }
    Origin[Index] = RndChar;
  }
  return Origin;
}

/**
 * 2: Swap adjacent bytes
 */
std::string mutateSwapAdjacent(std::string Origin) {
  for(int Times=0; Times<Origin.size(); Times++) {
    // Randomly pick a swap point
    int Index = rand()%Origin.size();
    if(Index==Origin.size()-1) {
      char tmp = Origin[Index];
      Origin[Index] = Origin[0];
      Origin[0] = tmp; 
    } else {
      char tmp = Origin[Index];
      Origin[Index] = Origin[Index+1];
      Origin[Index+1] = tmp;
    }
  }
  return Origin;  
}

/**
 * 3: Cycle through all values of each byte
 */
std::string mutateCycle(std::string Origin) {
  int Shift = rand()%(Origin.size()-1)+1;
  // Substring from Shift to end
  std::string RetString = Origin.substr(Shift);
  // Substring from start to Shift
  RetString += Origin.substr(0, Shift);
  return RetString;
}

/**
 * 4: Remove a random byte
 */
std::string mutateRemove(std::string Origin) {
  // Choose a random index
  Origin.erase(Origin.begin()+rand()%Origin.size());
  return Origin;
}

/**
 * 5: Insert a random byte
 */
std::string mutateInsert(std::string Origin) {
  // Randomly choose an index to insert, note we could also insert at idex Origin.size() (Origin.size()+1 options)
  int Index = rand()%(Origin.size()+1);
  // Randomly generate a char
  auto RndChar = rand()%ASCII_NUM;
  Origin.insert(Index, 1, RndChar);
  return Origin;
}

/**
 * Given string Origin, return a mutate string.
 */
std::string mutate(std::string Origin) {
  int opt = rand() % NUM_MUTATION_METHODS;
  // Make sure the mutant hasn't beed tried before
  if(opt==0) {
    Origin = mutateReplace(Origin);
  } else if(opt==1) {
    Origin = mutateSwapAdjacent(Origin);
  } else if(opt==2) {
    Origin = mutateCycle(Origin);
  } else if(opt==3) {
    Origin = mutateRemoveMultiple(Origin);
  } else if(opt==4) {
    Origin = mutateInsertMultiple(Origin);
  } 
#ifdef AVOID_DUPLICATE_MUTANT
  while(PastMutantMemo.find(Origin)!=PastMutantMemo.end()) {
    NumDuplicateMutant += 1;
    opt = rand() % NUM_MUTATION_METHODS;
    if(opt==0) {
      Origin = mutateReplace(Origin);
    } else if(opt==1) {
      Origin = mutateSwapAdjacent(Origin);
    } else if(opt==2) {
      Origin = mutateCycle(Origin);
    } else if(opt==3) {
      Origin = mutateRemoveMultiple(Origin);
    } else if(opt==4) {
      Origin = mutateInsertMultiple(Origin);
    }
  }
  PastMutantMemo.insert({Origin, 0});
#endif
  return Origin;
}

/*********************************************/
/* 		Implement your feedback algorithm	 */
/*********************************************/

void feedBack(std::string &Target, std::string &Mutated) {
  std::string CovPath = Target+".cov";
  std::ifstream CovFile(CovPath);
  std::string Line;
  bool NewCoverage = false;
  if(CovFile.is_open()) {
    while(std::getline(CovFile,Line)) {
      if(std::find(PastCoverage.begin(), PastCoverage.end(), Line)==PastCoverage.end()) {
        PastCoverage.push_back(Line);
#ifdef DEBUG
	std::cout << "[DEBUG-cov] " << Line << std::endl;
#endif
	NewCoverage = true;
      }
    }
    CovFile.close();
  }
  if(NewCoverage) {
#ifdef DEBUG
    std::cout << "[DEBUG-SeedInputs size before add] " << SeedInputs.size() << std::endl; 
#endif
    SeedInputs.push_back(Mutated);
  } else {
    if(rand()%1000<1) {
      SeedInputs.push_back(Mutated); 
    }
  }
}

/*****************************************************************/
//helper functions 

int readSeedInputs(std::string &SeedInputDir) {
  DIR *Directory;
  struct dirent *Ent;
  if ((Directory = opendir(SeedInputDir.c_str())) != NULL) {
    while ((Ent = readdir(Directory)) != NULL) {
      if (!(Ent->d_type == DT_REG))
        continue;
      std::string Path = SeedInputDir + "/" + std::string(Ent->d_name);
      std::string Line = readOneFile(Path);
      SeedInputs.push_back(Line);
    }
    closedir(Directory);
    return 0;
  } else {
    return 1;
  }
}

int Freq = 1000;
int Count = 0;

bool test(std::string &Target, std::string &Input, std::string &OutDir) {
  // Clean up old coverage file before running 
  std::string CoveragePath = Target + ".cov";
  std::remove(CoveragePath.c_str());

  Count++;
  int ReturnCode = runTarget(Target, Input);
  switch (ReturnCode) {
  case 0:
    if (Count % Freq == 0)
      storePassingInput(Input, OutDir);
    return true;
  case 256:
    fprintf(stderr, "%d crashes found\n", failureCount);
    storeCrashingInput(Input, OutDir);
    return false;
  case 127:
    fprintf(stderr, "%s not found\n", Target.c_str());
    exit(1);
  }
}

void storeSeed(std::string &OutDir, int randomSeed) {
  std::string Path = OutDir + "/randomSeed.txt";
  std::fstream File(Path, std::fstream::out | std::ios_base::trunc);
  File << std::to_string(randomSeed);
  File.close();
}

int main(int argc, char **argv) { 
  if (argc < 4) { 
    printf("usage %s [exe file] [seed input dir] [output dir] [seed (optional arg)]\n", argv[0]);
    return 1;
  }

  struct stat Buffer;
  if (stat(argv[1], &Buffer)) {
    fprintf(stderr, "%s not found\n", argv[1]);
    return 1;
  }

  if (stat(argv[2], &Buffer)) {
    fprintf(stderr, "%s not found\n", argv[2]);
    return 1;
  }

  if (stat(argv[3], &Buffer)) {
    fprintf(stderr, "%s not found\n", argv[3]);
    return 1;
  }

  int randomSeed = (int)time(NULL);
  if (argc > 4) {
    randomSeed = strtol(argv[4], NULL, 10);
  }
  srand(randomSeed);

  std::string Target(argv[1]);
  std::string SeedInputDir(argv[2]);
  std::string OutDir(argv[3]);
  
  storeSeed(OutDir, randomSeed);
  
  initialize(OutDir);

  if (readSeedInputs(SeedInputDir)) {
    fprintf(stderr, "Cannot read seed input directory\n");
    return 1;
  }

  while (true) {
      NumTriedMutant += 1;
      std::string SC = selectInput();
      auto Mutant = mutate(SC);
      test(Target, Mutant, OutDir);
      feedBack(Target, Mutant);
#ifdef DEBUG
      if(NumTriedMutant%REPORT_PERIOD==0) {
        std::cout << "[DEBUG] " << "Skipped " << NumDuplicateMutant << ", memo size " << PastMutantMemo.size() << ", #loops " << NumTriedMutant << ", #seed " << SeedInputs.size() << std::endl; 
      }
#endif
  }
  return 0;
}
