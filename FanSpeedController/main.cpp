#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <wiringPi.h>
#include "Fan.h"

void test1();
void test2(const int a);

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Not enough arguments!" << std::endl;
		return EXIT_FAILURE;
	}

	wiringPiSetup();

	if (strcmp(argv[1], "test1") == 0)
	{
		test1();
	}
	else if (strcmp(argv[1], "test2") == 0)
	{
		test2(std::stoi(argv[2]));
	}
		
	return EXIT_SUCCESS;
}

void func()
{
	pthread_exit(0);
}

void test2(const int a)
{
	Fan fan(26, 27, FanSpecification(900, 1900));
	fan.setSpeed(a);
	auto res{ fan.getRpm() };
	std::cout << "Result RPM: " << res << std::endl;

	std::cin.get();
}

void test1()
{
	wiringPiISR(27, INT_EDGE_BOTH, func);
	pullUpDnControl(27, PUD_UP);

	pinMode(26, PWM_OUTPUT);
	pwmSetRange(100);

	pwmSetClock(100);	
	pwmWrite(26, 25);

	//std::this_thread::sleep_for(std::chrono::milliseconds(31 * 150));

	waitForInterrupt(27, -1);

	for (size_t i = 1; i <= 250; i++)
	{
		waitForInterrupt(27, -1);
		waitForInterrupt(27, -1);
		auto start{std::chrono::steady_clock::now()};
		waitForInterrupt(27, -1);
		auto end{ std::chrono::steady_clock::now() };
		std::cout << i << ". " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << '\n';
	}
}