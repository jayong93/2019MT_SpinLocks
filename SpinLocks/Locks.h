#pragma once
#include <random>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>
#include <vector>

class BackOffLock {
public:
	BackOffLock(int _ = 0) {}
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
	static constexpr int MAX_DELAY = 400;
	static constexpr int MIN_DELAY = 50;

	class BackOff {
	public:
		BackOff(int min, int max) : limit{ min }, max_delay{ max } { }

		void back_off() {
			int delay = dist(re) % limit;
			limit = std::min(max_delay, limit * 2);
			std::chrono::milliseconds sleep_time{ delay };
			std::this_thread::sleep_for(sleep_time);
		}
	private:
		int max_delay, limit;
		thread_local static std::random_device rd;
		thread_local static std::default_random_engine re;
		thread_local static std::uniform_int_distribution<unsigned int> dist;
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
	ArrayPaddingLock(int capacity) : size{ capacity }, flags{ new bool[size * 64] } {
		flags[0] = true;
		my_index = 0;
	}
	~ArrayPaddingLock() {
		delete[] flags;
	}
	void lock() {
		int slot = tail.fetch_add(1) % size;
		my_index = slot;
		while (!flags[slot * 64]) {}
	}
	void unlock() {
		int slot = my_index;
		flags[slot * 64] = false;
		flags[((slot + 1) % size) * 64] = true;
	}

private:
	thread_local static int my_index;
	std::atomic_int tail{ 0 };
	volatile bool* flags;
	int size;
};

class CLHLock {
public:
	CLHLock(int _ = 0) : tail{ (uintptr_t)new std::atomic_bool{false} } {}
	~CLHLock()
	{
		auto ptr = (std::atomic_bool*)tail.load();
		if (ptr != nullptr)
			delete ptr;
	}
	void lock() {
		auto qnode = my_node;
		qnode->store(true);
		std::atomic_bool* pred = (std::atomic_bool*)tail.exchange((uintptr_t)qnode);
		my_pred = pred;
		while (pred->load()) {}
	}

	void unlock() {
		auto qnode = my_node;
		qnode->store(false);
		my_node = my_pred;
	}

private:
	thread_local static std::atomic_bool* my_pred;
	thread_local static std::atomic_bool* my_node;
	std::atomic_uintptr_t tail;
};

class MCSLock {
public:
	MCSLock(int _ = 0) : tail{ (uintptr_t)nullptr } {}
	~MCSLock()
	{
		auto ptr = (QNode*)tail.load();
		if (ptr != nullptr)
			delete ptr;
	}
	void lock() {
		auto qnode = my_node;
		auto pred = (QNode*)tail.exchange((uintptr_t)qnode);
		if (pred != nullptr) {
			qnode->locked.store(true);
			pred->next = qnode;
			while (qnode->locked.load()) {}
		}
	}

	void unlock() {
		auto qnode = my_node;
		if (qnode->next == nullptr) {
			auto cur_tail = qnode;
			if (tail.compare_exchange_strong((uintptr_t&)cur_tail, (uintptr_t)nullptr)) {
				return;
			}
			while (qnode->next == nullptr) {}
		}
		qnode->next->locked.store(false);
		qnode->next = nullptr;
	}

private:
	struct QNode {
		std::atomic_bool locked{ false };
		QNode* volatile next{ nullptr };
	};
	thread_local static QNode* my_node;
	std::atomic_uintptr_t tail;
};