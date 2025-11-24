//=================================================================================================
//
// \file    utils.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <termios.h>

#include "utils.h"

//
//
//

static struct termios gTermiosOld;
static struct termios gTermiosCurrent;

//
// Initialize new terminal i/o settings
//

static void InitTermios(int Echo)
{
    tcgetattr(0, &gTermiosOld);         // grab old terminal i/o settings
    gTermiosCurrent = gTermiosOld;      // make new settings same as old settings
    gTermiosCurrent.c_lflag &= ~ICANON; // disable buffered i/o

    if (Echo) {
        gTermiosCurrent.c_lflag |= ECHO; // set echo mode
    }

    else {
        gTermiosCurrent.c_lflag &= ~ECHO; // set no echo mode
    }

    tcsetattr(0, TCSANOW, &gTermiosCurrent); // use these new terminal i/o settings now
}

//
// restore old terminal i/o settings
//

static void ResetTermios(void)
{
    tcsetattr(0, TCSANOW, &gTermiosOld);
}

//
// read 1 character - echo defines echo mode
//

static char getch_(int echo)
{
    char ch;
    InitTermios(echo);
    ch = getchar();
    ResetTermios();

    return ch;
}

//
// Read 1 character without echo
//

char getch(void)
{
    return getch_(0);
}

//=================================================================================================
