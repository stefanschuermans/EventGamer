# EventGamer - telephone controls for Snake in MadMapper.
# Copyright (C) 2026 Stefan Schuermans
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Runs in the oscpack source directory. oscpack treats only x86_64 as LP64, so on
# other 64-bit targets (e.g. arm64 macOS) osc::int32 becomes an 8-byte long.
# Treat every LP64 target like x86_64. Safe to run repeatedly.
set(original "defined(__x86_64__) || defined(_M_X64)")
set(patched "${original} || defined(__LP64__)")
foreach(header osc/OscTypes.h osc/OscReceivedElements.h osc/OscOutboundPacketStream.h)
    file(READ "${header}" content)
    string(REPLACE "${patched}" "${original}" content "${content}")
    string(FIND "${content}" "${original}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Unexpected ${header}; cannot apply the LP64 int32 fix")
    endif()
    string(REPLACE "${original}" "${patched}" content "${content}")
    file(WRITE "${header}" "${content}")
endforeach()
