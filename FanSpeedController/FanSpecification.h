#pragma once

#ifndef FAN_SPECIFICATION_H

#define FAN_SPECIFICATION_H

#include <cstdint>

class FanSpecification
{
public:
	FanSpecification() = default;
	FanSpecification(const int64_t minRpm, const int64_t maxRpm) noexcept;
	int64_t getMaxRpm() const noexcept;
	int64_t getMinRpm() const noexcept;
	
private:
	int64_t m_maxRpm;
	int64_t m_minRpm;
};

#endif // !FAN_SPECIFICATION_H
