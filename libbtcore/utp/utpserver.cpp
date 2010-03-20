/***************************************************************************
 *   Copyright (C) 2009 by Joris Guisson                                   *
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

#include "utpserver.h"
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <util/log.h>
#include <util/constants.h>
#include <mse/streamsocket.h>
#include <torrent/globals.h>
#include <net/portlist.h>
#include "utpprotocol.h"
#include "utpserverthread.h"
#include "utpsocket.h"

#ifdef Q_WS_WIN
#include <util/win32.h>
#endif



using namespace bt;

namespace utp
{

	UTPServer::UTPServer(QObject* parent) 
		: ServerInterface(parent),
		sock(0),
		running(false),
		utp_thread(0),
		mutex(QMutex::Recursive),
		create_sockets(true),
		tos(0)
	{
		qsrand(time(0));
		connect(this,SIGNAL(accepted(Connection*)),this,SLOT(onAccepted(Connection*)),Qt::QueuedConnection);
		poll_pipes.setAutoDelete(true);
	}

	UTPServer::~UTPServer()
	{
		if (running)
			stop();
	}
	
	
	bool UTPServer::changePort(bt::Uint16 p)
	{
		if (sock && port == p)
			return true;
		
		QStringList possible = bindAddresses();
		foreach (const QString & ip,possible)
		{
			net::Address addr(ip,p);
			if (bind(addr))
				return true;
		}
		
		return false;
	}


	bool UTPServer::bind(const net::Address& addr)
	{
		if (sock)
		{
			Globals::instance().getPortList().removePort(port,net::UDP);
			delete sock;
		}
		
		sock = new net::Socket(false,addr.ipVersion());
		sock->setBlocking(false);
		if (!sock->bind(addr,false))
		{
			delete sock;
			sock = 0;
			return false;
		}
		else
		{
			Out(SYS_CON|LOG_NOTICE) << "UTP: bound to " << addr.toString() << endl;
			sock->setTOS(tos);
			Globals::instance().getPortList().addNewPort(addr.port(),net::UDP,true);
			return true;
		}
	}
	
	void UTPServer::setTOS(Uint8 type_of_service)
	{
		tos = type_of_service;
		if (sock)
			sock->setTOS(tos);
	}

	void UTPServer::run()
	{
		running = true;
		fd_set rfds;
		fd_set wfds;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		while (running)
		{
			int fd = sock->fd();
			FD_SET(fd,&rfds);
			if (output_queue.size() > 0)
				FD_SET(fd,&wfds);
			
			struct timeval tv = {0,10000};
			if (select(fd + 1,&rfds,&wfds,0,&tv) > 0)
			{
				if (FD_ISSET(fd,&rfds))
					readPacket();
				if (FD_ISSET(fd,&wfds))
					writePacket();
			}
			
			try
			{
				checkTimeouts();
				clearDeadConnections();
			}
			catch (utp::Connection::TransmissionError & err)
			{
				Out(SYS_CON|LOG_NOTICE) << "UTP: " << err.location << endl;
			}
		}
	}
	
	void UTPServer::readPacket()
	{
		QMutexLocker lock(&mutex);
		
		int ba = sock->bytesAvailable();
		QByteArray packet(ba,0);
		net::Address addr;
		if (sock->recvFrom((bt::Uint8*)packet.data(),ba,addr) > 0)
		{
		//	Out(SYS_CON|LOG_NOTICE) << "UTP: received " << ba << " bytes packet from " << addr.toString() << endl;
			// discard packets which are to small
			if (ba < (int)sizeof(utp::Header))
				return;
			
			try
			{
				handlePacket(packet,addr);
			}
			catch (utp::Connection::TransmissionError & err)
			{
				Out(SYS_CON|LOG_NOTICE) << "UTP: " << err.location << endl;
			}
		}
	}
	
	void UTPServer::writePacket()
	{
		// Keep sending until the output queue is empty or the socket 
		// can't handle the data anymore
		while (!output_queue.empty())
		{
			QPair<QByteArray,net::Address> & packet = output_queue.front();
			if (sock->sendTo((const bt::Uint8*)packet.first.data(),packet.first.size(),packet.second) == packet.first.size())
				output_queue.pop_front();
			else
				break;
		}
	}
	
	
	static void Dump(const QByteArray & data, const net::Address& addr)
	{
		Out(SYS_CON|LOG_DEBUG) << QString("Received packet from %1 (%2 bytes)").arg(addr.toString()).arg(data.size()) << endl;
		const bt::Uint8* pkt = (const bt::Uint8*)data.data();
		
		QString line;
		for (int i = 0;i < data.size();i++)
		{
			if (i > 0 && i % 32 == 0)
			{
				Out(SYS_CON|LOG_DEBUG) << line << endl;
				line = "";
			}
			
			uint val = pkt[i];
			line += QString("%1").arg(val,2,16,QChar('0'));
			if (i + 1 % 4)
				line += ' ';
		}
		Out(SYS_CON|LOG_DEBUG) << line << endl;
	}
	
	static void DumpPacket(const Header & hdr)
	{
		Out(SYS_CON|LOG_NOTICE) << "==============================================" << endl;
		Out(SYS_CON|LOG_NOTICE) << "UTP: Packet Header: " << endl;
		Out(SYS_CON|LOG_NOTICE) << "type:                              " << TypeToString(hdr.type) << endl;
		Out(SYS_CON|LOG_NOTICE) << "version:                           " << hdr.version << endl;
		Out(SYS_CON|LOG_NOTICE) << "extension:                         " << hdr.extension << endl;
		Out(SYS_CON|LOG_NOTICE) << "connection_id:                     " << hdr.connection_id << endl;
		Out(SYS_CON|LOG_NOTICE) << "timestamp_microseconds:            " << hdr.timestamp_microseconds << endl;
		Out(SYS_CON|LOG_NOTICE) << "timestamp_difference_microseconds: " << hdr.timestamp_difference_microseconds << endl;
		Out(SYS_CON|LOG_NOTICE) << "wnd_size:                          " << hdr.wnd_size << endl;
		Out(SYS_CON|LOG_NOTICE) << "seq_nr:                            " << hdr.seq_nr << endl;
		Out(SYS_CON|LOG_NOTICE) << "ack_nr:                            " << hdr.ack_nr << endl;
		Out(SYS_CON|LOG_NOTICE) << "==============================================" << endl;
	}
	
	void UTPServer::handlePacket(const QByteArray& packet, const net::Address& addr)
	{
		PacketParser parser(packet);
		if (!parser.parse())
			return;
		
		const Header* hdr = parser.header();
		//Dump(packet,addr);
		//DumpPacket(*hdr);
		
		switch (hdr->type)
		{
			case ST_DATA:
			case ST_FIN:
			case ST_STATE:
				try
				{
					Connection* c = find(hdr->connection_id);
					if (c)
						c->handlePacket(parser,packet);
					else
						Out(SYS_CON|LOG_NOTICE) << "UTP: unkown connection " << hdr->connection_id << endl;
				}
				catch (Connection::TransmissionError & err)
				{
					Out(SYS_CON|LOG_NOTICE) << "UTP: " << err.location << endl;
					// TODO: kill connection
				}
				break;
			case ST_RESET:
				reset(hdr);
				break;
			case ST_SYN:
				syn(parser,packet,addr);
				break;
		}
	}


	bool UTPServer::sendTo(const QByteArray& data, const net::Address& addr)
	{
		// if output_queue is not empty append to it, so that packet order is OK
		// (when they are being sent anyway)
		if (output_queue.empty())
		{
			// If we can't send kernel buffers are probably full,
			// so wait until socket becomes writeable
			if (sock->sendTo((const bt::Uint8*)data.data(),data.size(),addr) != data.size())
				output_queue.append(qMakePair(data,addr));
		}
		else
			output_queue.append(qMakePair(data,addr));
		
		return true;
	}

	Connection* UTPServer::connectTo(const net::Address& addr)
	{
		QMutexLocker lock(&mutex);
		quint16 recv_conn_id = qrand() % 32535;
		while (connections.contains(recv_conn_id))
			recv_conn_id = qrand() % 32535;
		
		Connection* conn = new Connection(recv_conn_id,Connection::OUTGOING,addr,this);
		connections.insert(recv_conn_id,conn);
		
		Out(SYS_CON|LOG_NOTICE) << "UTP: connecting to " << addr.toString() << endl;
		conn->startConnecting();
		return conn;
	}

	void UTPServer::syn(const PacketParser & parser, const QByteArray& data, const net::Address & addr)
	{
		const Header* hdr = parser.header();
		quint16 recv_conn_id = hdr->connection_id + 1;
		if (connections.find(recv_conn_id))
		{
			// Send a reset packet if the ID is in use
			Connection conn(recv_conn_id,Connection::INCOMING,addr,this);
			conn.sendReset();
		}
		else
		{
			Connection* conn = new Connection(recv_conn_id,Connection::INCOMING,addr,this);
			try
			{
				conn->handlePacket(parser,data);
				connections.insert(recv_conn_id,conn);
				accepted(conn);
			}
			catch (Connection::TransmissionError & err)
			{
				Out(SYS_CON|LOG_NOTICE) << "UTP: " << err.location << endl;
				delete conn;
			}
		}
	}

	void UTPServer::reset(const utp::Header* hdr)
	{
		Connection* c = find(hdr->connection_id);
		if (c)
		{
			c->reset();
		}
	}

	Connection* UTPServer::find(quint16 conn_id)
	{
		return connections.find(conn_id);
	}

	void UTPServer::clearDeadConnections()
	{
		QMutexLocker lock(&mutex);
		QList<Connection*>::iterator i = dead_connections.begin();
		while (i != dead_connections.end())
		{
			Connection* conn = *i;
			if (conn->connectionState() == CS_CLOSED)
			{
				connections.erase(conn->receiveConnectionID());
				i = dead_connections.erase(i);
			}
			else
				i++;
		}
	}

	void UTPServer::attach(UTPSocket* socket, Connection* conn)
	{
		QMutexLocker lock(&mutex);
		alive_connections.insert(conn,socket);
	}

	void UTPServer::detach(UTPSocket* socket, Connection* conn)
	{
		QMutexLocker lock(&mutex);
		UTPSocket* sock = alive_connections.find(conn);
		if (sock == socket)
		{
			// given the fact that the socket is gone, we can close it
			conn->close();
			alive_connections.erase(conn);
			dead_connections.append(conn);
		}
	}

	void UTPServer::stop()
	{
		running = false;
		if (utp_thread)
		{
			utp_thread->wait();
			delete utp_thread;
			utp_thread = 0;
		}
	
		// Cleanup all connections
		QList<UTPSocket*> sockets;
		bt::PtrMap<Connection*,UTPSocket>::iterator i = alive_connections.begin();
		while (i != alive_connections.end())
		{
			sockets.append(i->second);
			i++;
		}
		
		foreach (UTPSocket* s,sockets)
			s->reset();
		
		alive_connections.clear();
		connections.clear();
		qDeleteAll(dead_connections);
		dead_connections.clear();
		
		// Close the socket
		if (sock)
		{
			sock->close();
			delete sock;
			sock = 0;
			Globals::instance().getPortList().removePort(port,net::UDP);
		}
	}
	
	
	void UTPServer::start()
	{
		if (!utp_thread)
		{
			utp_thread = new UTPServerThread(this);
			utp_thread->start();
		}
	}

	void UTPServer::checkTimeouts()
	{
		QMutexLocker lock(&mutex);
	
		ConItr itr = connections.begin();
		while (itr != connections.end())
		{
			itr->second->checkTimeout();
			
			for (PollPipePairItr p = poll_pipes.begin();p != poll_pipes.end();p++)
				p->second->test(itr->second);
			itr++;
		}
	}


	void UTPServer::preparePolling(net::Poll* p, net::Poll::Mode mode,Connection* conn)
	{
		QMutexLocker lock(&mutex);
		PollPipePair* pair = poll_pipes.find(p);
		if (!pair)
		{
			pair = new PollPipePair();
			poll_pipes.insert(p,pair);
		}
		
		if (mode == net::Poll::INPUT)
		{
			pair->read_pipe.prepare(p,conn->receiveConnectionID());
		}
		else
		{
			pair->write_pipe.prepare(p,conn->receiveConnectionID());
		}
	}
	
	void UTPServer::onAccepted(Connection* conn)
	{
		if (create_sockets)
			newConnection(new mse::StreamSocket(new UTPSocket(conn)));
	}
	
	UTPServer::PollPipePair::PollPipePair() : read_pipe(net::Poll::INPUT),write_pipe(net::Poll::OUTPUT)
	{
		
	}
	
	
	void UTPServer::PollPipePair::test(Connection* conn)
	{
		if (read_pipe.readyToWakeUp(conn))
			read_pipe.wakeUp();
		
		if (write_pipe.readyToWakeUp(conn))
			write_pipe.wakeUp();
	}


}