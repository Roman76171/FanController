#pragma once

#ifndef FAN_H

#define FAN_H

#include <cstdint>
#include <chrono>
#include "FanSpecification.h"

class Fan
{
public:
	using percent_t = unsigned short;

	Fan() noexcept;
	Fan(const int pwmPin, const FanSpecification& fanSpec) noexcept;
	//Fan(const int rpmPin, const FanSpecification& fanSpec) noexcept;
	Fan(const int pwmPin, const int rpmPin, const FanSpecification& fanSpec) noexcept;
	Fan(Fan&& fan) noexcept;
	~Fan() noexcept;
	Fan& operator=(Fan&& fan) noexcept;
	int64_t getRpm() const;
	void setSpeed(const percent_t speed);

private:
	Fan(const Fan&) = delete;
	Fan& operator=(const Fan&) = delete;

	void setPwmSignal(int requiredSpeed, std::chrono::seconds StepTime=std::chrono::seconds(12)) noexcept;
	inline bool isInterrupt(const std::chrono::milliseconds& timeout) const;
	void reset() noexcept;
	int64_t rpmMeasurement() const;
	void setupPwmPin(const int pin) noexcept;
	void setupRpmPin(const int pin) const noexcept;

	FanSpecification m_fanSpecification;
	int m_pwmPin;
	int m_rpmPin;
	int m_speed;
};

#endif // !FAN_H