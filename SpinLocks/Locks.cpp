#include "Locks.h"

thread_local int ArrayLock::my_index = 0;
thread_local int ArrayPaddingLock::my_index = 0;
