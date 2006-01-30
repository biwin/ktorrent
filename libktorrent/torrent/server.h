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
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#ifndef BTSERVER_H
#define BTSERVER_H

#include <qptrlist.h>
#include <qobject.h>
#ifdef USE_KNETWORK_SOCKET_CLASSES
#include <kserversocket.h>
#endif
#include "globals.h"

namespace bt
{
	class PeerManager;
	class ServerAuthenticate;
	class SHA1Hash;

#ifndef USE_KNETWORK_SOCKET_CLASSES
	class ServerSocket;
#endif

	/**
	 * @author Joris Guisson
	 *
	 * Class which listens for incoming connections.
	 * Handles authentication and then hands of the new
	 * connections to a PeerManager.
	 *
	 * All PeerManager's should register with this class when they
	 * are created and should unregister when they are destroyed.
	 */
	class Server : public QObject
	{
		Q_OBJECT

		QPtrList<PeerManager> peer_managers;
		QPtrList<ServerAuthenticate> pending;
#ifdef USE_KNETWORK_SOCKET_CLASSES
		KNetwork::KServerSocket* sock;
#else
		ServerSocket* sock;
#endif
		Uint16 port;
	public:
		Server(Uint16 port);
		virtual ~Server();

		/// Check if everything is ok (are we succesfully listening on the port)
		bool isOK() const;
		
		/**
		 * Change the port.
		 * @param port The new port
		 */
		void changePort(Uint16 port);

		/// Get the port in use
		Uint16 getPortInUse() const;
		
		/**
		 * Add a PeerManager.
		 * @param pman The PeerManager
		 */
		void addPeerManager(PeerManager* pman);

		/**
		 * Remove a PeerManager.
		 * @param pman The PeerManager
		 */
		void removePeerManager(PeerManager* pman);
		
		/**
		 * Find the PeerManager given the info_hash of it's torrent.
		 * @param hash The info_hash
		 * @return The PeerManager or 0 if one can't be found
		 */
		PeerManager* findPeerManager(const SHA1Hash & hash);
		
		/**
		 * Update the Server.
		 */
		void update();

	private slots:
		void newConnection();
		void newConnection(int socket);
		void onError(int);
		
	private:
#ifndef USE_KNETWORK_SOCKET_CLASSES
		friend class ServerSocket;
#endif
	};

}

#endif
