#include <iostream>
#include <string>
#include <sstream>
#include <string.h>
using namespace std;

string xml_to_string(int port, string key, string mess) {
    stringstream st;
    string s;
    s = s + "<mess>" + mess + "</mess>\n";
    s = s + "<port>" + to_string(port) + "</port>\n";
    s = s + "<key>" + key + "</key>";

    return s;
}

int string_to_port(string st) {
    string line;
    stringstream s(st);
    getline(s, line);
    getline(s, line);

    string port = line.substr(6, line.size()-13);
    //cout << port << endl;
    char po[10];
    strcpy(po, port.c_str());
    return atoi(po);
}

string string_to_key(string st) {
    string line;
    stringstream s(st);
    getline(s, line);
    getline(s, line);
    getline(s, line);
    if(line.size()<11) {
        return string("");
    }
    string key = line.substr(5, line.size()-11);
    //cout << port << endl;
    
    return key;
}

string string_to_mess(string st) {
    string line;
    stringstream s(st);
    getline(s, line);
    if(line.size()<13) {
        return string("");
    }
    string key = line.substr(6, line.size()-13);
    //cout << port << endl;
    
    return key;
}

// int main() {

//     string s = xml_to_string(9020,"1234SXSXOOO", "FINDSUCC");
//    // cout << s << endl;
//     string port = string_to_mess(s);
//     cout << port << endl;
//     return 0;
// }
