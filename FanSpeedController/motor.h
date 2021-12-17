#pragma once

#ifndef MOTOR_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	namespace motor
	{
		enum class WiringPiMode
		{
			UNINITIALISED,
			DEFAULT,
			SYS,
			GPIO,
			PHYS,
		};
	}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !MOTOR_H
