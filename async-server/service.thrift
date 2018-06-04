/*
 * This file is part of the Thrift-ZeroMQ examples
 *    (https://github.com/stefbo/thrift-zmq-examples).
 * Copyright (c) 2018, Stefan Bolus.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

namespace cpp example

enum MathOp
{
	MATHOP_ADD,
	MATHOP_DIV
}

// Error codes:
enum ErrorCode {
	SERVICE_CMD_NO_ERROR = 0,
	SERVICE_CMD_UNKNOWN_COMMAND = 1,
	SERVICE_CMD_INVALID_ARGUMENTS = 2,
	SERVICE_CMD_CANCELLED = 3,
	// ...
}

exception Error {
	1: required ErrorCode errorCode
	2: optional string errorMessage
}

service Service {
	// Executed the given Math expression and returns the result.
	double math(1: required MathOp op, 2: required double arg1, 3: required double arg2) throws (1: Error ex)

	// Sleeps (at least) the given amount of time.
	i32 sleep(1: required i32 timeMsecs) throws (1: Error ex)
}