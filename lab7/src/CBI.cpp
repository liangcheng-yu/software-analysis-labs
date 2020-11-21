#include <cstdlib>
#include <string>
#include <unistd.h>

#include "Utils.h"

/*
 * Implement your CBI report generator.
 */
void generateReport() {
  ////////////////////////////////////////////////
  // Read files with name lists in SuccessLogs
  for (auto success : SuccessLogs) {
    // For each file, parse each line with format type,line,col,val
    std::string line;
    std::ifstream logfile (success);
    // We only care about the number of runs for each predicate, so we will count maximum once for each P
    std::set<std::tuple<int, int, State>> predicatesSet;
    if(logfile.is_open()) {
      while(getline(logfile, line)) {
	// Parse type, line, col, val
	std::string delimiter = ",";
	size_t pos = 0;
	std::vector<std::string> tokens;
	std::string token;
        while ((pos = line.find(delimiter)) != std::string::npos) {
          token = line.substr(0, pos);
	  tokens.push_back(token);
          line.erase(0, pos + delimiter.length());
        }
	// Don't forget the last component
	tokens.push_back(line);
	std::string type = tokens[0];
	std::string line = tokens[1];
	std::string col = tokens[2];
	std::string valStr = tokens[3];
	int valInt = atoi(valStr.c_str());
	int lineInt = atoi(line.c_str());
	int colInt = atoi(col.c_str());
	// Figure out the corresponding state
	State statetmp;
        if(type.compare("branch")==0) {
          if(valInt==1) {
	    statetmp = State::BranchTrue;
	  } else if(valInt==0) {
	    statetmp = State::BranchFalse;
	  } else {
//	    std::cout << "Unsupported branch value, not 0 or 1" << std::endl; 
	  }
	}
	else if(type.compare("return")==0) {
          if(valInt==0) {
	    statetmp = State::ReturnZero; 
	  } else if(valInt>0) {
	    statetmp = State::ReturnPos;
	  } else {
	    statetmp = State::ReturnNeg; 
	  }
	}
        // Construct the predicate tuple and append to predicateSet
        auto key = std::make_tuple(lineInt, colInt, statetmp);
	predicatesSet.insert(key);
      }
      logfile.close();
    }
    // Now, update S, SObs
    for(auto predicate : predicatesSet) {
      State predState = std::get<2>(predicate);
      int predLine = std::get<0>(predicate);
      int predCol = std::get<1>(predicate);
      
      if(predState==State::BranchTrue || predState==State::BranchFalse) {
	auto key1 = std::make_tuple(predLine, predCol, State::BranchTrue);
	auto key2 = std::make_tuple(predLine, predCol, State::BranchFalse);
        // For S, first make sure the entries are both there
	if(S.find(key1)==S.end()) {
	  S[key1] = 0;
	}
	if(S.find(key2)==S.end()) {
	  S[key2] = 0;
	}
	if(F.find(key1)==F.end()) {
	  F[key1] = 0;
	}
	if(F.find(key2)==F.end()) {
	  F[key2] = 0;
	}
	// Then increment the runs count for the true predicate
	S[predicate] += 1;
	// Same for SObs
	if(SObs.find(key1)==SObs.end()) {
	  SObs[key1] = 0;
	}
	if(SObs.find(key2)==SObs.end()) {
	  SObs[key2] = 0;
	}
	if(FObs.find(key1)==FObs.end()) {
	  FObs[key1] = 0;
	}
	if(FObs.find(key2)==FObs.end()) {
	  FObs[key2] = 0;
	}
	// For SObs, we need to increment BranchTrue, BranchFalse if branch, and ReturnNeg, ReturnZero, ReturnPos if return
	SObs[key1] += 1;
        SObs[key2] += 1;	
      } else if(predState==State::ReturnNeg || predState==State::ReturnZero || predState==State::ReturnPos) {
        auto key1 = std::make_tuple(predLine, predCol, State::ReturnNeg);
        auto key2 = std::make_tuple(predLine, predCol, State::ReturnZero);
        auto key3 = std::make_tuple(predLine, predCol, State::ReturnPos);
	if(S.find(key1)==S.end()) {
	  S[key1] = 0;
	}
	if(S.find(key2)==S.end()) {
	  S[key2] = 0;
	}
	if(S.find(key3)==S.end()) {
	  S[key3] = 0;
	}
 	if(F.find(key1)==F.end()) {
	  F[key1] = 0;
	}
	if(F.find(key2)==F.end()) {
	  F[key2] = 0;
	}
	if(F.find(key3)==F.end()) {
	  F[key3] = 0;
	}

	// Then increment the runs count for the true predicate
	S[predicate] += 1;

	if(SObs.find(key1)==SObs.end()) {
	  SObs[key1] = 0;
	}
	if(SObs.find(key2)==SObs.end()) {
	  SObs[key2] = 0;
	}
	if(SObs.find(key3)==SObs.end()) {
	  SObs[key3] = 0;
	}
 	if(FObs.find(key1)==FObs.end()) {
	  FObs[key1] = 0;
	}
	if(FObs.find(key2)==FObs.end()) {
	  FObs[key2] = 0;
	}
	if(FObs.find(key3)==FObs.end()) {
	  FObs[key3] = 0;
	}

	SObs[key1] += 1;
	SObs[key2] += 1;
	SObs[key3] += 1;
      }
    }
  }
  //////////////////////////////////////////////////////
  // Read files with name lists in FailureLogs
  for (auto failure : FailureLogs) {
    // For each file, parse each line with format type,line,col,val
    std::string line;
    std::ifstream logfile (failure);
    std::set<std::tuple<int, int, State>> predicatesSet;
    if(logfile.is_open()) {
      while(getline(logfile, line)) {
	// Parse type, line, col, val
	std::string delimiter = ",";
	size_t pos = 0;
	std::vector<std::string> tokens;
	std::string token;
        while ((pos = line.find(delimiter)) != std::string::npos) {
          token = line.substr(0, pos);
	  tokens.push_back(token);
          line.erase(0, pos + delimiter.length());
        }
	// Don't forget the last component
	tokens.push_back(line);
	std::string type = tokens[0];
	std::string line = tokens[1];
	std::string col = tokens[2];
	std::string valStr = tokens[3];
	int valInt = atoi(valStr.c_str());
	int lineInt = atoi(line.c_str());
	int colInt = atoi(col.c_str());

	// Figure out the corresponding state
	State statetmp;
        if(type.compare("branch")==0) {
          if(valInt==1) {
	    statetmp = State::BranchTrue;
	  } else if(valInt==0) {
	    statetmp = State::BranchFalse;
	  } else {
	    std::cout << "Unsupported branch value, not 0 or 1" << std::endl; 
	  }
	}
	else if(type.compare("return")==0) {
          if(valInt==0) {
	    statetmp = State::ReturnZero; 
	  } else if(valInt>0) {
	    statetmp = State::ReturnPos;
	  } else {
	    statetmp = State::ReturnNeg; 
	  }
	}
        // Construct the predicate tuple and append to predicateSet
        auto key = std::make_tuple(lineInt, colInt, statetmp);
	predicatesSet.insert(key);
      }
      logfile.close();
    }
    // Now, update F, FObs
    for(auto predicate : predicatesSet) {
      // For FObs, we need to increment BranchTrue, BranchFalse if branch, and ReturnNeg, ReturnZero, ReturnPos if return
      State predState = std::get<2>(predicate);
      int predLine = std::get<0>(predicate);
      int predCol = std::get<1>(predicate);
      if(predState==State::BranchTrue || predState==State::BranchFalse) {
	auto key1 = std::make_tuple(predLine, predCol, State::BranchTrue);
	auto key2 = std::make_tuple(predLine, predCol, State::BranchFalse);
 	if(S.find(key1)==S.end()) {
	  S[key1] = 0;
	}
	if(S.find(key2)==S.end()) {
	  S[key2] = 0;
	}
	if(F.find(key1)==F.end()) {
	  F[key1] = 0;
	}
	if(F.find(key2)==F.end()) {
	  F[key2] = 0;
	}       
        F[predicate] += 1;

	if(SObs.find(key1)==SObs.end()) {
	  SObs[key1] = 0;
	}
	if(SObs.find(key2)==SObs.end()) {
	  SObs[key2] = 0;
	}
	if(FObs.find(key1)==FObs.end()) {
	  FObs[key1] = 0;
	}
	if(FObs.find(key2)==FObs.end()) {
	  FObs[key2] = 0;
	}
	FObs[key1] += 1;
        FObs[key2] += 1;	
      } else if(predState==State::ReturnNeg || predState==State::ReturnZero || predState==State::ReturnPos) {
        auto key1 = std::make_tuple(predLine, predCol, State::ReturnNeg);
        auto key2 = std::make_tuple(predLine, predCol, State::ReturnZero);
        auto key3 = std::make_tuple(predLine, predCol, State::ReturnPos);

 	if(S.find(key1)==S.end()) {
	  S[key1] = 0;
	}
	if(S.find(key2)==S.end()) {
	  S[key2] = 0;
	}
	if(S.find(key3)==S.end()) {
	  S[key3] = 0;
	}
 	if(F.find(key1)==F.end()) {
	  F[key1] = 0;
	}
	if(F.find(key2)==F.end()) {
	  F[key2] = 0;
	}
	if(F.find(key3)==F.end()) {
	  F[key3] = 0;
	}
        F[predicate] += 1;

	if(SObs.find(key1)==SObs.end()) {
	  SObs[key1] = 0;
	}
	if(SObs.find(key2)==SObs.end()) {
	  SObs[key2] = 0;
	}
	if(SObs.find(key3)==SObs.end()) {
	  SObs[key3] = 0;
	}
 	if(FObs.find(key1)==FObs.end()) {
	  FObs[key1] = 0;
	}
	if(FObs.find(key2)==FObs.end()) {
	  FObs[key2] = 0;
	}
	if(FObs.find(key3)==FObs.end()) {
	  FObs[key3] = 0;
	}
	FObs[key1] += 1;
	FObs[key2] += 1;
        FObs[key3] += 1;	
      }
    }
  }
  /////////////////////////////////////////////////////
  // Now, populate Failure, Context, and Increase based on F, S, FObs, SObs
  // Failure
  for(auto item : F) {
    double fp = F[item.first];
    double sp = 0.0;
    if(S.find(item.first)!=S.end()) {
      sp = S[item.first]; 
    }
    double failure = 0.0;
    if(fp+sp!=0) {
      failure = fp/(fp+sp);
    } else {
      // As per piazza post 186, Increase should also be 0.0
      Increase[item.first] = 0.0;
    }
    Failure[item.first] = failure;
  }
  // Context
  for(auto item : FObs) {
    double fp = FObs[item.first];
    double sp = 0.0;
    if(SObs.find(item.first)!=SObs.end()) {
      sp = SObs[item.first];
    }
    double context = 0.0;
    if(fp+sp!=0) {
      context = fp/(fp+sp);
    } else {
      Increase[item.first] = 0.0;
    }
    Context[item.first] = context;
  }
  // Increase
  for(auto item : Failure) {
    if(Increase.find(item.first)!=Increase.end()) {
      continue;
    }
    double failure = Failure[item.first];
    double context = Context[item.first];
    double increase = failure - context;
    Increase[item.first] = increase; 
  }
}

// ./CBI [exe file] [fuzzer output dir]
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
  std::string OutDir(argv[2]);

  generateLogFiles(Target, OutDir);
  generateReport();
  printReport();
  return 0;
}
