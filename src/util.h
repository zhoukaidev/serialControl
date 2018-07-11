#pragma once

#include <future>
#include <mutex>
#include <queue>
namespace scl
{

	template <class T>
	class Thread_safe_Queue {
	public:
		Thread_safe_Queue() {

		}
		~Thread_safe_Queue() {

		}
		void push(T val) {
			std::lock_guard<std::mutex> lk(mQueueMx);
			mdataQueue.push(val);
			mQueueCv.notify_one();
		}

		T wait_and_pop() {
			std::unique_lock<std::mutex> lk(mQueueMx);
			mQueueCv.wait(lk, [this] {return !mdataQueue.empty(); });
			T retval = mdataQueue.front();
			mdataQueue.pop();
			return retval;
		}
		bool empty() {
			std::lock_guard<std::mutex> lk(mQueueMx);
			return mdataQueue.empty();
		}
		void clear() {
			std::lock_guard<std::mutex> lk(mQueueMx);
			while (!mdataQueue.empty())
				mdataQueue.pop();
		}
	private:
		std::queue<T> mdataQueue;
		std::mutex mQueueMx;
		std::condition_variable mQueueCv;
	};

}

