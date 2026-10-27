#pragma once
#include <fstream>
#include <iostream>

template<typename T>
class Singleton
{
public:
	static T& getInstance()
	{
		static T instance;
		return
	}
};


class LogBuffer
{
	std::ostream& m_stream;


public:

	LogBuffer()
		: m_stream(std::cout)
	{
	}

	LogBuffer(std::ostream& stream)
		: m_stream{ stream }
	{
	}

	//void setStream(std::ostream os)
	//{
	//	m_stream = &os;
	//}

	std::ostream& logOut()
	{
		return m_stream;
	}

	LogBuffer& operator << (const std::string& outMessage)
	{
		m_stream << outMessage;
	}
};
