/***

Aspect-Oriented PHP v4(aophpv4) is an Extension for PHP that allows the user Aspect-Oriented Programming Techniques
Copyright (C) 2006 Bryan Saunders.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

aophp, Copyright (C) 2006 Bryan Saunders, John Stamey
aophp comes with ABSOLUTELY NO WARRANTY; This is free software, and you are welcome
to redistribute it under certain conditions.

Command Line Flags
    -s [0|1] = Include Status 0-Off 1-On
    -l [0-5] = Include Level
    -d = Enable aophpDoc
    -h "#XXXXXX" = aophpDoc HTML Color
    -p "#XXXXXX" = aophpDoc AOPHP Color
    -m "/Path/To/File/" = Meta Data Logging
    -x = Debug Mode on
    -t = Test Mode on
    -u = Test Mode Debug on

To Do List
    - Properly Remove aophp_init function call
    - Add file Check to Advice Weaving
    - Color Coded Doc Output
    - Format Output Code
    - Cleanup Console Output


***/

#include <iostream>
#include <exception>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <stdexcept>
#include <vector>
#include "StringTokenizer.h"
#include "md5.h"
#include <boost/xpressive/xpressive_dynamic.hpp>

using namespace std;
using namespace boost::xpressive;

/*** Setup Variables ***/
const int MAX_CHARS_LINE = 500;
const int ARG_CNT = 2;

/*** Status Codes ***/
const int COMPLETED = 0;
const int DECLINED = 1;
const int SRC_FILE_ERROR = 2;
const int INV_ASPECT_FILE = 3;
const int INV_ASPECT_ARGS = 4;
const int ASPECT_FILE_ERROR = 5;
const int INVALID_PC_DEC = 6;
const int INVALID_PARAM = 7;
const int CMD_ARG_ERROR = 8;

/*** Arg Variables ***/
string DOC_HTML = "000000";
string DOC_PHP = "ff0000";
string META_LOG = "";
int INC_STATUS = 0; // Off by Default
int INC_DEPTH = 1; // 1 Level by Default
bool printDoc = false;
bool debug = false;
bool testMode = false;
bool testModeDebug = false;

/*** Global Data ***/
string phpIn;
string phpOut;
string phpFileName;
string phpFileDir;
string phpSource = "";
string aophpSource = "";

/*** Argument Node ***/
struct argNode
{
    string arg;
    string sval;
    int ival;
};

/*** Joinpoint Node ***/
struct jpNode
{
    string type;
    string file;
    string sig;
    vector<string> paramList;
    vector<argNode> args;
};

/*** Pointcut Node ***/
struct pcNode
{
    string name;
    vector<jpNode> jp;
};

/*** Advice Node ***/
struct advNode
{
    string type;
    vector<string> paramList;
    jpNode jp;
    string code;
};

/*** Abstract Syntax Table Node ***/
struct astNode
{
    int type;
    string val;
};

/*** Function Node ***/
struct funcNode
{
    string str; // Required
    string hash;
};

/*** Literal Node ***/
struct litNode
{
    string str;
    string hash;
};

/*** Variable Call Node ***/
struct varNode
{
    int type;
    string hash;
    string str;
    string val;
};

/*** Set Match Node ***/
struct setNode
{
    string match;
    string hash;
    string val;
};

/*** Target Node ***/
struct tgtNode
{
    jpNode jp;
    bool hasBefore;
    bool hasAfter;
    bool hasAround;
    advNode before;
    advNode after;
    advNode around;
};

/*** AST Type Codes ***/
const int CONST = 1; // Constant
const int LC = 2; // {
const int RC = 3; // }
const int EQUAL = 4; // =
const int SEMIC = 5; // ;
const int RP = 6; // )
const int LP = 7; // (
const int PIPE = 8; // |
const int AND = 9; // &&
const int QUOTE = 10; // "
const int COLON = 11; // :
const int DOLLAR = 12; // $
const int COMA = 13; // ,
const int DOT = 14; // .
const int SDELIM = 15; // {%
const int EDELIM = 16; // %}
const int FJPTYPE = 17; // "exec" "execr"
const int VJPTYPE = 18; // "set" "get"
const int ADVTYPE = 19; // "before" "after" "around"
const int ID = 20;
const int DIGIT = 21;
const int CODE = 22;
const int STRING = 23;
const int ADVICE = 24;
const int POINTCUT = 25;
const int ASPECT = 26;

/*** Recursive Descent Parser Variables ***/
int loc = 0;

/*** RDP Function Prototypes ***/
void error(string);
string lookToken();
string lookToken(int);
string lookTokenTo(string);
string nextToken();
string nextTokenWS();
void checkToken(string,int); // Expected String
void checkToken(int,string,int); // Regex Check Num, Rule Name
void checkToken(string[],int,int); // Possible Expecteds, Array Size
void parse(); // Start Parsing
void parseAspect();
void parseAspect_plus();
void parsePointcut();
void parsePointcut_star();
void parseAdvice();
void parseAdvice_star();
void parseJoinpoint();
void parseFJoinpoint();
void parseVJoinpoint();
void parseJoinpointlist_star();
void parseJoinpointlist();
void parseArgs();
void parseFJPType();
void parseVJPType();
void parseSignature();
void parseVarList_star();
void parseParamlist();
void parseArgVal();
void parseArgs_star();
void parseID();
void parseVar();
void parseVarList();
void parseDelim();
void parseEndDelim();
void parseNonDelim();
void parseAdviceType();
void parseCodeBlock();
bool re_1_ID(string); // Regex Check Function - Check Num 1 - Rule T
bool re_2_ArgVal(string);
bool re_3_NonDelim(string);
bool re_4_Digit(string);

/*** AST Variables ***/
int node = 0;
vector<astNode> ast;

/*** AST Function Prototypes ***/
void addAST(int,string);
void printAST();
astNode nextNode();
int lookNode();
int lookNode(int);
bool hasNodes();
void parseAST();
jpNode parseAST_Joinpoint();
void parseAST_Advice();
pcNode parseAST_Pointcut();
argNode parseAST_Argument();
string checkNode(int,string);
void printStats();
void printAdvice();
void printPointcuts();
vector<jpNode> pcsSearch(string);

/*** Code Generation Variables***/
vector<pcNode> pcs;
vector<advNode> adv;
vector<litNode> litList;
vector<funcNode> funcList;
vector<varNode> varList;
vector<string> tgtVars;
vector<tgtNode> tgts;
vector<setNode> setList;
string aophpFuncs;

/*** Code Generation Prototypes ***/
void genTargetList();
void printTargetList();
int isTarget(advNode,jpNode);
bool isTargetVar(string);
string buildFHelper(advNode,jpNode,string);
string buildVHelper(advNode,jpNode,string);
string processTargets();
string buildFMain(tgtNode,advNode);
string buildVMain(tgtNode,advNode);
void weave();
void weaveFuncs();
void weaveSet();
void weaveGet();
void stripSet(string&);
void stripGet(string&);
varNode getVarNode(string);

/*** Processing Prototypes ***/
void writeOutFile(string oFile, string content);
bool open_parse_PHP();
bool parse_args(int argc, char* argv[]);
void pre_process_aspects(string&);
void pre_process_php(string&);
void post_process_php(string&);
string find_aspect_files(string&);
bool parse_aspects(string); // Not Complete
int countChar(string,string);
void printArgs();
string itos(int);

/*** String Functions ***/
string subStrSect(string&,int,int);
string grabCode(string&,int);
int findCodeEnd(string&,int);
int findCodeStart(string&,int);
void stripComments(string&);
void stripLiterals(string&); // Not Complete
void stripFunctions(string&);
void insertFunctions(string&);
void insertLiterals(string&);
void removeInitCall(string&);
void trim(string& str);
void replace(string,string,string&);
int findString(int,string,string);
int findActualString(string&,int,string);
void replaceString(string&,string,string);

/*** Main Function ***/
int main(int argc, char* argv[])
{
    // Parse Command Line Arguments
    if(!parse_args(argc,argv))
        return CMD_ARG_ERROR;
    // Test Mode
    if(testMode)
    {
        parse_aspects(phpIn);
        pre_process_aspects(aophpSource);
        parse();
        //printAST();
        parseAST();
        if(testModeDebug)
            printStats();
        if(testModeDebug)
            printPointcuts();
        if(testModeDebug)
            printAdvice();
        return 0;
    }
    // Print Command Arguments
    printArgs();
    // Open and Parse PHP File
    if(!open_parse_PHP())
    {
        cout << "Error Reading Input Source File." << endl;
        // Write out original file
        return SRC_FILE_ERROR;
    }
    // Find Aspect Files, then Parse to get input
    parse_aspects(find_aspect_files(phpSource));
    // PreProcess PHP Source - Strip Comments, Literals, & Functions
    pre_process_php(phpSource);
    if(debug)
        cout << "PHP Code After Pre-Process:\n-----------\n" << phpSource << endl;
    // PreProcess aophp Source - Strip Comments, New Lines,  & Literals
    pre_process_aspects(aophpSource);
    // Recusrive Descent Parsing
    if(debug)
        cout << "Parsing: " << aophpSource << "\n";
    parse();
    parseAST();
    // Code Generation
    if(debug)
        printStats();
    if(debug)
        printPointcuts();
    if(debug)
        printAdvice();

    genTargetList();
    printTargetList();

    aophpFuncs = processTargets();
    weave();
    post_process_php(phpSource);

    if(debug)
        cout << "Weaved Source:\n--------------------\n" << phpSource << "\n--------------------------\n";

    aophpSource = "<?\n"+aophpFuncs+"?>\n\n"+phpSource;
    cout << "AOPHP Source:\n--------------------\n" << aophpSource << "\n--------------------------\n";
    cout << "Weave Complete.\n";

    writeOutFile(phpOut,aophpSource);
    cout << "File Write Complete.\n";

    exit(0); /*** HACK - to defeat glibc detected, corrupted linked list error ***/
    return COMPLETED;
}
bool open_parse_PHP()
{
    int s = phpIn.find_last_of('/')+1;
    int l = phpIn.find_first_of('.') - s;
    phpFileName = phpIn.substr(s,l);
    phpFileDir = phpIn.substr(0,s);
    if(debug)
        cout << "Paths: " << phpFileName << " - " << phpFileDir << endl;
    // Open PHP Source Input File
    ifstream inFile;
    inFile.open(phpIn.c_str(),ios::in);
    if(!inFile)
    {
        cerr << "ERR: Could Not Source Open File\n";
        return false;
    }
    else
    {
        cout << "Opened Source File\n";
    }

    // Parse PHP Source File to String
    string tmpStr;
    string src;
    while(!inFile.eof())
    {
        getline(inFile,tmpStr);
        src += tmpStr + "\n";
    }
    inFile.close();
    if(debug)
        cout << endl << src << endl;
    cout << "PHP Source Parsed\n";
    trim(src);
    phpSource = src;
    return true;
}
bool parse_aspects(string list)
{
    StringTokenizer strtok = StringTokenizer(list,",");
    while(strtok.hasMoreTokens())
    {
        string curfile = strtok.nextToken();
        ifstream inFile;
        int k = curfile.find(".aophp");
        if(k < 0)
        {
            continue;
        }
        string tempstring = phpFileDir+curfile;
        inFile.open(tempstring.c_str(),ios::in);
        if(!inFile)
        {
            return false;
        }
        else
        {
            string tmpStr = "";
            while(!inFile.eof())
            {
                getline(inFile,tmpStr);
                aophpSource += tmpStr;
            }
            inFile.close();
        }
    }
    if(debug)
        cout << "-->|" << aophpSource << "|<--" << endl;
    return true;
}
string find_aspect_files(string& phpcode)
{
    sregex ws = sregex::compile("[\\s]*"); // Remove Whitespace
    string format(""); // Empty String Format
    sregex token = sregex::compile("(aophp_init\\(\")([A-Za-z0-9.,_-]*)(\"\\);)");
    sregex_iterator cur( phpcode.begin(), phpcode.end(), token );
    sregex_iterator end;
    string files; // aophp File List
    string m;
    int fCnt = 0;
    bool matched = false;
    for( ; cur != end; ++cur )
    {
        // Get Current Match
        smatch const &what = *cur;
        // Remove Whitespace
        files = regex_replace(what[2].str(),ws,format);
        m = what[0].str();
        if(files.compare("")!=0)
        {
            matched = true;
        }
        break;
    }
    sregex match = sregex::compile(m);
    phpcode = regex_replace(phpcode,match,format);
    return files;
}
void pre_process_aspects(string& code)
{
    stripComments(code);
    trim(code);
}
void pre_process_php(string& code)
{
    stripComments(code);
    stripLiterals(code);
    stripFunctions(code);
}
void post_process_php(string& code)
{
    insertFunctions(code);
    insertLiterals(code);
    //removeInitCall(code);
}
void writeOutFile(string oFile, string content)
{
    ofstream outFile;
    outFile.open(oFile.c_str(),ios::out);
    outFile << content;
    outFile.close();
}

/*** Parse Command Line Arguments ***/
bool parse_args(int argc, char* argv[])
{
    // Parse Aguments

    // Cmd Line Flags
    // -s [0|1] = Include Status 0-Off 1-On
    // -l [0-5] = Include Level
    // -d = Enable aophpDoc
    // -h "#XXXXXX" = aophpDoc HTML Color
    // -p "#XXXXXX" = aophpDoc AOPHP Color
    // -m "/Path/To/File/" = Meta Data Logging
    // -x = Debug Mode On
    // -t XX = Run in Test Mode, XX Num of Test Cases
    string argtemp = "";
    try
    {
        for(int i=0;i<argc;i++)
        {
            argtemp = argv[i];
            if(argtemp.compare("-s")==0)
            {
                i++;
                argtemp = argv[i];
                INC_STATUS = atoi(argv[i]);
            }
            else if(argtemp.compare("-t")==0)
            {
                testMode = true;
            }
            else if(argtemp.compare("-u")==0)
            {
                testModeDebug = true;
            }
            else if(argtemp.compare("-l")==0)
            {
                i++;
                argtemp = argv[i];
                INC_DEPTH = atoi(argv[i]);
            }
            else if(argtemp.compare("-h")==0)
            {
                i++;
                string htmldebugtemp(argv[i]);
                DOC_HTML = htmldebugtemp;
            }
            else if(argtemp.compare("-p")==0)
            {
                i++;
                string phpdebugtemp(argv[i]);
                DOC_PHP = phpdebugtemp;
            }
            else if(argtemp.compare("-d")==0)
            {
                printDoc = true;
            }
            else if(argtemp.compare("-x")==0)
            {
                debug = true;
            }
            else if(argtemp.compare("-m")==0)
            {
                i++;
                string metafiletemp(argv[i]);
                META_LOG = metafiletemp;
            }
            else
            {
                if(i==1)
                {
                    phpIn = argtemp;
                }
                else if(i==2)
                {
                    phpOut = argtemp;
                }
            }
        }
    }
    catch(std::logic_error&)
    {
        cerr << "Invalid Parameters. Failing" << endl;
        return false;
    }
    return true;
}
void printArgs()
{
    cout << "Arguments: " << endl;
    cout << "In File: " << phpIn << endl;
    cout << "Out File: " << phpOut << endl;
    cout << "Inc. Status: " << INC_STATUS << endl;
    cout << "Inc. Depth: " << INC_DEPTH << endl;
    cout << "Doc. HTML Color: " << DOC_HTML << endl;
    cout << "Doc. PHP Color: " << DOC_PHP << endl;
    cout << "Print Doc: " << printDoc << endl;
    cout << "Meta Log File: " << META_LOG << endl;
}

/*** String Functions ***/
void replace(string search, string replace,string& code)
{
    int l = search.length();
    int x = findString(0,search,code);
    int r = replace.length() + l + 1;

    while(x != -1 && x<code.length())
    {
        code.erase(x,l);
        code.insert(x,replace);
        x = findString(x+r,search,code);
    }
}

int findString(int s, string substring,string code)
{
    bool found = false;
    bool inString = false;

    //int l = code.length();
    int sl = substring.length();
    int r = -1;

    char c = substring.at(0);
    char z;
    char t;

    for(int i=s;i<code.length();i++)
    {
        t = code.at(i);
        if(t == '\\')
        {
            i++;
            continue;
        }
        else if(t == '"')
        {
            inString = !inString;
        }
        else if(t == '\'')
        {
            i+=2;
        }
        else
        {
            if(inString)
            {
                continue;
            }
            else
            {
                if(t == c)
                {
                    //Found, Lookahead
                    //cout << "CC: " << t << endl;
                    r = i;
                    found = true;
                    for(int k=0;k<sl;k++)
                    {
                        z = substring.at(k);
                        //cout << "IC: " << k << "-" << z << endl;
                        if(z == code.at(i+k))
                        {
                            found = true;
                        }
                        else
                        {
                            found = false;
                            break;
                        }
                    }
                    if(found)
                    {
                        //cout << "FOUND" << endl;
                        return r;
                        //cout << "AFR" << endl;
                    }
                    else
                    {
                        r = -1;
                    }
                }
            }
        }
    }
    return r;
}

void trim(string& str)
{
    string::size_type pos1 = str.find_first_not_of(" \t\r\n");
    string::size_type pos2 = str.find_last_not_of(" \t\r\n");
    str = str.substr(pos1 == string::npos ? 0 : pos1,
                     pos2 == string::npos ? str.length() - 1 : pos2 - pos1 + 1);
}
int countChar(string s, string ch)
{
    int c = 0;
    int l = s.length();
    for(int i=0;i<l;i++)
    {
        string t = s.substr(i,1);
        if(t.compare(ch)==0)
            c++;
    }
    return c;
}
string itos(int x)
{
    stringstream ss;
    ss << x;
    return ss.str();
}
int findCodeStart(string& code, int s)
{
    int l = code.length();
    bool inString = false;
    char t;
    for(int i=s;i<l;i++)
    {
        t = code.at(i);
        switch(t)
        {
            case '\\':
            i++;
            continue;
            break;
            case '"':
            inString = !inString;
            break;
            case '{':
            if(!inString)
            {
                return i;
            }//else Ignore It
            break;
            default:
            break;
        }
    }
    return string::npos; // Not Found
}
int findCodeEnd(string& code, int s)
{
    int l = code.length();
    bool inString = false;
    int cnt = 1;
    char t;
    for(int i=s+1;i<l;i++)
    {
        t = code.at(i);
        switch(t)
        {
            case '"':
            inString = !inString;
            break;
            case '{':
            if(!inString)
            {
                cnt++;
            }//else Ignore It
            break;
            case '}':
            if(!inString)
            {
                cnt--;
                if(cnt == 0)
                {
                    return i;
                }
            }//else Ignore It
            break;
            default:
            break;
        }
    }
    return string::npos; // Not Found
}
string grabCode(string& code, int s)
{
    stripComments(code);
    int x = findCodeStart(code,s);
    int y = findCodeEnd(code,x);
    return subStrSect(code,x+1,y-1);
}
string subStrSect(string& code,int s, int e)
{
    int d = e - s;
    string temp = "";
    temp = code.substr(s,d+1);
    return temp;
}
void replaceString(string& code, string search, string replace)
{
    int l = search.length();
    int x = findActualString(code,0,search);
    int r = replace.length() + l + 1;
    //int q = code.length();
    while(x != -1 && x<code.length())
    {
        code.erase(x,l);
        code.insert(x,replace);
        //cout << "REP_LOOP 1: " << x << endl;
        x = findActualString(code,x+r,search);
        //cout << "REP_LOOP 2: " << x << endl;
    }
}
int findActualString(string& code,int s,string substring)
{
    bool found = false;
    int sl = substring.length();
    int r = -1;

    char c = substring.at(0);
    char z;
    char t;

    for(int i=s;i<code.length();i++)
    {
        t = code.at(i);
        if(t == c)
        {
            //Found, Lookahead
            //cout << "CC: " << t << endl;
            r = i;
            found = true;
            for(int k=0;k<sl;k++)
            {
                z = substring.at(k);
                //cout << "IC: " << k << "-" << z << endl;
                if(z == code.at(i+k))
                {
                    found = true;
                }
                else
                {
                    found = false;
                    break;
                }
            }
            if(found)
            {
                //cout << "FOUND" << endl;
                return r;
                //cout << "AFR" << endl;
            }
            else
            {
                r = -1;
            }
        }
    }
}
void stripComments(string& code)
{
    string input = code;
    string output = "";
    int status = 0;
    bool inString = false;
    // Strip All Comments
    // 0 - Not In Comment
    // 1 - Full Line Comment
    // 2 - Block Type Comment
    char t;
    for(int i=0;i<input.length();i++)
    {
        t = input.at(i);
        if(t == '"')
        {
            inString = !inString;
        }
        if(inString && status == 0)
        {
            output += t;
        }
        else
        {
            if(status == 0)
            {
                if(t == '/')
                {
                    if(input.at(i+1) == '/')
                    {
                        status = 1;
                    }
                    else if(input.at(i+1) == '*')
                    {
                        status = 2;
                    }
                    else
                    {
                        output += t;
                    }
                }
                else
                {
                    output += t;
                }
            }
            else if(status == 1)
            {
                if(t == '\n')
                {
                    status = 0;
                }
            }
            else if(status == 2)
            {
                if(t == '*' && input.at(i+1) == '/')
                {
                    status = 0;
                    i++;
                }
            }
        }
    }
    code = output;
}
void stripLiterals(string& code)
{
    string phpcode = code;
    sregex token = sregex::compile("\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\"|'[^'\\\\]*(?:\\\\.[^'\\\\]*)*'");
    // Iterate Through PHP Cpde
    sregex_iterator cur( phpcode.begin(), phpcode.end(), token );
    sregex_iterator end;
    // Iterate
    for( ; cur != end; ++cur )
    {
        // Get Current Match
        smatch const &what = *cur;
        litNode t;
        int l = what.position(0)+what.length(0);
        string fTotal = subStrSect(code,what.position(0),l-1);
        t.hash = MD5String(fTotal.c_str());
        t.str = fTotal;
        litList.push_back(t);
        cout << "Found String: " << what[0] << endl;
        /**** DEBUGGING **/
        if(debug)
            cout << "    -> " << fTotal << endl;
    }

    for(int k=0;k<litList.size();k++)
    {
        string func = litList[k].str;
        string hash = litList[k].hash;
        /**** DEBUGGING **/
        if(debug)
            cout << "Replacing: " << func << endl;
        replaceString(code,func,"!!STR:"+hash+"!!");
    }

}
void stripFunctions(string& code)
{
    string phpcode = code;
    sregex token = sregex::compile("(function[\\s]*[ ]*)([a-zA-Z0-9_]*)([\\s]*)\\(([a-zA-Z0-9_\"$', .()]*)\\)");
    // Iterate Through PHP Cpde
    sregex_iterator cur( phpcode.begin(), phpcode.end(), token );
    sregex_iterator end;
    // Iterate
    for( ; cur != end; ++cur )
    {
        // Get Current Match
        smatch const &what = *cur;
        funcNode t;
        int x = findCodeStart(code,what.position(0));
        int y = findCodeEnd(code,x);
        string fTotal = subStrSect(code,what.position(0),y);
        t.hash = MD5String(fTotal.c_str());
        t.str = fTotal;
        funcList.push_back(t);
        cout << "Found: " << what[0] << endl;
        /**** DEBUGGING **/
        if(debug)
            cout << "    -> " << fTotal << endl;
    }

    for(int k=0;k<funcList.size();k++)
    {
        string func = funcList[k].str;
        string hash = funcList[k].hash;
        replaceString(code,func,"!!FH:"+hash+"!!");
    }
}
void insertFunctions(string& code)
{
    for(int k=0;k<funcList.size();k++)
    {
        funcNode f = funcList[k];
        string func = f.str;
        string hash = "!!FH:"+f.hash+"!!";
        replace(hash,func,code);
    }
}
void insertLiterals(string& code)
{
    for(int k=0;k<litList.size();k++)
    {
        litNode f = litList[k];
        string func = f.str;
        string hash = "!!STR:"+f.hash+"!!";
        replace(hash,func,code);
    }
}
void removeInitCall(string& code)
{
    string replace = ""; // String to Replace With
    string search = "[^A-Za-z0-9_-]*aophp_init[\\s]*\\(\"[A-Za-z0-9_\\-.,]*\"\\);"; // String to Search For
    sregex sr = sregex::compile(search); // Search Regex
    code = regex_replace( code, sr, replace );
    /*** CODE NOTE:
        src = regex_replace( src, sr, replace );
        ^^^ This code does all of the below marked code, however, it produces the error:
            *** glibc detected *** corrupted double-linked list: 0x0a0a6be0 ***
        This error is caused for an unknown reason and appears at the very end of execution of the program causing an Abort

        Needs Further Investigation.
    ***/
    /*SM
    sregex_iterator cur( code.begin(), code.end(), sr ); // Start Iterator
    sregex_iterator end; // End Iterator
    for( ; cur != end; ++cur )
    {
        // Get Current Match
        smatch const &what = *cur;
        // Matched - Remove/Replace
        int s = what.position(0);
        int l = what.length(0);
        code.erase(s,l);
    }
    EM*/
}

/*** Recursive Descent Parser Functions ***/
void checkToken(string expected,int type)
{
    int l = expected.length();
    for(int i=0;i<l;i++)
    {
        string s = nextToken();
        if(s.compare(expected.substr(i,1))!=0)
            //throw parseException("PE: Parse Error on Token \""+s+"\", Expecting \""+expected+"\" - Bailing Out");
            error("Parse Error on Token \""+s+"\", Expecting \""+expected+"\" - Bailing Out");
    }
    addAST(type,expected);
    return;
}
void checkToken(string ch[],int size,int type)
{
    for(int i=0;i<size;i++)
    {
        int l = ch[i].length();
        if(lookToken(l).compare(ch[i])==0)
        {
            checkToken(ch[i],type);
            return;
        }
    }
    error("Parse Error on String, No Matching Rule Found - Bailing Out");
}
void checkToken(int regex,string rule,int type)
{
    string s = "";
    if(regex == 1)
    {
        s = nextToken();
        while(re_1_ID(s))
        {
            s += nextToken();
        }
        addAST(type,s.substr(0,s.length()-1));
        loc--;
        return;
    }
    else if(regex == 2)
    {
        s = nextToken();
        while(re_2_ArgVal(s))
        {
            s += nextToken();
        }
        addAST(type,s.substr(0,s.length()-1));
        loc--;
        return;
    }
    else if(regex == 3)
    {
        s = nextToken();
        while(re_3_NonDelim(s))
        {
            s += nextTokenWS();
        }
        addAST(type,s.substr(0,s.length()-1));
        loc--;
        return;
    }
    else if(regex == 4)
    {
        s = nextToken();
        while(re_4_Digit(s))
        {
            s += nextToken();
        }
        addAST(type,s.substr(0,s.length()-1));
        loc--;
        return;
    }
    else
        error("Parse Error on Token \""+s+"\" Regex Match, Does not Match Rule "+rule+" - Bailing Out");
}
string lookTokenTo(string s)
{
    string t = aophpSource.substr(loc,1);
    int l = loc;
    if(t.compare(" ")!=0&&t.compare("\t")!=0)
    {
        int x = aophpSource.find_first_of(s,l-1);
        string temp = aophpSource.substr(l,x-l);
        return temp;
    }
    else
    {
        while(t.compare(" ")==0||t.compare("\t")==0)
        {
            t = aophpSource.substr(l,1);
            l++;
        }
        int x = aophpSource.find_first_of(s,l-1);
        return aophpSource.substr(l-1,x-l+1);
    }
}
string lookToken()
{
    string t = aophpSource.substr(loc,1);
    int l = loc;
    if(t.compare(" ")!=0&&t.compare("\t")!=0)
        return t;
    else
    {
        while(t.compare(" ")==0||t.compare("\t")==0)
        {
            t = aophpSource.substr(l,1);
            l++;
        }
        return t;
    }
}
string lookToken(int num)
{
    string t = aophpSource.substr(loc,1);
    int l = loc;
    if(t.compare(" ")!=0&&t.compare("\t")!=0)
    {
        return aophpSource.substr(loc,num);
    }
    else
    {
        while(t.compare(" ")==0||t.compare("\t")==0)
        {
            t = aophpSource.substr(l,1);
            l++;
        }
        return aophpSource.substr(l-1,num);
    }
}
string nextToken()
{
    string t = aophpSource.substr(loc,1);
    loc++;
    while(t.compare(" ")==0||t.compare("\t")==0)
    {
        t = aophpSource.substr(loc,1);
        loc++;
    }
    return t;
}
string nextTokenWS()
{
    string t = aophpSource.substr(loc,1);
    loc++;
    return t;
}
void parse()
{ // START -> <ASPECT>+
    int l = aophpSource.length();
    parseAspect_plus();
    if(loc != l)
        error("Invalid String, Length Error");
}
void parseAspect_plus()
{
    parseAspect();
    string s = lookToken(6);
    while(s.compare("aspect")==0)
    {
        parseAspect();
        s = lookToken(6);
    }
}
void parseAspect()
{ // ASPECT -> "aspect" <ID> '{' <POINTCUT>* <ADVICE>* '}'
    checkToken("aspect",ASPECT);
    parseID();
    checkToken("{",LC);
    parsePointcut_star();
    parseAdvice_star();
    checkToken("}",RC);
}
void parsePointcut_star()
{
    //cout << "Parsing Pointcut Star" << endl;
    string s = lookToken(8);
    while(s.compare("pointcut")==0)
    {
        parsePointcut();
        s = lookToken(8);
    }
}
void parsePointcut()
{ // POINTCUT -> "pointcut" <ID> '=' <JOINPOINT> <JOINPOINTLIST>* ';'
    checkToken("pointcut",POINTCUT);
    parseID();
    checkToken("=",EQUAL);
    parseJoinpoint();
    parseJoinpointlist_star();
    checkToken(";",SEMIC);
}
void parseJoinpoint()
{ // JOINPOINT -> <FJOINPOINT> | <VJOINPOINT> | ID
    string s = lookTokenTo(" (");
    if(s.compare("exec")==0 || s.compare("execr")==0)
        parseFJoinpoint();
    else if(s.compare("set")==0 || s.compare("get")==0)
        parseVJoinpoint();
    else
        parseID();
}
void parseFJoinpoint()
{ // FJOINPOINT -> <FJPTYPE>'('<SIGNATURE><PARAMLIST>')'<ARGS>*
    parseFJPType();
    checkToken("(",LP);
    parseSignature();
    parseParamlist();
    checkToken(")",RP);
    parseArgs_star();
}
void parseVJoinpoint()
{ // VJOINPOINT -> <FJPTYPE>'('<SIGNATURE>')'<ARGS>*
    parseVJPType();
    checkToken("(",LP);
    parseSignature();
    checkToken(")",RP);
    parseArgs_star();
}
void parseJoinpointlist()
{ // JOINPOINTLIST -> '|' <JOINPOINT>
    checkToken("|",PIPE);
    parseJoinpoint();
}
void parseJoinpointlist_star()
{
    string s = lookToken();
    while(s.compare("|")==0)
    {
        parseJoinpointlist();
        s = lookToken();
    }
}
void parseFJPType()
{ // FJPTYPE -> "exec" | "execr"
    string ch[2] =
        {"execr","exec"
        };
    checkToken(ch,2,FJPTYPE);
}
void parseVJPType()
{ // VJPTYPE -> "set" | "get"
    string ch[2] =
        {"set","get"
        };
    checkToken(ch,2,VJPTYPE);
}
void parseArgs()
{ // ARGS -> "&&" <ID> '('<ARGVAL>')'
    checkToken("&&",AND);
    parseID();
    checkToken("(",LP);
    parseArgVal();
    checkToken(")",RP);
}
void parseArgs_star()
{
    string s = lookToken(2);
    while(s.compare("&&")==0)
    {
        parseArgs();
        s = lookToken(2);
    }
}
void parseArgVal()
{ // ARGVAL -> ('"'(.)*'"')|(<DIGIT>*)
    //checkToken(2,"ArgVal");
    string s = lookToken();
    if(s.compare("\"")==0)
    {
        checkToken("\"",QUOTE);
        checkToken(2,"ArgVal",STRING);
        checkToken("\"",QUOTE);
    }
    else
    {
        checkToken(4,"Digit",DIGIT);
    }
}
void parseAdvice()
{ // ADVICE -> "advice" <ADVICETYPE> <PARAMLIST> ':' <JOINPOINT> <JOINPOINTLIST>* <CODEBLOCK>
    checkToken("advice",ADVICE);
    parseAdviceType();
    parseParamlist();
    checkToken(":",COLON);
    parseJoinpoint();
    parseJoinpointlist_star();
    parseCodeBlock();
}
void parseAdvice_star()
{
    string s = lookToken(6);
    while(s.compare("advice")==0)
    {
        parseAdvice();
        s = lookToken(6);
    }
}
void parseCodeBlock()
{ // CODEBLOCK -> <DELIM> <NONDELIM>* <ENDDELIM>
    parseDelim();
    parseNonDelim();
    parseEndDelim();
}
void parseID()
{ // ID -> <LETTER><ALPHA>*
    checkToken(1,"ID",ID);
}
void parseVar()
{ // VAR -> '$'<ID>
    checkToken("$",DOLLAR);
    parseID();
}
void parseVarList()
{ // VAR_LIST -> ','<VAR>
    checkToken(",",COMA);
    parseVar();
}
void parseVarlist_star()
{
    string s = lookToken();
    while(s.compare(",")==0)
    {
        parseVarList();
        s = lookToken();
    }
}
void parseVars()
{ // VARS -> '$'<ID><VARLIST>*
    checkToken("$",DOLLAR);
    parseID();
    parseVarlist_star();
}
void parseSignature()
{ // SIGNATURE -> <ID>'.'<ID>[<PARAMLIST>]
    parseID();
    checkToken(".",DOT);
    parseID();
}
void parseParamlist()
{ // PARAMLIST -> '('[<VARS>]')'
    //cout << "Parsing Paramlist" << endl;
    checkToken("(",LP);
    if(lookToken().compare("$")==0)
        parseVars();
    checkToken(")",RP);
}
void parseAdviceType()
{
    string ch[3] =
        {"before","after","around"
        };
    checkToken(ch,3,ADVTYPE);
}
void parseDelim()
{ // DELIM -> "{%"
    checkToken("{%",SDELIM);
}
void parseEndDelim()
{ // ENDDELIM -> "%}"
    checkToken("%}",EDELIM);
}
void parseNonDelim()
{ // NONDELIM -> ([^%]|[%][^}])*
    checkToken(3,"NonDelim",CODE);
}
bool re_1_ID(string token)
{
    sregex str = sregex::compile("[A-Za-z][A-Za-z0-9]*");
    return regex_match(token,str);
}
bool re_2_ArgVal(string token)
{
    sregex str = sregex::compile("[^\"\\r\\n]*");
    return regex_match(token,str);
}
bool re_4_Digit(string token)
{
    sregex str = sregex::compile("[0-9]*");
    return regex_match(token,str);
}
bool re_3_NonDelim(string token)
{
    sregex str = sregex::compile("([^%]|[%][^}])*");
    return regex_match(token,str);
}
void error(string err)
{
    cout << "\t\33[33m [ERROR] \33[0m" << err << endl;
    exit(5);
}

/*** Abstract Syntax Tree Functions ***/
void printAST()
{
    for(int i=0;i<ast.size();i++)
    {
        astNode t = ast[i];
        cout << t.type << " -:" << t.val << endl;
    }
}
void addAST(int type, string val)
{
    astNode t;
    t.type = type;
    t.val = val;
    ast.push_back(t);
}
bool hasNodes()
{
    int s = ast.size();
    return node<s;
}
astNode nextNode()
{
    astNode t = ast[node];
    node++;
    return t;
}
int lookNode()
{
    astNode t = ast[node];
    return t.type;
}
int lookNode(int s)
{
    astNode t = ast[node+(s-1)];
    return t.type;
}
string checkNode(int t,string func="")
{
    astNode temp = nextNode();
    if(temp.type == t)
        return temp.val;
    else
        error("AST Process Error. Invalid Type Found, Found: \""+temp.val+"\"::"+itos(temp.type)+" Expecting: "+itos(t)+" in Func: "+func);
}
void parseAST()
{
    while(hasNodes())
    {
        checkNode(ASPECT);
        checkNode(ID);
        checkNode(LC);
        while(lookNode()==POINTCUT)
        {
            pcs.push_back(parseAST_Pointcut());
        }
        while(lookNode()==ADVICE)
        {
            parseAST_Advice();
        }
        checkNode(RC);
    }
}
pcNode parseAST_Pointcut()
{
    // Get PC Name
    pcNode t;
    checkNode(POINTCUT,"Pointcut - pointcut");
    t.name = checkNode(ID,"Pointcut - Name");
    checkNode(EQUAL,"Pointcut");
    // Get Actual Joinpoints, Must be atleast 1
    if(lookNode()==FJPTYPE || lookNode()==VJPTYPE)
    {
        t.jp.push_back(parseAST_Joinpoint());
    }
    else
    {
        // Reference another Pointcut
        vector<jpNode> t_list;
        t_list = pcsSearch(checkNode(ID));
        int s = t_list.size();
        for(int i=0;i<s;i++)
        {
            t.jp.push_back(t_list[i]);
        }
    }
    while(lookNode()==PIPE)
    {
        checkNode(PIPE);
        if(lookNode()==FJPTYPE || lookNode()==VJPTYPE)
        {
            t.jp.push_back(parseAST_Joinpoint());
        }
        else
        {
            // Reference another Pointcut
            vector<jpNode> t_list;
            t_list = pcsSearch(checkNode(ID));
            int s = t_list.size();
            for(int i=0;i<s;i++)
            {
                t.jp.push_back(t_list[i]);
            }
        }
    }
    // Ending Semicolon
    checkNode(SEMIC,"Pointcut");
    return t;
}
jpNode parseAST_Joinpoint()
{
    jpNode t;
    if(lookNode()==FJPTYPE)
    {
        t.type = checkNode(FJPTYPE,"Func Joinpoint - Type");
        checkNode(LP,"Func Joinpoint");
        t.file = checkNode(ID,"Func Joinpoint - File");
        checkNode(DOT,"Func Joinpoint");
        t.sig = checkNode(ID,"Func Joinpoint - Sig");
        checkNode(LP,"Func Joinpoint");
        while(lookNode()!=RP)
        {
            checkNode(DOLLAR,"Func Joinpoint");
            t.paramList.push_back(checkNode(ID,"Func Joinpoint - Paramlist"));
            if(lookNode()==COMA)
                checkNode(COMA);
        }
        checkNode(RP,"Func Joinpoint");
        checkNode(RP,"Func Joinpoint");
        // Argument Check
        if(lookNode()==AND)
        {
            while(lookNode()==AND)
            {
                t.args.push_back(parseAST_Argument());
            }
        }
    }
    else if(lookNode()==VJPTYPE)
    {
        t.type = checkNode(VJPTYPE,"Var Joinpoint - Type");
        checkNode(LP,"Var Joinpoint");
        t.file = checkNode(ID,"Var Joinpoint - File");
        checkNode(DOT,"Var Joinpoint");
        t.sig = checkNode(ID,"Var Joinpoint - Sig");
        checkNode(RP,"Var Joinpoint");
        // Argument Check
        if(lookNode()==AND)
        {
            while(lookNode()==AND)
            {
                t.args.push_back(parseAST_Argument());
            }
        }
    }
    return t;
}
argNode parseAST_Argument()
{
    argNode t;
    checkNode(AND,"Argument");
    t.arg = checkNode(ID,"Argument");
    checkNode(LP,"Argument");
    if(lookNode()==QUOTE)
    {
        checkNode(QUOTE,"Argument");
        t.sval = checkNode(STRING,"Argument - Str Val");
        checkNode(QUOTE,"Argument");
    }
    else
        t.ival = atoi(checkNode(DIGIT,"Argument - Int Val").c_str());
    checkNode(RP,"Argument");
    return t;
}
void parseAST_Advice()
{
    vector<jpNode> jps;
    advNode a;
    checkNode(ADVICE,"Advice - advice");
    a.type = checkNode(ADVTYPE,"Advice - Type");
    checkNode(LP,"Advice");
    while(lookNode()!=RP)
    {
        checkNode(DOLLAR,"Advice");
        a.paramList.push_back(checkNode(ID,"Advice - Paramlist"));
        if(lookNode()==COMA)
            checkNode(COMA,"Advice");
    }
    checkNode(RP,"Advice");
    checkNode(COLON,"Advice");
    while(lookNode()!=SDELIM)
    {
        // Grab Joinpoints
        if(lookNode()==FJPTYPE || lookNode()==VJPTYPE)
            jps.push_back(parseAST_Joinpoint());
        else
        {
            vector<jpNode> temp;
            temp = pcsSearch(checkNode(ID));
            jps.insert(jps.end(), temp.begin(), temp.end());
        }
        while(lookNode()==PIPE)
        {
            checkNode(PIPE,"Advice - Joinpoint Pipe");
            if(lookNode()==FJPTYPE || lookNode()==VJPTYPE)
                jps.push_back(parseAST_Joinpoint());
            else
            {
                vector<jpNode> temp;
                temp = pcsSearch(checkNode(ID));
                jps.insert(jps.end(), temp.begin(), temp.end());
            }
        }
    }
    checkNode(SDELIM,"Advice");
    a.code = checkNode(CODE,"Advice - Code");
    checkNode(EDELIM,"Advice");
    // need to Add the Advice to the table
    for(int i=0;i<jps.size();i++)
    {
        advNode cur;
        cur.type = a.type;
        cur.paramList = a.paramList;
        cur.code = a.code;
        cur.jp = jps[i];
        adv.push_back(cur);
    }
}
vector<jpNode> pcsSearch(string id)
{
    int s = pcs.size();
    for(int i=0;i<s;i++)
    {
        if(id.compare(pcs[i].name)==0)
        {
            return pcs[i].jp;
        }
    }
    vector<jpNode> j;
    return j;
}

/*** Code Generation Functions ***/
void printStats()
{
    int a = adv.size();
    int p = pcs.size();
    cout << "\t\33[34m [DEBUG] \33[0mFound " << a << " Advice & " << p << " Pointcut(s)." << endl;
}
void printAdvice()
{
    cout << "\t\33[34m [DEBUG] \33[0mPrinting Advice:" << endl;
    for(int i=0;i<adv.size();i++)
    {
        advNode t;
        t = adv[i];
        cout << "\t" << i << ": " << t.type << "(";
        for(int k=0;k<t.paramList.size();k++)
        {
            cout << t.paramList[k] << ",";
        }
        cout << ")" << t.jp.type << "(" << t.jp.file << "." << t.jp.sig << ")" << endl;
    }
}
void printPointcuts()
{
    cout << "\t\33[34m [DEBUG] \33[0mPrinting Pointcuts:" << endl;
    for(int i=0;i<pcs.size();i++)
    {
        pcNode t;
        t = pcs[i];
        for(int k=0;k<t.jp.size();k++)
        {
            cout << "\t" << i << "." << k << ": " << t.name << " = " << t.jp[k].type << "(" << t.jp[k].file << "." << t.jp[k].sig << ")" << endl;
        }
    }
}
void genTargetList()
{
    cout << "Building Target List from " << adv.size() << " Adivce ..." << endl;
    advNode t;
    for(int i=0;i<adv.size();i++)
    {
        t = adv[i];
        jpNode j;
        j = t.jp;
        int n = isTarget(t,j);
        tgtNode temp;
        switch(n)
        {
            case -2:
            // Matched List, Toss Out
            break;
            case -1:
            // Not Matched in List
            // Build Target Node

            if(t.type.compare("before")==0)
            {
                temp.hasBefore = true;
                temp.hasAfter = false;
                temp.hasAround = false;
                temp.before = t;
                temp.jp = j;
            }
            else if(t.type.compare("after")==0)
            {
                temp.hasBefore = false;
                temp.hasAfter = true;
                temp.hasAround = false;
                temp.after = t;
                temp.jp = j;
            }
            else if(t.type.compare("around")==0)
            {
                temp.hasBefore = false;
                temp.hasAfter = false;
                temp.hasAround = true;
                temp.around = t;
                temp.jp = j;
            }
            // Push Target Node
            tgts.push_back(temp);
            break;
            default:
            // 0 or Greater, n is Node
            // In List, but advType not a match, add advType
            if(t.type.compare("before")==0)
            {
                tgts[n].hasBefore=true;
                tgts[n].before = t;
            }
            else if(t.type.compare("after")==0)
            {
                tgts[n].hasAfter=true;
                tgts[n].after = t;
            }
            else if(t.type.compare("around")==0)
            {
                tgts[n].hasAround=true;
                tgts[n].around = t;
            }
            break;
        }
    }
    cout << "Target List Built." << endl;
}
int isTarget(advNode a,jpNode j)
{
    for(int i=0;i<tgts.size();i++)
    {
        tgtNode c = tgts[i];
        if(c.jp.type.compare(j.type)==0 && c.jp.file.compare(j.file)==0 && c.jp.sig.compare(j.sig)==0 && c.jp.paramList.size()==j.paramList.size())
        {
            if(a.type.compare("before")==0)
            {
                if(c.hasBefore)
                    return -2;
                else
                    return i;
            }
            else if(a.type.compare("after")==0)
            {
                if(c.hasAfter)
                    return -2;
                else
                    return i;
            }
            else if(a.type.compare("around")==0)
            {
                if(c.hasAround)
                    return -2;
                else
                    return i;
            }
        }
    }
    return -1;
}
void printTargetList()
{
    for(int i=0;i<tgts.size();i++)
    {
        tgtNode t;
        t = tgts[i];
        cout << i << ") " << t.jp.file << "." << t.jp.sig << "(" << t.jp.paramList.size() << ")";
        cout << " | bf:" << t.hasBefore << " ar:" << t.hasAround << " af:" << t.hasAfter << endl;
    }
}
string processTargets()
{
    string temp;
    temp = "function aophp_init($files){ /* Does Nothing */ } \n";
    for(int i=0;i<tgts.size();i++)
    {
        tgtNode t;
        advNode a;
        t = tgts[i];
        // Build Funcs
        if(t.jp.type.compare("set")==0||t.jp.type.compare("get")==0)
        {

            if(t.hasBefore)
            {
                temp += buildVHelper(t.before,t.jp,"bf");
                a=t.before;
            }
            if(t.hasAround)
            {
                temp += buildVHelper(t.around,t.jp,"ar");
                a=t.around;
            }
            if(t.hasAfter)
            {
                temp += buildVHelper(t.after,t.jp,"af");
                a=t.after;
            }
            tgtVars.push_back(t.jp.sig);
            temp += buildVMain(t,a);
        }
        else
        {
            if(t.hasBefore)
            {
                temp += buildFHelper(t.before,t.jp,"bf");
                a=t.before;
            }
            if(t.hasAround)
            {
                temp += buildFHelper(t.around,t.jp,"ar");
                a=t.around;
            }
            if(t.hasAfter)
            {
                temp += buildFHelper(t.after,t.jp,"af");
                a=t.after;
            }
            temp += buildFMain(t,a);
        }
    }
    if(debug)
        cout << "Funcs: \n------------------------------\n" << temp << "\n--------------------------------------\n";
    return temp;
}
string buildFMain(tgtNode t,advNode a)
{
    jpNode j;
    j = t.jp;
    bool returns = false;
    if(j.type.compare("execr")==0)
        returns=true;
    string func = "";
    string t2 = j.type+j.file+j.sig;
    string hash = MD5String(t2.c_str());
    string pString = "";

    func += "function fi_"+hash+"_"+j.sig+"(";
    for(int i=0;i<j.paramList.size();i++)
    {
        pString += "$";
        pString += a.paramList[i];
        if(i!=j.paramList.size()-1)
        {
            pString += ",";
        }
    }
    func += pString+"){";

    string call = "_"+hash+"_"+j.sig+"("+pString+");";

    // Build Middle of Function
    if(t.hasBefore)
        func+="bf"+call;
    if(t.hasAround)
    {
        if(returns)
            func+="$aophp_temp_return_val = ar"+call; // Call Around Func, Store Return
        else
            func+="ar"+call; // Call Around Func
    }
    else
    {
        if(returns)
            func+="$aophp_temp_return_val = "+j.sig+"("+pString+");"; // Call Orig Func, Store Return
        else
            func+=j.sig+"("+pString+");"; // Call Orig Func
    }
    if(t.hasAfter)
        func+="af"+call;
    if(returns)
        func+="return $aophp_temp_return_val;";
    func += "}\n";
    return func;
}
string buildFHelper(advNode a,jpNode j,string type)
{
    string func = "";
    string t = j.type+j.file+j.sig;
    string hash = MD5String(t.c_str());
    string pString = "";

    func += "function "+type+"_"+hash+"_"+j.sig+"(";
    for(int i=0;i<j.paramList.size();i++)
    {
        pString += "$";
        pString += a.paramList[i];
        if(i!=j.paramList.size()-1)
        {
            pString += ",";
        }
    }
    string code = a.code;
    // Replace Proceed Calls with a Return Statement if After Advice
    if(type.compare("ar")==0)
    {
        replace("proceed","return "+j.sig,code);
    }
    func += pString+"){"+code+"}\n";
    return func;
}
string buildVHelper(advNode a,jpNode j,string type)
{
    string func = "";
    string t = j.type+j.file+j.sig;
    string hash = MD5String(t.c_str());
    string pString = "";

    func += "function "+type+"_"+hash+"_aopvar"+j.type+"_"+j.sig+"(";
    for(int i=0;i<a.paramList.size();i++)
    {
        pString += "$";
        pString += a.paramList[i];
        if(i!=a.paramList.size()-1)
        {
            pString += ",";
        }
    }
    string code = a.code;
    // Replace Proceed Calls with a Return Statement if After Advice
    if(type.compare("ar")==0)
    {
        replace("proceed_orig","return "+pString,code);
        replace("proceed","return ",code);
    }
    func += pString+"){"+code+"}\n";
    return func;
}
string buildVMain(tgtNode t,advNode a)
{
    jpNode j;
    j = t.jp;
    bool returns = false;
    if(j.type.compare("get")==0)
        returns=true;
    string func = "";
    string t2 = j.type+j.file+j.sig;
    string hash = MD5String(t2.c_str());
    string pString = "";

    func += "function fi_"+hash+"_aopvar"+j.type+"_"+j.sig+"(";
    for(int i=0;i<a.paramList.size();i++)
    {
        pString += "$";
        pString += a.paramList[i];
        if(i!=a.paramList.size()-1)
        {
            pString += ",";
        }
    }
    func += pString+"){";

    string call = "_"+hash+"_aopvar"+j.type+"_"+j.sig+"("+pString+");";

    // Build Middle of Function
    if(t.hasBefore)
        func+="bf"+call;
    if(t.hasAround)
    {
        if(returns)
            func+="$aophp_temp_return_val = ar"+call; // Call Around Func, Store Return
        else
            func+="ar"+call; // Call Around Func
    }
    else
    {
        if(returns)
            func+="$aophp_temp_return_val = "+j.sig+"("+pString+");"; // Call Orig Func, Store Return
        else
            func+=j.sig+"("+pString+");"; // Call Orig Func
    }
    if(t.hasAfter)
        func+="af"+call;
    if(returns)
        func+="return $aophp_temp_return_val;";
    func += "}\n";
    return func;
}
void weave()
{
    stripSet(phpSource);
    stripGet(phpSource);
    weaveFuncs();
    weaveGet();
    weaveSet();
}
void weaveFuncs()
{
    string src = phpSource;
    for(int i=0;i<tgts.size();i++)
    {
        tgtNode t;
        t = tgts[i];
        jpNode j;
        j = t.jp;
        if(j.type.compare("set")==0||j.type.compare("get")==0)
            continue;
        string t2 = j.type+j.file+j.sig;
        string hash = MD5String(t2.c_str());
        string replace = "\nfi_"+hash+"_"+j.sig+"("; // String to Replace With
        string search = "[^A-Za-z0-9_-]"+j.sig+"[\\s]*\\("; // String to Search For
        sregex sr = sregex::compile(search); // Search Regex
        src = regex_replace( src, sr, replace );
        /*** CODE NOTE:
            src = regex_replace( src, sr, replace );
            ^^^ This code does all of the below marked code, however, it produces the error:
                *** glibc detected *** corrupted double-linked list: 0x0a0a6be0 ***
            This error is caused for an unknown reason and appears at the very end of execution of the program causing an Abort


            Jan 8, 2007: Works Now. Dont know Why. But it Does.
        ***/
        /*SM
        sregex_iterator cur( src.begin(), src.end(), sr ); // Start Iterator
        sregex_iterator end; // End Iterator
        for( ; cur != end; ++cur )
        {
            // Get Current Match
            smatch const &what = *cur;
            // Matched - Remove/Replace
            int s = what.position(0);
            int l = what.length(0);
            src.erase(s,l);
            src.insert(s,replace);
        }
        /*EM*/
    }
    phpSource = src;
}
void stripSet(string& code)
{
    // Type 1
    string phpcode = code;
    sregex token = sregex::compile("(\\$[A-Za-z][A-Za-z0-9-_]*[\\s]*=)([^;]*)(;)");
    // Iterate Through PHP Cpde
    sregex_iterator cur( phpcode.begin(), phpcode.end(), token );
    sregex_iterator end;
    // Iterate
    for( ; cur != end; ++cur )
    {
        // Get Current Match
        smatch const &what = *cur;
        varNode t;
        int l = what.position(0)+what.length(0);
        string fTotal = subStrSect(code,what.position(0),l-1);
        t.hash = MD5String(fTotal.c_str());
        t.str = fTotal;
        t.val = what[2];
        t.type = 1;
        varList.push_back(t);
        cout << "Found Set: " << what[0] << endl;
        /**** DEBUGGING **/
        if(debug)
            cout << "    -> " << fTotal << endl;
    }

    for(int k=0;k<varList.size();k++)
    {
        if(varList[k].type==2)
            continue;
        string func = varList[k].str;
        string hash = varList[k].hash;
        string val = varList[k].val;
        string rep = "!!SET:"+hash+"!! "+val+" ;";
        /**** DEBUGGING **/
        //if(debug)
        cout << "Replacing Set: " << func << " With |" << rep <<"|" <<endl;
        replaceString(code,func,rep);
    }
}
void stripGet(string& code)
{
    // Type 2
    string phpcode = code;
    sregex token = sregex::compile("\\$[A-Za-z][A-Za-z0-9-_]*");
    // Iterate Through PHP Cpde
    sregex_iterator cur( phpcode.begin(), phpcode.end(), token );
    sregex_iterator end;
    // Iterate
    for( ; cur != end; ++cur )
    {
        // Get Current Match
        smatch const &what = *cur;
        varNode t;
        int l = what.position(0)+what.length(0);
        string fTotal = subStrSect(code,what.position(0),l-1);
        t.hash = MD5String(fTotal.c_str());
        t.str = fTotal;
        t.type = 2;
        varList.push_back(t);
        cout << "Found Get: " << what[0] << endl;
        /**** DEBUGGING **/
        if(debug)
            cout << "    -> " << fTotal << endl;
    }

    for(int k=0;k<varList.size();k++)
    {
        if(varList[k].type==1)
            continue;
        string func = varList[k].str;
        string hash = varList[k].hash;
        string rep = "!!GET:"+hash+"!!";
        /**** DEBUGGING **/
        if(debug)
            cout << "Replacing Get: " << func << " With |" << rep <<"|" <<endl;
        replaceString(code,func,rep);
    }
}
void weaveSet()
{
// Regex: "!!SET:([a-z0-9]{32})!![\\s]*([^;]*);"

    // Find All Sets & add to Set List
    string code = phpSource;
    sregex token = sregex::compile("!!SET:([a-z0-9]{32})!![\\s]*([^;]*);");
    sregex_iterator cur( code.begin(), code.end(), token );
    sregex_iterator end;
    for( ; cur != end; ++cur )
    {
        // Get Current Match
        smatch const &what = *cur;
        setNode t;
        t.match = what[0];
        t.hash = what[1];
        t.val = what[2];
        setList.push_back(t);
        cout << "Found Set: " << what[0] << "-" << what[1] << "-" << what[2] << endl;
    }

    //Locate Matched Sets, and Replace accordingly
    for(int k=0;k<setList.size();k++)
    {
        string match = setList[k].match;
        string hash = setList[k].hash;
        string val = setList[k].val;
        varNode t = getVarNode(hash);
        string var = subStrSect(t.str,t.str.find_first_not_of("$"),t.str.find_first_of("=")-1);
        trim(var);
        string isTgt = "$"+var+" = "+"fi_"+hash+"_aopvarset_"+var+"("+val+");";
        string notTgt = "$"+var+" = "+val+";";
        cout << "Node: " << var << " - " << t.str << endl;
        if(isTargetVar(var))
            replace(match,isTgt,code);
        else
            replace(match,notTgt,code);
    }

    phpSource = code;

}
void weaveGet()
{
    string code = phpSource;
    for(int k=0;k<varList.size();k++)
    {
        varNode f = varList[k];
        if(f.type == 2){
            string var = f.str; // $var
            string v = var.substr(var.find_first_not_of("$"));
            trim(v);
            string hash = "!!GET:"+f.hash+"!!";
            string func = "fi_"+f.hash+"_aopvarget_"+v+"($"+v+")";
            if(isTargetVar(v))
                replace(hash,func,code);
            else
                replace(hash,var,code);
        }
    }
    phpSource = code;
}

varNode getVarNode(string h){
    varNode t;
    for(int k=0;k<varList.size();k++)
    {
        if(varList[k].hash.compare(h)==0)
            return varList[k];
    }
    return t;

}
bool isTargetVar(string v){
    for(int k=0;k<tgtVars.size();k++)
    {
        if(tgtVars[k].compare(v)==0)
            return true;
    }
    return false;
}

