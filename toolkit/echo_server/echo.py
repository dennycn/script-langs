#!/usr/bin/python
# $Id: echo.python,v 1.7 2001/05/06 22:21:26 doug Exp $
# see: http://www.bagley.org/~doug/shootout/
# with help from Brad Knotwell
# usage: $0 [bytes]
# note: run on unix/linux
# date: 2017/9/6

import sys
import os
from socket import *

DATA = "Hello there sailor\n"
bufferSize = len(DATA)


def server_sock():
    sock = socket(AF_INET, SOCK_STREAM)
    sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    sock.bind(('127.0.0.1', 0))
    sock.listen(2)
    return(sock)


def get_port(sock):
    host, port = sock.getsockname()
    return(port)


def client_sock(port):
    sock = socket(AF_INET, SOCK_STREAM)
    sock.connect(('127.0.0.1', port))
    return(sock)


def echo_client(n, port):
    sock = client_sock(port)
    sender, receiver = sock.send, sock.recv
    for i in range(0, n):
        sender(DATA)
        ans = receiver(bufferSize)
        while ans[-1] != "\n":
            ans += receiver(bufferSize - len(ans))
            if ans <> DATA:
                raise("client: \"%s\" ne \"%s\"" % (DATA, ans))
                sock.close()


def echo_server(n):
    ssock = server_sock()
    if os.fork() > 0:
        # parent is server
        csock, addr = ssock.accept()
        n = 0
        sender, receiver = csock.send, csock.recv
        while 1:
            dat = receiver(bufferSize)
            if not dat:
                break
            sender(dat)
            n += len(dat)
            print "server processed %d bytes" % n
            os.wait()
    else:
        # child is client
        echo_client(n, get_port(ssock))


def main():
    n = int(sys.argv[1])
    if n < 1:
        n = 1
    echo_server(n)


if __name__ == "__main__":
    main()
