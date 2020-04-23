/**
 * TimerH (https://github.com/UnexomWid/timerh)
 *
 * This project is licensed under the MIT license.
 * Copyright (c) 2018-2020 UnexomWid (https://uw.exom.dev)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "timer.hxx"

CHRONOMETER time_now() {
	return std::chrono::high_resolution_clock::now();
}

uint64_t get_exec_time_h(CHRONOMETER start) {
	return std::chrono::duration_cast<std::chrono::hours>(time_now() - start).count();
}

uint64_t get_exec_time_m(CHRONOMETER start) {
	return std::chrono::duration_cast<std::chrono::minutes>(time_now() - start).count();
}

uint64_t get_exec_time_s(CHRONOMETER start) {
	return std::chrono::duration_cast<std::chrono::seconds>(time_now() - start).count();
}

uint64_t get_exec_time_ms(CHRONOMETER start) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(time_now() - start).count();
}

uint64_t get_exec_time_mis(CHRONOMETER start) {
	return std::chrono::duration_cast<std::chrono::microseconds>(time_now() - start).count();
}

uint64_t get_exec_time_ns(CHRONOMETER start) {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(time_now() - start).count();
}

std::string format_time_ms(uint64_t milliseconds) {
	std::string sTime;

	uint64_t tmp = milliseconds / 86400000; //Days.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " day";
		if(tmp > 1)
			sTime += 's';
		milliseconds %= 86400000;
	}

	tmp = milliseconds / 3600000; // Hours.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " hour";
		if (tmp > 1)
			sTime += 's';
		milliseconds %= 3600000;
	}

	tmp = milliseconds / 60000; // Minutes.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " minute";
		if (tmp > 1)
			sTime += 's';
		milliseconds %= 60000;
	}

	tmp = milliseconds / 1000; // Seconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " second";
		if (tmp > 1)
			sTime += 's';
		milliseconds %= 1000;
	}

	tmp = milliseconds; // Milliseconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " millisecond";
		if (tmp > 1)
			sTime += 's';
	}
	else if (sTime == "") {
		sTime += "0 milliseconds";
	}

	return sTime;
}

std::string format_time_mis(uint64_t microseconds) {
	std::string sTime;

	uint64_t tmp = microseconds / 86400000000; //Days.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " day";
		if (tmp > 1)
			sTime += 's';
		microseconds %= 86400000000;
	}

	tmp = microseconds / 3600000000; // Hours.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " hour";
		if (tmp > 1)
			sTime += 's';
		microseconds %= 3600000000;
	}

	tmp = microseconds / 60000000; // Minutes.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " minute";
		if (tmp > 1)
			sTime += 's';
		microseconds %= 60000000;
	}

	tmp = microseconds / 1000000; // Seconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " second";
		if (tmp > 1)
			sTime += 's';
		microseconds %= 1000000;
	}

	tmp = microseconds / 1000; // Milliseconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " millisecond";
		if (tmp > 1)
			sTime += 's';
		microseconds %= 1000;
	}

	tmp = microseconds; // Microseconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " microsecond";
		if (tmp > 1)
			sTime += 's';
	}
	else if (sTime == "") {
		sTime += "0 microseconds";
	}

	return sTime;
}

std::string format_time_ns(uint64_t nanoseconds) {
	std::string sTime;

	uint64_t tmp = nanoseconds / 86400000000000; //Days.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " day";
		if (tmp > 1)
			sTime += 's';
		nanoseconds %= 86400000000000;
	}

	tmp = nanoseconds / 3600000000000; // Hours.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " hour";
		if (tmp > 1)
			sTime += 's';
		nanoseconds %= 3600000000000;
	}

	tmp = nanoseconds / 60000000000; // Minutes.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " minute";
		if (tmp > 1)
			sTime += 's';
		nanoseconds %= 60000000000;
	}

	tmp = nanoseconds / 1000000000; // Seconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " second";
		if (tmp > 1)
			sTime += 's';
		nanoseconds %= 1000000000;
	}

	tmp = nanoseconds / 1000000; // Milliseconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " millisecond";
		if (tmp > 1)
			sTime += 's';
		nanoseconds %= 1000000;
	}

	tmp = nanoseconds / 1000; // Microseconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " microsecond";
		if (tmp > 1)
			sTime += 's';
		nanoseconds %= 1000;
	}
	
	tmp = nanoseconds; // Nanoseconds.
	if (tmp != 0) {
		if (sTime != "")
			sTime += ", ";
		sTime += std::to_string(tmp);
		sTime += " nanosecond";
		if (tmp > 1)
			sTime += 's';
	}
	else if (sTime == "") {
		sTime += "0 nanoseconds";
	}

	return sTime;
}

std::string getf_exec_time_ms(CHRONOMETER start) {
	return format_time_ms(get_exec_time_ms(start));
}

std::string getf_exec_time_mis(CHRONOMETER start) {
	return format_time_mis(get_exec_time_mis(start));
}

std::string getf_exec_time_ns(CHRONOMETER start) {
	return format_time_ns(get_exec_time_ns(start));
}