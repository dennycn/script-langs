/* -*- mode: c -*-
 * $Id: echo.gcc,v 1.9 2001/06/28 03:04:47 doug Exp $
 * http://www.bagley.org/~doug/shootout/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>


typedef int (*SOCKACTION_P)(int,struct sockaddr *,socklen_t);
#define DATA "Hello there sailor\n"

void myabort (char *m) {
    fprintf(stderr, "%s\n", m);
    exit(1);
}
void sysabort (char *m) {
    perror(m);
    exit(1);
}

int sigchld = 0;
void reaper (int sig) {
    sigchld = 1;
}

int
genericSock(int port,SOCKACTION_P action,char *actionExceptionText) {
    int ss, optval = 1;
    struct sockaddr_in sin;
    if ((ss = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        sysabort("socket");
    if (setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        sysabort("setsockopt");
    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin.sin_port = port;
    if (action(ss, (struct sockaddr *)&sin,(socklen_t)sizeof(sin)) == -1)
        sysabort(actionExceptionText);

    return(ss);
}

int
server_sock () {
    int ss = genericSock(0,(SOCKACTION_P)bind,"server/bind");
    return(listen(ss,2),ss);
}

int
client_sock (int port) {
    return(genericSock(port,(SOCKACTION_P)connect,"client/connect"));
}

int
get_port (int sock) {
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    if (getsockname(sock, (struct sockaddr *)&sin, &slen) == -1)
        sysabort("server/getsockname");
    return(sin.sin_port);
}

void
echo_client (int n, int port) {
    int i, sock, olen, len, nwritten, nread;
    char *offset, obuf[64], ibuf[64];
    char *end = ibuf + sizeof(ibuf);

    sock = client_sock(port);
    strcpy(obuf, DATA);
    olen = strlen(obuf);
    for (i=0; i<n; i++) {
        len = olen;
        offset = obuf;
        while (len > 0) {
            if ((nwritten = write(sock, offset, len)) == -1)
                sysabort("client/write");
            offset += nwritten;
            len -= nwritten;
        }
        offset = ibuf;
        while ((nread = read(sock, offset, (end - offset))) > 0) {
            offset += nread;
            if (*(offset-1) == '\n') break;
        }
        if (nread == -1)
            sysabort("client/read");
        *offset = 0;
        if ((strcmp(obuf, ibuf)) != 0) {
            char mbuf[128];
            sprintf(mbuf, "client: \"%s\" ne \"%s\"", obuf, ibuf);
            myabort(mbuf);
        }
    }
    close(sock);
}

void
echo_server (int n) {
    int ssock, csock, len, nwritten, total_bytes;
    pid_t pid;
    char buf[64], *offset;
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    int status;

    ssock = server_sock();
    signal(SIGCHLD, reaper);
    if ((pid = fork()) == -1)
        sysabort("server/fork");
    if (pid) {

        if ((csock = accept(ssock, (struct sockaddr *)&sin, &slen)) == -1)
            sysabort("server/accept");
        total_bytes = 0;
        while ((len = read(csock, buf, sizeof(buf))) > 0) {
            if (sigchld) myabort("server/sigchld");
            offset = buf;
            total_bytes += len;
            while (len > 0) {
                if ((nwritten = write(csock, offset, len)) == -1)
                    sysabort("server/write");
                offset += nwritten;
                len -= nwritten;
            }
        }
        if (len == -1)
            sysabort("server/read");
        close(csock);
        fprintf(stdout, "server processed %d bytes\n", total_bytes);
    } else {

        echo_client(n, get_port(ssock));
    }
    wait(&status);
}

int
main(int argc, char *argv[]) {
    echo_server((argc == 2) ? atoi(argv[1]) : 1);
    return(0);
}

