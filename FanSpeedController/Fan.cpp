#include <pthread.h>
#include <climits>
#include <cstdint>
#include <cmath>
#include <utility>
#include <thread>
#include <wiringPi.h>
#include "Fan.h"

#include <iostream>

/*
* ************** *
* PUBLIC METHODS *
* ************** *
*/

Fan::Fan() noexcept : m_fanSpecification{}, m_pwmPin{ -1 }, m_rpmPin{ -1 }, m_speed{ -1 }
{

}

Fan::Fan(const int pwmPin, const FanSpecification& fanSpec) noexcept : m_fanSpecification{ fanSpec }, m_pwmPin{ pwmPin }, m_rpmPin{ -1 }, m_speed{ -1 }
{
	setupPwmPin(pwmPin);
}

/*Fan::Fan(const int rpmPin, const FanSpecification& fanSpec) noexcept : m_fanSpecification{fanSpec}, m_pwmPin{-1}, m_rpmPin{rpmPin}, m_speed{-1}
{
	setupRpmPin(rpmPin);
}*/

Fan::Fan(const int pwmPin, const int rpmPin, const FanSpecification& fanSpec) noexcept : m_fanSpecification{ fanSpec }, m_pwmPin{ pwmPin }, m_rpmPin{ rpmPin }, m_speed{ -1 }
{
	setupRpmPin(rpmPin);
	setupPwmPin(pwmPin);
}

Fan::Fan(Fan&& fan) noexcept : m_fanSpecification{ fan.m_fanSpecification }, m_pwmPin{ fan.m_pwmPin }, m_rpmPin{ fan.m_rpmPin }, m_speed{ -1 }
{
	int speed{ this->m_speed };
	fan.~Fan();
	if (this->m_rpmPin >= 0)
	{
		this->setupRpmPin(this->m_rpmPin);
	}
	if (this->m_pwmPin >= 0)
	{
		this->setupPwmPin(fan.m_pwmPin);
		this->setPwmSignal(speed);
	}
}

Fan::~Fan() noexcept
{
	reset();
}

Fan& Fan::operator=(Fan&& fan) noexcept
{
	if (this == &fan)
	{
		return *this;
	}
	FanSpecification fanSpec{ std::move(fan.m_fanSpecification) };
	int pwmPin{ fan.m_pwmPin };
	int rpmPin{ fan.m_rpmPin };
	int speed{ fan.m_speed };
	fan.~Fan();
	this->reset();
	this->m_fanSpecification = std::move(fanSpec);
	this->m_pwmPin = pwmPin;
	this->m_rpmPin = rpmPin;
	this->m_speed = -1;
	if (this->m_rpmPin >= 0)
	{
		this->setupRpmPin(rpmPin);
	}
	if (this->m_pwmPin >= 0)
	{
		this->setupPwmPin(fan.m_pwmPin);
		this->setPwmSignal(speed);
	}
	return *this;
}

int64_t Fan::getRpm() const
{
	if (m_rpmPin < 0)
	{
		throw std::runtime_error("RPM measurement pin not set!");
	}
	size_t numberOfMeasurements{ 20 };
	int64_t totalRpm{ 0 };
	for (size_t counter = 0;  counter < numberOfMeasurements; counter++)
	{
		int64_t rpm{ rpmMeasurement() };
		std::cout << "RPM now: " << rpm << '\n';
		totalRpm += rpm;
	}
	return (totalRpm / numberOfMeasurements);
}

void Fan::setSpeed(const percent_t speed)
{
	if (m_pwmPin < 0)
	{
		throw std::runtime_error("PWM pin not set!");
	}
	else if (m_speed == speed)
	{
		return;
	}
	else if (speed > 100)
	{
		throw std::runtime_error("The value cannot be more than 100%!");
	}
	else
	{
		setPwmSignal(speed);
	}
}

/*
* *************** *
* PRIVATE METHODS *
* *************** *
*/

void Fan::setPwmSignal(int requiredSpeed, std::chrono::seconds stepTime) noexcept
{
	static const int maxDivisorPwmClock{ 400 }; //Соостветсвует частоте ШИМа 24 кГц при 50% заполнении.

	if (stepTime < std::chrono::seconds(12))
	{
		stepTime = std::chrono::seconds(12);
	}
	int currentSpeed{ m_speed };	
	bool isPullup{ true };
	int stepDivisorPwmClock{ maxDivisorPwmClock * 1 / 50 };
	int currentDivisorPwmClock{ currentSpeed * stepDivisorPwmClock };
	int stepSpeed{ 1 };
	if (requiredSpeed < currentSpeed)
	{
		isPullup = false;
		stepDivisorPwmClock = -stepDivisorPwmClock;
		stepSpeed = -stepSpeed;		
	}
	if (currentSpeed > 50)
	{
		currentDivisorPwmClock -= (currentSpeed - 50) * abs(stepDivisorPwmClock) * 2;
		stepDivisorPwmClock = -stepDivisorPwmClock;
	}
	std::chrono::milliseconds wait{ std::chrono::duration_cast<std::chrono::milliseconds>(stepTime) / 100 };
	currentSpeed += stepSpeed;
	currentDivisorPwmClock += stepDivisorPwmClock;
	while (isPullup ? (currentSpeed <= requiredSpeed) : (currentSpeed >= requiredSpeed))
	{
		pwmSetClock(currentDivisorPwmClock);
		pwmWrite(m_pwmPin, currentSpeed);
		m_speed = currentSpeed;
		if (currentSpeed == 50)
		{
			stepDivisorPwmClock = -stepDivisorPwmClock;
		}
		currentDivisorPwmClock += stepDivisorPwmClock;
		currentSpeed += stepSpeed;
		std::this_thread::sleep_for(wait);
	}
}

inline bool Fan::isInterrupt(const std::chrono::milliseconds& timeout) const
{
	int statusInterrupt{ 0 };
	if (timeout.count() < INT_MAX)
	{
		statusInterrupt = waitForInterrupt(m_rpmPin, static_cast<int>(timeout.count()));
	}
	else
	{
		statusInterrupt = waitForInterrupt(m_rpmPin, INT_MAX);
	}
	if (statusInterrupt == -1)
	{
		throw std::runtime_error("An error occurred while waiting for an interrupt! Сonsult the 'errno' global variable.");
	}
	else if (statusInterrupt == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void Fan::reset() noexcept
{
	//reset RPM pin
	if (m_rpmPin >= 0)
	{
		pullUpDnControl(m_rpmPin, PUD_OFF);
		pinMode(m_rpmPin, OUTPUT);
		digitalWrite(m_rpmPin, LOW);
	}

	//reset PWM pin
	if (m_pwmPin >= 0)
	{
		setPwmSignal(0);
		pinMode(m_pwmPin, OUTPUT);
		digitalWrite(m_pwmPin, LOW);
		pwmSetMode(PWM_MODE_BAL);
		pwmSetRange(1024);
		pwmSetClock(32);
	}

	//delete class data
	//fanSpecification;
	m_pwmPin = -1;
	m_rpmPin = -1;
	m_speed = 0;
}

int64_t Fan::rpmMeasurement() const
{
	using namespace std::chrono;

	/*nanoseconds minInterruptDuration{ milliseconds(1) };
	milliseconds maxInterruptTimeout{ minutes(1) };
	if (m_fanSpecification.getMaxRpm() > 0)
	{
		minInterruptDuration = duration_cast<nanoseconds>(minutes(1)) / static_cast<int64_t>(round(m_fanSpecification.getMaxRpm() * 1.25)) / 4;
		maxInterruptTimeout = duration_cast<milliseconds>(minutes(1)) / static_cast<int64_t>(round(m_fanSpecification.getMaxRpm() * 0.01 * 0.75)) / 2;
	}
	if (m_fanSpecification.getMinRpm() > 0 )
	{
		maxInterruptTimeout = duration_cast<milliseconds>(minutes(1)) / static_cast<int64_t>(round(m_fanSpecification.getMinRpm() * 0.75)) / 2;
	}*/
	microseconds interruptDuration{ 0 };
	bool isSuccessfulInterruption{ false };
	isInterrupt(hours(1)); //clear all interrupts
	isInterrupt(hours(1)); //clear all interrupts
	while (interruptDuration < microseconds(375))
	{
		auto start{ steady_clock::now() };
		isSuccessfulInterruption = isInterrupt(minutes(1));
		auto end{ steady_clock::now() };
		interruptDuration = duration_cast<microseconds>(end - start);
		//std::cout << "\ntime: " << interruptDuration.count() << '\n';
	}
	if (!isSuccessfulInterruption)
	{
		return 0;
	}
	return (minutes(1) / (interruptDuration * 4));
}

void Fan::setupPwmPin(const int pin) noexcept
{
	pinMode(pin, PWM_OUTPUT);
	//Max frequency "MS" mode -- 18,7 kHz. Max frequency "BAL" mode -- 9,6 MHz. Based on tests performed.
	pwmSetMode(PWM_MODE_BAL);
	//Set the maximum number of PWM adjustment steps
	pwmSetRange(100);
	setPwmSignal(0);
}

void Fan::setupRpmPin(const int pin) const noexcept
{
	pullUpDnControl(pin, PUD_UP);
	wiringPiISR(pin, INT_EDGE_BOTH, [] {pthread_exit(EXIT_SUCCESS); });
}

/*
* ************** *
* OTHER FUNCTION *
* ************** *
*/

