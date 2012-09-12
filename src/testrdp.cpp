#include <iostream>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include "StringTokenizer.h"

using namespace std;

string src = "";

string flags = "-t -u";

bool inFailList(int s){
    StringTokenizer strtok = StringTokenizer(src,",");
    while(strtok.hasMoreTokens()){
        string cur = strtok.nextToken();
        int c = atoi(cur.c_str());
        if(c==s) return true;
    }
    return false;
}

int main(int argc, char* argv[]){
    bool failed = false;
    int failNum = 0;
    int cases = atoi(argv[1]);
    string phpIn = argv[2];
    //system("make install");

    ifstream inFile;
	inFile.open(phpIn.c_str(),ios::in);
	if(!inFile){
		cerr << "ERR: Could Not Open Fail List\n";
		return false;
	}else{
		cout << "Opened Fail List\n";
	}

	// Parse PHP Source File to String
	string tmpStr;
	while(!inFile.eof()){
		getline(inFile,tmpStr);
		src += tmpStr;
	}
	inFile.close();
    cout << "Fail List: " << src << endl;
    for(int i=1;i<=cases;i++){
        stringstream stream;
        cout << "\33[36m Testing Case " << i << "\33[0m " << endl;
        stream << "./aophp test/testcase_" << i << ".aophp " << flags;
        string s = stream.str();
        int x = system(s.c_str());
        if(x==0)
          if(inFailList(i))
            cout << "\33[31m [FAIL] \33[0m Test " << i << " Passed, But should have Failed" << endl;
          else
            cout << "\33[32m [PASS] \33[0m Test " << i << " Passed" << endl;
        else{
          if(inFailList(i))
            cout << "\33[32m [PASS] \33[0m Test " << i << " Failed, But was Expected" << endl;
          else
            cout << "\33[31m [FAIL] \33[0m Test " << i << " Failed" << endl;
        }
    }
    return 1;
}
