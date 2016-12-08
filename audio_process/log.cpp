/* -----------------------------
* File   : log.cpp
* Author : Yuki Chai
* Created: 2016.11.28
* Project: Yuki
*/
#include "log.h"

namespace Yuki {
    LOG *LOG::instance = NULL;
    const char * _operations[] = {
        " LE ",
        " LT ",
        " GE ",
        " GT ",
        " EQ ",
        " NE "
    };
}