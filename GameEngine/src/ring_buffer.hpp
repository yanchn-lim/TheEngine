#pragma once

template<typename T,size_t N>
struct RingBuffer
{
	std::array<T, N> data{};
	size_t head{ 0 };
	size_t count{ 0 };

	//push into latest
	void Push(T value)
	{
		//assign a value to the head
		data[head] = value;
		//updt head and clamp to N
		head = (head + 1) % N;
		//keep count
		if (count < N) ++count;
	}

	//copy from array into the ring buffer
	void CopyOrdered(std::array<T, N>& dst) const
	{
		for (size_t i = 0; i < N; ++i)
			dst[i] = data[(head + i) % N];
	}

	//return latest
	T Last() const
	{
		return count ? data[(head + N - 1) % N] : T{};
	}
};