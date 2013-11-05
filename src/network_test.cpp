#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <string.h>
#include <string>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

using namespace std;

inline void err_exit(const int err)
{
    cerr << strerror(err) << endl;
    exit(1);
}

int main()
{
    int pfds[2] = {0};

    if (pipe(pfds) < 0)
    {
        err_exit(errno);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        err_exit(errno);
    }
    else if (pid == 0) // child
    {
        close(pfds[0]);

        sleep(1);
        const string str("hello");
        write(pfds[1], str.c_str(), str.size());
        cout << "child: wrote " << str << " to pipe" << endl;
        return 0;
    }
    else // parent
    {
        close(pfds[1]);
        const int pflag = fcntl(pfds[0], F_GETFL);
        fcntl(pfds[0], F_SETFL, pflag | O_NONBLOCK);

        fd_set rfds;
        int n = 0;
        while (true)
        {
            FD_ZERO(&rfds);
            FD_SET(pfds[0], &rfds);
            n = select(pfds[0] + 1, &rfds, NULL, NULL, NULL);

            if (n < 0)
            {
                err_exit(errno);
            }
            else if (n == 0)
            {
                cerr << "parent: impossible to reach here" << endl;
                exit(1);
            }
            else
            {
                if (FD_ISSET(pfds[0], &rfds))
                {
                    char c;
                    n = read(pfds[0], &c, 1);
                    if (n == 0)
                    {
                        cout << "parent: child has close the pipe" << endl;
                        return 0;
                    }
                    else
                    {
                        cout << "parent: read '" << c << "' from the pipe" << endl;
                        sleep(1);
                    }
                }
                else
                {
                    cerr << "parent: pfds[0] is unreadable" << endl;
                }
            }
        }
    }

    cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
    return 0;
}
