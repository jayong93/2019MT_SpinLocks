#include "Locks.h"

thread_local std::random_device BackOffLock::BackOff::rd;
thread_local std::default_random_engine BackOffLock::BackOff::re{BackOffLock::BackOff::rd()};
thread_local std::uniform_int_distribution<unsigned int> BackOffLock::BackOff::dist;
thread_local int ArrayLock::my_index = 0;
thread_local int ArrayPaddingLock::my_index = 0;
thread_local std::atomic_bool* CLHLock::my_node = new std::atomic_bool{ true };
thread_local std::atomic_bool* CLHLock::my_pred = nullptr;
thread_local MCSLock::QNode* MCSLock::my_node = new MCSLock::QNode;
