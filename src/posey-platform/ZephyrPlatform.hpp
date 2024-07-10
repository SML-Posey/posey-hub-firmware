#pragma once

#define PLATFORM_POSEY

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "platform/BaseRateLimiter.hpp"
#include "platform/BaseRateTask.hpp"

#include "posey-platform/platform/ZephyrClock.hpp"

using Clock = ZephyrClock;

typedef BaseRateLimiter<Clock, unsigned long, float> RateLimiter;

typedef BaseRateTask<RateLimiter> RateTask;

#include "posey-platform/platform/io/NordicNUSReader.hpp"
#include "posey-platform/platform/io/NordicNUSWriter.hpp"

extern NordicNUSReader reader;
extern NordicNUSWriter writer;

bool init_platform();
