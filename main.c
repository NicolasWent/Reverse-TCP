#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <err.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    if (argc != 2)
        errx(EXIT_FAILURE, "Usage:\n"
            "Arg 1 = Port number (e.g. 2048)");

    struct addrinfo *result;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // get address info

    if (getaddrinfo(NULL, argv[1], &hints, &result) != 0)
    {
        errx(EXIT_FAILURE, "Fail oppening server on port %s",
                argv[1]);
    }

    // loop until we find a response
    int cnx;
    struct addrinfo *rp;
    struct sockaddr addr;
    socklen_t addrlen;
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        cnx = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (cnx == -1) continue;
        int v = 1;
        if (setsockopt(cnx, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(int)) == -1)
            errx(EXIT_FAILURE, "Fail on setsockopt");
        if (bind(cnx, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            addr = *rp->ai_addr;
            addrlen = rp->ai_addrlen;
            break;
        }
        close(cnx);
    }

    // care it can break the rp->ai_addr;
    freeaddrinfo(result);

    if (rp == NULL)
    {
        errx(1, "No socket has been opened");
    }

    if (listen(cnx, 5) == -1)
    {
        errx(EXIT_FAILURE, "Impossible to listen");
    }
    for (;;)
    {
        int newsocket = accept(cnx, &addr, &addrlen);
        if (newsocket == -1)
        {
            errx(1, "fail to accept a connection");
        }
        if (fork())
        {
            close(newsocket);
            signal(SIGCHLD, SIG_IGN);
            continue;
        }
        else
        {
            close(cnx);
            dup2(newsocket, STDIN_FILENO);
            dup2(newsocket, STDOUT_FILENO);
            dup2(newsocket, STDERR_FILENO);
            close(newsocket);
            char *args[] = { "bash", NULL };
            execvp("bash", args);
            // execlp("i3-sensible-terminal", "i3-sensible-terminal",  NULL);
            return 0;
        }
    }
    close(cnx);
}
