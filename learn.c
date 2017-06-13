//
//  main.c
//  learn_socket
//
//  Created by ~GG~ on 3/15/17.
//  Copyright © 2017 ~GG~. All rights reserved.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

#define MAXBUF 4096

typedef struct handler {
    int sockIn;
    int sockOut;
} handler_t;

void proses_client(void *h);
void proses_proxy(void *h);

pthread_t clthr, prthr;

int main(int argc, const char * argv[]) {
    int socketServer;
    struct sockaddr_in addrServer;
    int error;
    // Test set signal handler untuk SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    // Buat socket untuk server
    socketServer = socket(AF_INET, SOCK_STREAM, 0);
    // Buat struktur address
    bzero(&addrServer, sizeof(addrServer));
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(8080);
    addrServer.sin_addr.s_addr = INADDR_ANY;
    // Bind struktur ke socket (casting ke sockaddr)
    error = bind(socketServer, (struct sockaddr *)&addrServer, sizeof(addrServer));
    printf("Bind: %d\n", error);
    // Listen port
    error = listen(socketServer, 5);
    printf("Listen: %d\n", error);
    // Handle koneksi masuk dengan infinite loop
    // accept() akan tertrigger ketika ada koneksi masuk pada socketServer
    // dan memberikan return nilai int yang merupakan fd (file descriptor)
    // dari socket baru yang terbuat
    for(;;){
        // Handle koneksi masuk
        int socketClient;
        struct sockaddr_in addrClient;
        socklen_t sizeSockIn = sizeof(addrClient);
        socketClient = accept(socketServer, (struct sockaddr *)&addrClient, &sizeSockIn);
        // Jika ada koneksi masuk, langsung buat sambungan ke proxy parent
        int socketProxy;
        int error;
        struct sockaddr_in addrProxy;
        bzero(&addrProxy, sizeof(addrProxy));
        addrProxy.sin_family = AF_INET;
        addrProxy.sin_port = htons(3128); // hardcode untuk permulaan
        addrProxy.sin_addr.s_addr = inet_addr("192.168.3.33");
        // Buat socket untuk proxy
        socketProxy = socket(AF_INET, SOCK_STREAM, 0);
        // Saatnya konek!
        error = connect(socketProxy, (struct sockaddr *)&addrProxy, sizeof(addrProxy));
        if(error)
            printf("Cannot connect to proxy! %d\n", error);
        // Jika semua lancar maka kita selanjutnya handle data masuk
        // jadi, kita hanya dilewati saja
        handler_t* handleclient;
        handler_t* handleproxy;
        handleclient = (handler_t*)malloc(sizeof(handler_t));
        handleproxy = (handler_t*)malloc(sizeof(handler_t));
        // Data dari client ke proxy
        handleclient->sockIn = socketClient;
        handleclient->sockOut = socketProxy;
        pthread_create(&clthr, NULL, (void*) &proses_client, (void*) handleclient);
        // Data dari proxy ke client
        handleproxy->sockIn = socketProxy;
        handleproxy->sockOut = socketClient;
        pthread_create(&prthr, NULL, (void*) &proses_proxy, (void*) handleproxy);
    }
    return 0;
}

void proses_client(void *h){
    ssize_t n = 0;
    handler_t* handle;
    handle = (handler_t*)h;
    char buf[MAXBUF+1];
    while((n = read(handle->sockIn, buf, MAXBUF)) > 0){
        buf[n] = 0;
        // Header modification here
        write(handle->sockOut, buf, n);
    }
}

void proses_proxy(void *h){
    ssize_t n = 0;
    handler_t* handle;
    handle = (handler_t*)h;
    char buf[MAXBUF+1];
    while((n = read(handle->sockIn, buf, MAXBUF)) > 0){
        buf[n] = 0;
        write(handle->sockOut, buf, n);
    }
}
