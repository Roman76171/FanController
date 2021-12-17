#include "FanSpecification.h"

FanSpecification::FanSpecification(const int64_t minRpm, const int64_t maxRpm) noexcept : m_maxRpm{ maxRpm }, m_minRpm{ minRpm }
{

}

int64_t FanSpecification::getMaxRpm() const  noexcept
{
	return m_maxRpm;
}

int64_t FanSpecification::getMinRpm() const  noexcept
{
	return m_minRpm;
}