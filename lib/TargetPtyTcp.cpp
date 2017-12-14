/*
This file is part of Konsole

Copyright (C) 2006-2007 by Robert Knight <robertknight@gmail.com>
Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

Rewritten for QT4 by e_k <e_k at users.sourceforge.net>, Copyright (C)2008

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301  USA.
*/

// Own
#include "TargetPtyTcp.h"

using namespace Konsole;

int TargetPtyTcp::start(Emulation* _emulation,
	const QStringList& environment,
	ulong winid) {

	connect(&s, &QAbstractSocket::stateChanged, this, &TargetPtyTcp::stateChanged);
	connect(&s, &QIODevice::readyRead, this, &TargetPtyTcp::readReady);

	started = true;
	s.connectToHost("127.0.0.1", 19863);

	return 0;
}

void TargetPtyTcp::setWriteable(bool writeable) {}
void TargetPtyTcp::setFlowControlEnabled(bool on) {}
bool TargetPtyTcp::flowControlEnabled() const {
	return false;
}
void TargetPtyTcp::setWindowSize(int lines, int cols) {}
QSize TargetPtyTcp::windowSize() const {
	return QSize();
}
void TargetPtyTcp::setErase(char erase) {}
char TargetPtyTcp::erase() const {
	return 0;
}
bool TargetPtyTcp::isRunning() const {
	return connected;
}
void TargetPtyTcp::setUtf8Mode(bool on) {}

void TargetPtyTcp::sendData(const char* buffer, int length) {
	s.write(buffer, length);
}

void TargetPtyTcp::stateChanged(QAbstractSocket::SocketState socketState) {
	switch (socketState) {
	case QAbstractSocket::UnconnectedState:
		if (started) {
			connected = false;
			emit finished();
		}
		break;
	case QAbstractSocket::ConnectedState:
		connected = true;
		break;
	case QAbstractSocket::ClosingState:
		s.close();
		break;
	}
}

void TargetPtyTcp::readReady() {
	auto data = s.readAll();
	emit receivedData(data.constData(), data.length());
}
