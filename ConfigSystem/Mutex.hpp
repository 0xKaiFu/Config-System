#pragma once

template <typename T>
class MutexLock {
private:
	bool* isLockedPtr;
public:
	T* data;
	MutexLock(bool* lockPtr, T* _data) {
		isLockedPtr = lockPtr;
		data = _data;
	}
	~MutexLock() {
		*isLockedPtr = false;
	}
};

class Mutex {
private:
	bool isLocked = false;
public:
	MutexLock<void> lock()
	{
		while (isLocked) {}
		isLocked = true;
		return MutexLock<void>(&isLocked, 0);
	}
	void unlock() {
		isLocked = false;
	}
	bool* lockPtr() {
		return &isLocked;
	}
};

template <typename T>
class ProtectedResource {
private:
	Mutex mutex;
	T internalData;
public:
	ProtectedResource(T initialData) {
		internalData = initialData;
	}

	MutexLock<T> get() { return MutexLock<T>(mutex.lockPtr(), &internalData); }
};