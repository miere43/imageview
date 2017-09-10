#pragma once

#define E_VERIFY(cond) \
	do { \
		if ((!cond)) { \
			__debugbreak(); \
			return; \
		} \
	} while (0)
