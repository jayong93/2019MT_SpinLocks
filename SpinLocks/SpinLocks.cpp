#include <iostream>
#include <vector>
#include "Locks.h"

using Lock = CLHLock;

volatile int sum = 0;

int main()
{
	for (auto i = 1; i <= 32; i *= 2) {
		Lock lock{i};
		std::vector<std::thread> thread_vec;

		auto start_time = std::chrono::high_resolution_clock::now();
		for (auto j = 0; j < i; j++)
		{
			thread_vec.emplace_back([i, &lock]() {
				for (auto k = 0; k < 50000000 / i; ++k) {
					lock.lock();
					sum += 2;
					lock.unlock();
				}
			});
		}
		for (auto& t : thread_vec) t.join();
		auto end_time = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
		printf("Thread Num: %d, Sum: %d, time: %lld ms\n", i, sum, duration);

		sum = 0;
	}
}
