#include <iostream>
#include <string>
#include <string.h>
#include "sha1.cpp"
using namespace std;

string getHash(char *ip, int port) {
    char str[1024];
    
    string port_str = to_string(port);
    uint32_t key;
    unsigned char hash[1024];
    strcpy(str, ip);
    strcat(str, ":");
    strcat(str, port_str.c_str());
    
    SHA1 checksum;
    checksum.update(str);
    cout << checksum.final() << endl;
    // memcpy(&key, hash + 16, sizeof(key));
    // cout << key << endl;
    return checksum.final();
} 

int main() {
    char a[102];
    strcpy(a, "127.0.0.1");
    getHash(a, 8001);
    getHash(a, 8001);
        getHash(a, 8001);

    getHash(a, 8001);

}