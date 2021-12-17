#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <wiringPi.h>

#include "motor.h"

namespace motor
{
	/*
	* **************************** *
	* ****** TYPES SECTION ******* *
	* **************************** *
	*/

	enum class UseCase
	{
		PWM,
		RPM,
	};

	struct PinInfo
	{
		int gpioPin;
		UseCase useCase;
		int value;

	};

	/*
	* **************************** *
	* * GLOBAL VARIABLES SECTION * *
	* **************************** *
	*/

	static std::vector<PinInfo> g_usedPins     {}; //In BCM(GPIO)
	static WiringPiMode         g_wiringPiMode { WiringPiMode::UNINITIALISED };

	/*
	* **************************** *
	* FUNCTION DECLARATION SECTION *
	* **************************** *
	*/

	static void   checkPin(const int gpioPin);
	static size_t findIndexPin(const int desiredGpioPin);
	static bool   isUsedPin(const int desiredGpioPin);
	static void   setupPwmPin(const int originPin);
	static int    originPinToGpio(const int originPin);

	/*
	* **************************** *
	* * LIBRARY FUNCTIONS SECTION  *
	* **************************** *
	*/

	void setSpeed(const int pin, const int percentOfPower)
	{
		if (percentOfPower < 0 || percentOfPower > 100)
		{
			throw std::invalid_argument(std::string("It is not possible to set such a percentage of the motor speed: ") + std::to_string(percentOfPower) + 
				std::string("%. Variable \"percentOfPower\" can take values from 0 to 100 inclusive."));
		}
		const int gpioPin{ originPinToGpio(pin) };
		if (!isUsedPin(gpioPin))
		{
			checkPin(gpioPin);
			setupPwmPin(pin);
			g_usedPins.push_back(PinInfo{ gpioPin, UseCase::PWM, 0 });
		}
		size_t indexPin{};
		try
		{
			indexPin = findIndexPin(gpioPin);
		}
		catch (const std::runtime_error& exception)
		{
			throw std::runtime_error(std::string("Fatal error, unable to continue. Error message: \"") + exception.what() + std::string("\"."));
		}

	}

	void setWiringPiSetupMode(const WiringPiMode wiringPiMode)
	{
		g_wiringPiMode = wiringPiMode;
	}

	/*
	* **************************** *
	* FUNCTION DEFINITION SECTION  *
	* **************************** *
	*/

	static void checkPin(const int gpioPin)
	{
		const std::pair<int, int> pwmChannel0{ std::make_pair(12, 18) };//value in BCM(GPIO)
		const std::pair<int, int> pwmChannel1{ std::make_pair(13, 19) };//value in BCM(GPIO)

		auto hasPairValue{ [](const std::pair<int, int>& pair, const int value) {return (std::get<0>(pair) == value || std::get<1>(pair) == value); } };
		if (hasPairValue(pwmChannel0, gpioPin))
		{
			if (isUsedPin(std::get<0>(pwmChannel0)))
			{
				throw std::invalid_argument("GPIO pin 18 cannot be used, PWM channel 0 is occupied by pin 12. Use channel 1: GPIO pin 13, GPIO pin 19");
			}
			else if (isUsedPin(std::get<1>(pwmChannel0)))
			{
				throw std::invalid_argument("GPIO pin 12 cannot be used, PWM channel 0 is occupied by pin 18. Use channel 1: GPIO pin 13, GPIO pin 19");
			}
		}
		else if (hasPairValue(pwmChannel1, gpioPin))
		{
			if (isUsedPin(std::get<0>(pwmChannel0)))
			{
				throw std::invalid_argument("GPIO pin 19 cannot be used, PWM channel 1 is occupied by pin 13. Use channel 0: GPIO pin 12, GPIO pin 18");
			}
			else if (isUsedPin(std::get<1>(pwmChannel0)))
			{
				throw std::invalid_argument("GPIO pin 13 cannot be used, PWM channel 1 is occupied by pin 19. Use channel 0: GPIO pin 12, GPIO pin 18");
			}
		}
		else
		{
			throw std::invalid_argument("Not a hardware capable PWM pin! Compatible PWM pins: channel0(GPIO/BCM pin 12, GPIO/BCM pin 18), channel1(GPIO/BCM pin 13, GPIO/BCM pin 19).");
		}
	}

	static size_t findIndexPin(const int desiredGpioPin)
	{
		auto iteratorUsedPin{ std::find_if(g_usedPins.begin(), g_usedPins.end(), [=](const PinInfo usedPin) {return (usedPin.gpioPin == desiredGpioPin); }) };
		if (iteratorUsedPin == g_usedPins.end())
		{
			throw std::runtime_error("findIndexPin(): pin not found!");
		}
		else
		{
			auto indexPin{ std::distance(g_usedPins.begin(), iteratorUsedPin) };
			if (indexPin < 0)
			{
				indexPin = -indexPin;
			}
			return (static_cast<size_t>(indexPin));
		}
	}

	static bool isUsedPin(const int desiredGpioPin)
	{
		if (g_usedPins.empty())
		{
			return false;
		}
		for (auto usedPin : g_usedPins)
		{
			if (usedPin.gpioPin == desiredGpioPin)
			{
				return true;
			}
		}
	}

	static void setupPwmPin(const int originPin)
	{
		pinMode(originPin, PWM_OUTPUT);

		//Max frequency "MS" mode -- 18,7 kHz. Max frequency "BAL" mode -- 9,6 MHz. Based on tests performed on Raspberry Pi 2.
		//By default, the wiringPi library uses the "BAL" mode.
		pwmSetMode(PWM_MODE_BAL);

		//Set the maximum number of PWM adjustment steps.
		//By default, the wiringPi library uses 1024 adjustment step.
		pwmSetRange(100);

		//Corresponds to a 24 kHz PWM frequency at 50% duty cycle.
		//pwmSetClock(400);

		//Set zero in pin.
		pwmWrite(originPin, 0);
	}

	static int originPinToGpio(const int originPin)
	{
		switch (g_wiringPiMode)
		{
		case WiringPiMode::UNINITIALISED:
			throw std::runtime_error("The way to initialize the wiringPi library is not specified, or it is not initialized!");
		case WiringPiMode::DEFAULT:
			return wpiPinToGpio(originPin);
		case WiringPiMode::GPIO:
			return originPin;
		case WiringPiMode::PHYS:
			return physPinToGpio(originPin);
		case WiringPiMode::SYS:
		default:
			throw std::runtime_error(std::string("With this initialization method, it is not possible to continue.") +
				std::string(" Possible initialization methods: \"DEFAULT\" - wiringPiSetup(), \"GPIO\" - wiringPiSetupGpio(), \"PHYS\" - wiringPiSetupPhys()."));
			break;
		}
	}
}