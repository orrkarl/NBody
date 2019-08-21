#pragma once

template <typename T>
class DoubleBuffer
{
public:
	DoubleBuffer(T buf0, T buf1)
		: m_buffers{buf0, buf1}, m_swapped(false)
	{
	}

	T& front() { return m_buffers[1 - int(m_swapped)]; }
	const T& front() const { return m_buffers[1 - int(m_swapped)]; }

	T& back() { return m_buffers[int(m_swapped)]; }
	const T& back() const { return m_buffers[int(m_swapped)]; }

	void swap()
	{
		m_swapped = !m_swapped;
	}

private:
	T	 m_buffers[2];
	bool m_swapped;
};