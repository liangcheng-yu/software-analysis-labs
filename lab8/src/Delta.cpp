#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <string.h>

#include "Utils.h"

// A faithful implementation based on the algorithm's pseudocode
std::string DD(std::string &Target, std::string &Input, int K) {
  if(K <= Input.length()) {
    std::vector<std::string> Deltas;
    std::vector<std::string> Nablas;
    // Divide Input into K roughly equal parts Delta1, Delta2...
    // Meanwhile, store their complements Nabla1, Nabla2...
    if(K==1) {
      Deltas.push_back(Input);
      Nablas.push_back(""); 
    } 
    // If K>=2
    else {
      int DeltaLen = Input.length()/K;
      // The last segment could be special if Input.length()% K !=0
      for(int i = 0; i < K-1; i++) {
        Deltas.push_back(Input.substr(i*DeltaLen, DeltaLen));
        if(i==0) {
          // Delta is the first segment
          Nablas.push_back(Input.substr(DeltaLen));
        }else {
          Nablas.push_back(Input.substr((i-1)*DeltaLen, DeltaLen)+Input.substr((i+1)*DeltaLen));  
	}
      }
      // Don't forget the last Delta/Nabla
      Deltas.push_back(Input.substr((K-1)*DeltaLen));
      Nablas.push_back(Input.substr(0, (K-1)*DeltaLen));
    }
    // Now check each Delta and Nabla, see if they still crash the Target
    for(auto Delta : Deltas) {
      if(runTarget(Target, Delta)) {
        return DD(Target, Delta, 2); 
      }
    }
    for(auto Nabla : Nablas) {
      if(runTarget(Target, Nabla)) {
        return DD(Target, Nabla, K-1); 
      }
    }
    // If no deltas or nablas crash the Target
    return DD(Target, Input, 2*K);
  } else {
    // In this case, K > Input.length()
    // If we still try to partition it, we will get empty string and a bunch of single chars
    return Input;
  }
}

/*
 * Implement the delta-debugging algorithm (1-minimal minimization algorithm) to shrink Input that causes the program to crash
 * and find a 1-minimal input still crashes the input program.
 */
std::string delta(std::string &Target, std::string &Input) {
  std::string EmptyStr = "";
  if(runTarget(Target, EmptyStr)) {
    // As a corner case, if empty string crash the program (i.e., the program always crashes)
    // then the empty string must be 1-minimal	  
    return EmptyStr;
  }
  return DD(Target, Input, 2);
}

// ./delta [exe file] [crashing input file]
int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Invalid usage\n");
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

  std::string Target(argv[1]);
  std::string InputFile(argv[2]);
  std::string Input = readOneFile(InputFile);
  if (!runTarget(Target, Input)) {
    fprintf(stderr, "Sanity check failed: the program does not crash with the "
                    "initial input\n");
    return 1;
  }

  std::string DeltaOutput = delta(Target, Input);

  std::string Path = InputFile + ".delta";
  std::ofstream OutFile(Path);
  OutFile << DeltaOutput;
  OutFile.close();

  return 0;
}
