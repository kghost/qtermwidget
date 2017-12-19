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

#include "protocol.h"

using namespace Konsole;

namespace Konsole {
	__declspec(dllexport) short TargetPtyTcpPort = 0;
}

TargetPtyTcp::~TargetPtyTcp() {
	disconnect();
}

int TargetPtyTcp::start(Emulation* _emulation,
	const QStringList& environment,
	ulong winid) {

	connect(&s, &QAbstractSocket::stateChanged, this, &TargetPtyTcp::stateChanged);
	connect(&s, &QIODevice::readyRead, this, &TargetPtyTcp::readReady);

	started = true;
	s.connectToHost("127.0.0.1", TargetPtyTcpPort);

	return 0;
}

void TargetPtyTcp::setWriteable(bool writeable) {}
void TargetPtyTcp::setFlowControlEnabled(bool on) {}
bool TargetPtyTcp::flowControlEnabled() const {
	return false;
}
void TargetPtyTcp::setWindowSize(int lines, int cols) {
	size.setHeight(lines);
	size.setWidth(cols);
	Pty::WinSize win;
	win.cols = cols;
	win.rows = lines;
	auto size = win.space();

	Pty::Message msg;
	msg.type = Pty::Message::TYPE_WINCH;
	msg.xid = 0;
	msg.flags = 0;
	msg.length = size;

	QByteArray qb(size, 0);
	win.marshal(qb.begin(), qb.end());
	s.write((char*)&msg, sizeof(msg));
	s.write(qb);
}

QSize TargetPtyTcp::windowSize() const {
	return size;
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
	if (length <= 0) return;
	Pty::Message msg;
	msg.type = Pty::Message::TYPE_DATA;
	msg.xid = 0;
	msg.flags = 0;
	if (length >= (1 << 16)) msg.length = -1;
	else msg.length = length;
	s.write((char*)&msg, sizeof(msg));
	s.write(buffer, msg.length);
	sendData(buffer + msg.length, length - msg.length);
}

void TargetPtyTcp::stateChanged(QAbstractSocket::SocketState socketState) {
	switch (socketState) {
	case QAbstractSocket::UnconnectedState:
		if (started && connected) {
			connected = false;
			auto s = "\r\nbridge disconnected.";
			emit receivedData(s, strlen(s));
		}
		break;
	case QAbstractSocket::ConnectedState: {
		connected = true;

		Pty::Exec exec;
		exec.envs.push_back({ "TERM", "xterm" });
		auto size = exec.space();

		Pty::Message msg;
		msg.type = Pty::Message::TYPE_EXEC;
		msg.xid = 0;
		msg.flags = 0;
		msg.length = size;
		QByteArray qb(size, 0);
		exec.marshal(qb.begin(), qb.end());
		// XXX: does write block ?
		s.write((char*)&msg, sizeof(msg));
		s.write(qb);
		break;
	}
	case QAbstractSocket::ClosingState:
		s.close();
		break;
	}
}

void TargetPtyTcp::readReady() {
	while (s.bytesAvailable() > 0)
	{
		Pty::Message msg;
		s.read((char*)&msg, sizeof(msg));
		QByteArray qb(msg.length, 0);
		s.read(qb.begin(), qb.length());
		switch (msg.type)
		{
		case Pty::Message::TYPE_DATA: emit receivedData(qb.constData(), qb.length()); break;
		case Pty::Message::TYPE_EXIT:
			Pty::Exit exit;
			exit.unmarshal(qb.begin(), qb.end());
			if (exit.exited) emit finished(exit.exit_status);
			break;
		default:
			break;
		}
	}
}
