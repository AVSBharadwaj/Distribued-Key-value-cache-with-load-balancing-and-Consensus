#include <iostream>
#include <map>
#include <string>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <stdint.h>
#include <math.h>

#include "xml.cpp"
#include "sha1.cpp"
using namespace std;
pthread_t listen_thread;
pthread_t stabilize_thread;
pthread_t fixfinger_thread;

int no=0;

struct Node {
    int port;
    string key;
    char ip_addr[1024];
};

Node fetch_successor(string);
bool belongs_to(string, string, string);
void send_notify(Node);
struct Node successor;
struct Node predecessor;
struct Node local;
struct Node finger_table[17];
map<string, string> hash_table;

void *stabilize(void *ptr) {
    while(true) {
       // cout << "THIS" << endl;
        if(predecessor.key.compare("NULL") != 0) {
            cout << "Predecessor: " << predecessor.port << endl;
        }
        if(successor.key.compare("NULL") != 0) {
            cout << "Successor: " << successor.port << endl;
        }
        cout << endl << endl;
        //cout << "Current: " << local.port << endl;
        if (successor.port == local.port && predecessor.key.compare("NULL")!=0) {
            //cout << "IN" << endl;
            finger_table[1] = predecessor;
            successor = predecessor;
        }
        else {
            // find and make a node of x=succ.pred
            int port = successor.port;
            // if(local.port == successor.port) {
            //     sleep(5);
            //     continue;
            // }
            string req = xml_to_string(local.port, local.key, "PRED");

            int sock = socket(AF_INET, SOCK_STREAM, 0);

            struct sockaddr_in serv_addr;
            char resp[1024];
            serv_addr.sin_family = AF_INET; 
	        serv_addr.sin_port = htons(port);
            inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
            connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

            strcpy(resp, req.c_str());
           // cout << "JUST BEFORE SEND" << endl;
            send(sock, resp, strlen(resp), 0);
         //   cout << "AFTER SEND" << endl;
            read(sock, resp, 1024);
       //     cout << "AFTER READ" << endl;
            //close(sock);
            req =  string(resp);

            port = string_to_port(req);
            string key = string_to_key(req);

            Node x;
            x.port = port;
            x.key = key;

            // cout << "Responding port: " << port << endl;
            //cout << "port received: " << x.port << endl;
            strcpy(x.ip_addr, "127.0.0.1");
            
            // if(belongs_to(x, local, succ)) succ = x
            if(x.key.compare("NULL")!=0 && belongs_to(x.key, local.key, successor.key) && local.key.compare(successor.key)!=0 && local.key.compare(x.key)!=0 && x.key.compare(successor.key)!=0) {
               //cout << "Bleongs to : true" << endl;
                successor = x;
               // cout << "Successor: " << successor.port << endl;
                finger_table[1] = x;
            }


        }
        no++;
        send_notify(successor);
        sleep(1);
    }
}

void notify(Node n_dash) {
    //cout << "Inside notify" << endl;
    

    if (predecessor.key.compare("NULL") == 0 || predecessor.port == local.port) {
        predecessor = n_dash;

       // cout << "Found Predecessor: " << predecessor.key << " : "  << predecessor.port << endl;

    }
    else if(belongs_to(n_dash.key, predecessor.key, local.key) &&  local.key.compare(predecessor.key)!=0 && local.key.compare(n_dash.key)!=0 && n_dash.key.compare(predecessor.key)!=0) {
        predecessor = n_dash;

       // cout << "Found Predecessor: " << predecessor.key << " : "  << predecessor.port << endl;

    }

}

void send_notify(Node succ) {
    int port = succ.port;
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    char resp[1024];
    serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    string message = xml_to_string(local.port, local.key, "NOTIFY");
    strcpy(resp, message.c_str());

    send(sock, resp, strlen(resp), 0);
    //close(sock);
}

Node encode(int port, string key) {
    Node n;
    n.port = port;
    strcpy(n.ip_addr, "127.0.0.1");
    n.key = key;

    return n;
}
// string getHashInt(long int i) {
//     string s = to_string(i);
//     char st[1024];
//     uint32_t key;
//     strcpy(st, s.c_str());
//     unsigned char hash[1024];
//     SHA1((unsigned char *)st, sizeof(st)-1, hash);
//     memcpy(&key, hash+16, sizeof(key));
//     return to_string(key);
// }

string getHashInt(long int i) {
    string s = to_string(i);
    SHA1 checksum;
    checksum.update(s.c_str());
    return checksum.final();
}
void *fix_fingers(void *ptr) {
    int i=0;
    while(true) {
        uint32_t local_key = atoi(local.key.c_str());
        long int power_2 = local_key + pow(2,i);
        finger_table[i] = fetch_successor(getHashInt(power_2));
        i = (i+1)%16;
        sleep(5);
    }
}

void *talk(void *client_fd) {
    // cout << "HEre" << endl;
    char message[1024];
    char response[1024];
    string resp;
    int client = *((int *)client_fd);
    //cout << client << endl;
    read(client , message, 1024);
    //cout << message << endl;
    
    while(1) {
        string messg = string(message);
        string mo = string_to_mess(messg);
        if(strcmp(message, "SUCC") == 0) {
            resp = to_string(successor.port);
            strcpy(response, resp.c_str());
            send(client, response, strlen(response), 0);
        }
        else if(mo.compare("FINDSUCC") == 0) {
            string mess = string(message);
            // cout << "Received FINDSUCC on " << 8001 << endl;
            
            string key  = string_to_key(mess);
           // cout << key << endl;
            // Call to fetch_successor with the key
            Node n = fetch_successor(key);
            //cout << "fetch successor returned" << endl;
            string s = xml_to_string(n.port, n.key, "FINDSUCC");
            char resp[1024];
            strcpy(resp, s.c_str());
            send(client, resp, strlen(resp), 0);
            
        }
        else if(mo.compare("NOTIFY") == 0) {
            // Read port from client
            // Read key from client
            
            int p = string_to_port(messg);
            
            string k = string_to_key(messg);

            Node n = encode(p, k);
           // cout << "notify received from " << p << endl;
            notify(n);

            // start transfer of keys
        }
        else if(mo.compare("PRED") == 0) {
            Node x;
            if(predecessor.key.compare("NULL") == 0) {
            
                x.port = 0;
                x.key = "NULL";
                strcpy(x.ip_addr, "127.0.0.1");
            }
            else {
                x = predecessor;
            }
            string respo = xml_to_string(x.port, x.key, "RETURN");
            char resp[1024];
            strcpy(resp, respo.c_str());
            send(client, resp, strlen(resp), 0);
        }
        else if(mo.compare("FINGER") == 0) {
            for (int i=1; i<=16; i++) {
                cout << i << " " << finger_table[i].port << endl;
            }
        }
        else if(messg.compare("INSERTKEYVAL")) {

        }
        else if(messg.compare("DELETEKEYVAL")) {

        }
        read(client, message, 1024);
    } 
}

void *listen_start(void *ptr) {
    int port = local.port;
    struct sockaddr_in address; 
    int sockfd, clientfd;
    int addrlen = sizeof(address); 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port);

    bind(sockfd, (struct sockaddr *)&address,  sizeof(address));

    listen(sockfd, 100);
    while (true) {
        clientfd = accept(sockfd, (struct sockaddr *)&address,  (socklen_t*)&addrlen);
       // cout << "Received Connection" << endl;
    //    int *arg = malloc(sizeof(&arg));
    //    *arg = clientfd;
        pthread_t talker_thread;
        pthread_create(&talker_thread, NULL, talk, &clientfd);
        
    }

}

Node find_successor(string key, int rem_port) {
    
    //cout << "Inside find_successor" << endl;
    //cout << rem_port << endl;
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char resp[1024];
    serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(rem_port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    int k = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    //cout << k << endl;
    //cout << "connected" << endl;
    char mess[1024];

    string message = xml_to_string(local.port, local.key, "FINDSUCC");
    strcpy(mess, message.c_str());
    send(client_fd, mess, strlen(mess), 0);
    
    
    char respo[1024];
    
    read(client_fd, respo, 1024);
    //close(client_fd);
    int porta = string_to_port(string(respo));
    string keym = string_to_key(string(respo));
    
    Node n = encode(porta, keym);
    //cout << "Received successor node" << endl;
    
    //cout << keym << endl;
    //cout << porta << endl;
    return n;
}
bool belongs_to(string key1, string n, string successor) {
    // cout << "Inside belongs to" << endl;
    uint32_t a,b,c;

    c = atoi(key1.c_str());
    b = atoi(successor.c_str());
    a = atoi(n.c_str());

    int size = 1<<16;
    
    c = c%size;
    a = a%size;
    b = b%size;

//    cout << "a: " << a << endl;
//    cout << "b: " << b << endl;
//    cout << "c: " << key << endl;
    
   if(a<b) {
       
       return (a<=c && c<b);

   }
   return (a<=c || c<b);

    // if(c>a && c<b) {
    //     return true;
    // }
}

// string getHash(char *ip, int port) {
//     char str[1024];
    
//     string port_str = to_string(port);
//     uint32_t key;
//     unsigned char hash[1024];
//     strcpy(str, ip);
//     strcat(str, ":");
//     strcat(str, port_str.c_str());
//     cout << str << endl;
//     SHA1((unsigned char *)str, sizeof(str)-1, hash);
//     memcpy(&key, hash + 16, sizeof(key));
//     return to_string(key);
// } 


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
    return checksum.final();
} 

void join(int rem_port, int port) {
   // cout << "Inside join" << endl;
    strcpy(local.ip_addr, "127.0.0.1");
    //cout << "Out of here" << endl; 
    local.port = port;
    
    string local_key = getHash("127.0.0.1", port);
    cout << "Hash of this node: " << local_key << endl;

    local.key = local_key;
    successor = find_successor(local_key, rem_port);
   // cout << "Successor found: " << successor.key << " : " << successor.port << endl;
    
    finger_table[1] = successor;
    predecessor.key = "NULL";
    for(int i=2; i<=16; i++) {
        finger_table[i].key = "NULL";
    }

    //send_notify(successor);
    pthread_create(&listen_thread, NULL, listen_start, (void *)local.port); 
    sleep(0.5); 
    pthread_create(&stabilize_thread, NULL, stabilize, (void *)local.port);
    sleep(0.5);
    pthread_create(&fixfinger_thread, NULL, fix_fingers, (void *)local.port);

    // start stabilizing
    // start fixfingers
}

Node closest_preceeding_node(string key) {
    //cout << "Inside closest" << endl; 
    for(int i=16; i>=1; i--) {
        if(finger_table[i].key.compare("NULL") != 0) {
            if(belongs_to(finger_table[i].key, local.key, key)) {
                //cout << "Belongs to returned true" << endl;
                return finger_table[i];
            }
        }
    }
    return local;
}

Node fetch_successor(string key) {
   // cout << "Inside fetch successoor" << endl;
    if(belongs_to(key, local.key, successor.key) && local.key.compare(successor.key)!=0 && local.key.compare(key)!=0 && key.compare(successor.key)!=0) {
        // return successor
        //cout << "Inside if" << endl;
        return successor;
    }
    else {
        //cout << "Inside else of fetch_successor" << endl;
        Node n_dash = closest_preceeding_node(key);
        if(n_dash.port == local.port) {
            return local;
        }
        //cout << "Sending request to " << n_dash.port << endl;
        char key_str[1024];
        strcpy(key_str, key.c_str());

        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serv_addr;
        char resp[1024];
        serv_addr.sin_family = AF_INET; 
	    serv_addr.sin_port = htons(n_dash.port);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        
        // send a request to n_dash with key

        string req = xml_to_string(local.port, key,"FINDSUCC");
        strcpy(resp, req.c_str());
        send(client_fd, resp, strlen(resp), 0);
        
        read(client_fd, resp, 1024);
       // close(client_fd);
        string fo = string(resp);
        int porta = string_to_port(fo);
        string keym = string_to_key(fo);

        return encode(porta, key);

    }
}

void initialize_chord_ring(int portno) {

    local.port = portno;
    
    local.key = getHash("127.0.0.1", portno);
    cout << local.key << endl;
    strcpy(local.ip_addr, "127.0.0.1");
    for(int i=1; i<=16; i++) {
        finger_table[i].key = "NULL";
    }
    finger_table[1] = local;
   successor = local;
    predecessor.key = "NULL";
     
    pthread_create(&listen_thread, NULL, listen_start, (void *)local.port);
    sleep(0.5);
    pthread_create(&stabilize_thread, NULL, stabilize, (void *)local.port);
    sleep(0.5);
    pthread_create(&fixfinger_thread, NULL, fix_fingers, (void *)local.port);
    
}

int main(int argc, char **argv) {
    //int port = atoi(argv[0]);

    if(argc == 2) {
        // cout << "Here" << endl;
        initialize_chord_ring(atoi(argv[1]));
    }
    else {
        // cout << "Here1" << endl;
        int rem_port = atoi(argv[1]);
        int port = atoi(argv[2]);
        
        join(rem_port, port);
    }
    //pthread_join(listen_thread, NULL);
    //pthread_join(stabilize_thread, NULL);
    while(1);
    return 0;
}