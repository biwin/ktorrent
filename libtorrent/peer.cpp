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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "log.h"
#include "peer.h"
#include "chunk.h"
#include "speedestimater.h"
#include "piece.h"
#include "request.h"
#include "packetreader.h"
#include "packetwriter.h"

namespace bt
{
	
	
	
	
	Peer::Peer(QSocket* sock,const PeerID & peer_id,Uint32 num_chunks) 
	: sock(sock),pieces(num_chunks),peer_id(peer_id)
	{
		speed = new SpeedEstimater();
		preader = new PacketReader(sock,speed);
		choked = am_choked = true;
		interested = am_interested = false;
		
		connect(sock,SIGNAL(connectionClosed()),this,SLOT(connectionClosed()));
		connect(sock,SIGNAL(readyRead()),this,SLOT(readyRead()));
		connect(sock,SIGNAL(error(int)),this,SLOT(error(int)));
		pwriter = new PacketWriter(this);
	}


	Peer::~Peer()
	{
		delete pwriter;
		delete preader;
		sock->close();
		delete sock;
		delete speed;
	}

	void Peer::connectionClosed() 
	{
		Out() << "Connection Closed" << endl;
		fatalError(this);
	} 
	
	void Peer::closeConnection()
	{
		sock->close();
	}
	
	void Peer::readyRead() 
	{
		readPacket();
	}
	
	void Peer::error(int err)
	{
		Out() << "Error : " << err << endl;
		fatalError(this);
	}
	

	
	void Peer::readPacket()
	{
		while (preader->readPacket() && preader->ok())
		{
			handlePacket(preader->getPacketLength());
		}
		
		if (!preader->ok())
			fatalError(this);
	}
	
	void Peer::handlePacket(Uint32 len)
	{
		if (len == 0)
			return;
		const Uint8* tmp_buf = preader->getData();
		//Out() << "Got packet : " << len << " type = " << type <<  endl;
		Uint8 type = tmp_buf[0];
		switch (type)
		{
			case CHOKE:
				if (len != 1)
				{
					Out() << "len err CHOKE" << endl;
				//	fatalError(this);
					return;
				}
				
				choked = true;
				break;
			case UNCHOKE:
				if (len != 1)
				{
					Out() << "len err UNCHOKE" << endl;
				//	fatalError(this);
					return;
				}
				
				choked = false;
				break;
			case INTERESTED:
				if (len != 1)
				{
					Out() << "len err INTERESTED" << endl;
				//	fatalError(this);
					return;
				}
				
				interested = true;
				break;
			case NOT_INTERESTED:
				if (len != 1)
				{
					Out() << "len err NOT_INTERESTED" << endl;
				//	fatalError(this);
					return;
				}
				
				interested = false;
				break;
			case HAVE:
				if (len != 5)
				{
					Out() << "len err HAVE" << endl;
				//	fatalError(this);
					return;
				}
				
				haveChunk(this,ReadUint32(tmp_buf,1));
				pieces.set(ReadUint32(tmp_buf,1),true);
				break;
			case BITFIELD:
				if (len != 1 + pieces.getNumBytes())
				{
					Out() << "len err BITFIELD" << endl;
				//	fatalError(this);
					return;
				}
				
				pieces = BitSet(tmp_buf+1,pieces.getNumBits());
				break;
			case REQUEST:
				if (len != 13)
				{
					Out() << "len err REQUEST" << endl;
				//	fatalError(this);
					return;
				}
				
				{
					Request r(
							ReadUint32(tmp_buf,1),
							ReadUint32(tmp_buf,5),
							ReadUint32(tmp_buf,9),
							this);
					request(r);
				}
				break;
			case PIECE:
				if (len < 9)
				{
					Out() << "len err PIECE" << endl;
				//	fatalError(this);
					return;
				}
				
				snub_timer.update();
				
				{
					Piece p(ReadUint32(tmp_buf,1),
							ReadUint32(tmp_buf,5),
							len - 9,this,tmp_buf+9);
					piece(p);
				}
				break;
			case CANCEL:
				if (len != 13)
				{
					Out() << "len err CANCEL" << endl;
				//	fatalError(this);
					return;
				}
				
				{
					Request r(ReadUint32(tmp_buf,1),
							  ReadUint32(tmp_buf,5),
							  ReadUint32(tmp_buf,9),
							  this);
					canceled(r);
				}
				break;
		}
	}
	
	void Peer::sendData(const Uint8* data,Uint32 len,bool record)
	{
		sock->writeBlock((const char*)data,len);
		if (record)
			speed->onWrite(len);
	}
	
	Uint32 Peer::getUploadRate() const 
	{
		return speed->uploadRate();
	}
	
	Uint32 Peer::getDownloadRate() const 
	{
		return speed->downloadRate();
	}
	
	bool Peer::readyToSend() const 
	{
		return true;
	}
	
	void Peer::updateSpeed()
	{
		speed->update();
	}
	
	bool Peer::isSnubbed() const
	{
		return snub_timer.getElapsedSinceUpdate() >= 60000;
	}
}

#include "peer.moc"
