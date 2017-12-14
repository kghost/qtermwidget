/*
This file is part of Konsole, an X terminal.

Copyright (C) 2007 by Robert Knight <robertknight@gmail.com>
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

#ifndef TARGET_PTYTCP_H
#define TARGET_PTYTCP_H

#include <QStringList>
#include <QTcpSocket>

#include "Target.h"

namespace Konsole {
class Emulation;
class TargetPtyTcp : public Target {
	Q_OBJECT

public:
	/**
	* Constructor
	*/
	TargetPtyTcp() {}
	virtual ~TargetPtyTcp() {}

	virtual int start(Emulation* _emulation,
		const QStringList& environment,
		ulong winid);

	/** TODO: Document me */
	virtual void setWriteable(bool writeable);

	/**
	* Enables or disables Xon/Xoff flow control.  The flow control setting
	* may be changed later by a terminal application, so flowControlEnabled()
	* may not equal the value of @p on in the previous call to setFlowControlEnabled()
	*/
	virtual void setFlowControlEnabled(bool on);

	/** Queries the terminal state and returns true if Xon/Xoff flow control is enabled. */
	virtual bool flowControlEnabled() const;

	/**
	* Sets the size of the window (in lines and columns of characters)
	* used by this teletype.
	*/
	virtual void setWindowSize(int lines, int cols);

	/** Returns the size of the window used by this teletype.  See setWindowSize() */
	virtual QSize windowSize() const;

	/** TODO Document me */
	virtual void setErase(char erase);

	/** */
	virtual char erase() const;

	virtual bool isRunning() const;

  public slots:

	/**
	* Put the pty into UTF-8 mode on systems which support it.
	*/
    virtual void setUtf8Mode(bool on);

	/**
	* Sends data to the process currently controlling the
	* teletype ( whose id is returned by foregroundProcessGroup() )
	*
	* @param buffer Pointer to the data to send.
	* @param length Length of @p buffer.
	*/
    virtual void sendData(const char* buffer, int length);

  signals:

	/**
	* Emitted when a new block of data is received from
	* the teletype.
	*
	* @param buffer Pointer to the data received.
	* @param length Length of @p buffer
	*/
	void receivedData(const char* buffer, int length);

  private:
	QTcpSocket s;
	bool connected = false;
	bool started = false;
  private slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
	void readReady();
};
}

#endif
