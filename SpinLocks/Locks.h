#pragma once
#include <random>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>
#include <vector>

class BackOffLock {
public:
	BackOffLock(int _=0) {}
	void lock() {
		BackOff backoff{ MIN_DELAY, MAX_DELAY };
		while (true) {
			while (state.load()) {}
			if (!state.exchange(true)) {
				return;
			}
			else {
				backoff.back_off();
			}
		}
	}

	void unlock() {
		state.store(false);
	}

private:
	std::atomic_bool state{ false };
	static constexpr int MAX_DELAY = 4000;
	static constexpr int MIN_DELAY = 50;

	class BackOff {
	public:
		BackOff(int min, int max) : limit{ min }, dist{ 0, limit }, max_delay{ max } { }

		void back_off() {
			int delay = dist(re);
			limit = std::min(max_delay, limit * 2);
			dist = std::uniform_int_distribution<int>{ 0, limit };
			std::chrono::milliseconds sleep_time{ delay };
			std::this_thread::sleep_for(sleep_time);
		}
	private:
		int max_delay, limit;
		std::random_device rd;
		std::default_random_engine re{rd()};
		std::uniform_int_distribution<int> dist;
	};
};

class ArrayLock {
public:
	ArrayLock(int capacity) : size{ capacity }, flags{ new bool[size] } {
		flags[0] = true;
		my_index = 0;
	}
	~ArrayLock() {
		delete[] flags;
	}
	void lock() {
		int slot = tail.fetch_add(1) % size;
		my_index = slot;
		while (!flags[slot]) {}
	}
	void unlock() {
		int slot = my_index;
		flags[slot] = false;
		flags[(slot + 1) % size] = true;
	}
	
private:
	thread_local static int my_index;
	std::atomic_int tail{ 0 };
	volatile bool* flags;
	int size;
};

class ArrayPaddingLock {
public:
	ArrayPaddingLock(int capacity) : size{ capacity }, flags{ new bool[size*64] } {
		flags[0] = true;
		my_index = 0;
	}
	~ArrayPaddingLock() {
		delete[] flags;
	}
	void lock() {
		int slot = tail.fetch_add(1) % size;
		my_index = slot;
		while (!flags[slot*64]) {}
	}
	void unlock() {
		int slot = my_index;
		flags[slot*64] = false;
		flags[((slot + 1) % size)*64] = true;
	}
	
private:
	thread_local static int my_index;
	std::atomic_int tail{ 0 };
	volatile bool* flags;
	int size;
};
