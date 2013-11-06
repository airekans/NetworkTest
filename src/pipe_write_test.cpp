#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <string.h>
#include <string>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <fcntl.h>

using namespace std;

static inline void err_exit(const int err)
{
    cerr << strerror(err) << endl;
    exit(1);
}

int select_fd(int fd)
{
    const int pflag = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, pflag | O_NONBLOCK);

    cout << "child: prepare to select the writable fd" << endl;
    fd_set wfds;
    int n = 0;
    int total = 0;
    //        string cs(1024 * 8, 'a');
    string cs(1024, 'a');
    while (true)
    {
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        n = select(fd + 1, NULL, &wfds, NULL, NULL);

        if (n < 0)
        {
            err_exit(errno);
        }
        else if (n == 0)
        {
            cerr << "child: impossible to reach here" << endl;
            exit(1);
        }
        else
        {
            if (FD_ISSET(fd, &wfds))
            {
                n = write(fd, cs.c_str(), cs.size());
                if (n < 0)
                {
                    if (errno == EAGAIN)
                    {
                        cout << "child: pipe is not full and unable to write" << endl;
                        return 0;
                    }
                }
                else if (n == 0)
                {
                    cout << "child: cannot write to fd" << endl;
                    exit(1);
                }
                else
                {
                    total += n;
                    cout << "child: write " << n << " bytes, total " << total << endl;
                    sleep(1);
                }
            }
            else
            {
                cerr << "child: pfds[1] is unwritable" << endl;
                exit(1);
            }
        }
    }

    return 0;
}

namespace {
    class Epoll {
        int m_epoll_fd;
    public:
        Epoll(const int size)
        {
            m_epoll_fd = epoll_create(size);
            if (m_epoll_fd < 0)
            {
                strerror(errno);
            }
        }

        int get_fd() const { return m_epoll_fd; }

        ~Epoll()
        {
            close(m_epoll_fd);
        }
    };
}

void fd_writable(const int fd, const string& cs, int& total)
{
    int n = write(fd, cs.c_str(), cs.size());
    if (n < 0)
    {
        if (errno == EAGAIN)
        {
            cout << "child: pipe is not full and unable to write" << endl;
            exit(0);
        }
    }
    else if (n == 0)
    {
        cout << "child: cannot write to fd" << endl;
        exit(1);
    }
    else
    {
        total += n;
        cout << "child: write " << n << " bytes, total " << total << endl;
        sleep(1);
    }
}

int epoll_fd(int fd)
{
    Epoll ep(10);
    int epoll_fd = ep.get_fd();

    struct epoll_event ev, events[10];

//    ev.events = EPOLLOUT | EPOLLET;
    ev.events = EPOLLOUT;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        strerror(errno);
    }

    int res = 0;
    int total = 0;
    string cs(1024, 'a');
    while (true)
    {
        res = epoll_wait(epoll_fd, events, 10, -1);
        if (res < 0)
        {
            strerror(errno);
        }

        for (int i = 0; i < res; ++i)
        {
            fd_writable(events[i].data.fd, cs, total);
        }
    }

    return 0;
}

int pipe_write_test_main()
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

//        return select_fd(pfds[1]);
        return epoll_fd(pfds[1]);
    }
    else // parent
    {
        close(pfds[1]);
        pid = wait(NULL);
        cout << "parent: child " << pid << " has exited" << endl;
        return 0;
    }

    cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
    return 0;
}
