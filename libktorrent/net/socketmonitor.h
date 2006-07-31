/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#ifndef NETSOCKETMONITOR_H
#define NETSOCKETMONITOR_H

#include <qstring.h>
#include <qmutex.h>
#include <qptrlist.h>
#include <util/constants.h>


namespace net
{
	using bt::Uint32;
	
	class BufferedSocket;
	class MonitorThread;

	/**
		@author Joris Guisson <joris.guisson@gmail.com>
	*/
	class SocketMonitor 
	{
		static SocketMonitor self;
		static Uint32 dcap,ucap;
		QMutex mutex;
		MonitorThread* mt;
		QPtrList<BufferedSocket> smap;
		Uint32 last_selected;
		Uint32 speeds_last_updated;
		Uint32 leftover_d,leftover_u;
		
		SocketMonitor();	
	public:
		virtual ~SocketMonitor();
		
		void add(BufferedSocket* sock);
		void remove(BufferedSocket* sock);
		void update();
		
		static void setDownloadCap(Uint32 bytes_per_sec);
		static void setUploadCap(Uint32 bytes_per_sec);
		static SocketMonitor & instance() {return self;}
	private:
		void processOutgoingData(QPtrList<BufferedSocket> & wbs,Uint32 now);
		void processIncomingData(QPtrList<BufferedSocket> & rbs,Uint32 now);
	};

}

#endif
