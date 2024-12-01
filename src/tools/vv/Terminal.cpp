#include <array>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "Terminal.hpp"
#include "util/Panic.hpp"

namespace
{
constexpr std::array termFileNo = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };

static int s_termFd = -1;
static struct termios s_termSave;
}

bool OpenTerminal()
{
    CheckPanic( s_termFd < 0, "Terminal already open" );

    int fd = -1;
    for( auto termfd : termFileNo )
    {
        if( isatty( termfd ) )
        {
            auto name = ttyname( termfd );
            if( name )
            {
                fd = open( name, O_RDWR );
                if( fd != -1 )
                {
                    mclog( LogLevel::Info, "Opened terminal: %s", name );
                    break;
                }
            }
        }
    }
    if( fd < 0 ) return false;

    if( tcgetattr( fd, &s_termSave ) != 0 )
    {
        close( fd );
        return false;
    }

    struct termios tio = s_termSave;
    tio.c_lflag &= ~( ICANON | ECHO );
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if( tcsetattr( fd, TCSANOW, &tio ) != 0 )
    {
        close( fd );
        return false;
    }

    s_termFd = fd;
    return true;
}

void CloseTerminal()
{
    CheckPanic( s_termFd >= 0, "Terminal not open" );
    tcsetattr( s_termFd, TCSAFLUSH, &s_termSave );
    close( s_termFd );
    s_termFd = -1;
}

std::string QueryTerminal( const char* query )
{
    CheckPanic( s_termFd >= 0, "Terminal not open" );

    const auto sz = strlen( query );
    if( write( s_termFd, query, sz ) != sz ) return {};

    return QueryTerminal();
}

std::string QueryTerminal()
{
    std::string ret;
    char buf[1024];
    while( true )
    {
        struct pollfd pfd = { .fd = s_termFd, .events = POLLIN };
        const auto pr = poll( &pfd, 1, 1000 );
        if( pr < 0 ) return {};
        if( pr == 0 ) break;

        const auto rd = read( s_termFd, buf, sizeof( buf ) );
        if( rd < 0 ) return {};
        ret.append( buf, rd );
        if( rd < sizeof( buf ) ) break;
    }

    return ret;
}
