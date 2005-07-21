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
#include <klocale.h>
#include <libtorrent/peer.h>
#include "peerview.h"
#include "functions.h"

PeerView::PeerView(QWidget *parent, const char *name)
		: KListView(parent, name)
{
	addColumn(i18n("ID"));
	addColumn(i18n("Down Speed"));
	addColumn(i18n("Up Speed"));
	addColumn(i18n("Choked"));
	addColumn(i18n("Snubbed"));
	connect(&timer,SIGNAL(timeout()),this,SLOT(update()));
	timer.start(250);
}


PeerView::~PeerView()
{}

void PeerView::addPeer(bt::Peer* peer)
{
	KListViewItem* i = new KListViewItem(
			this,
			peer->getPeerID().toString(),
			KBytesPerSecToString(peer->getDownloadRate()),
			KBytesPerSecToString(peer->getUploadRate()),
			peer->isChoked() ? "yes" : "no",
			peer->isSnubbed() ? "yes" : "no");
	items.insert(peer,i);
}

void PeerView::removePeer(bt::Peer* peer)
{
	KListViewItem* it = items[peer];
	delete it;
	items.erase(peer);
}

void PeerView::update()
{
	QMap<bt::Peer*,KListViewItem*>::iterator i = items.begin();
	while (i != items.end())
	{
		KListViewItem* it = i.data();
		bt::Peer* peer = i.key();
		it->setText(0,peer->getPeerID().toString());
		it->setText(1,KBytesPerSecToString(peer->getDownloadRate() / 1024.0));
		it->setText(2,KBytesPerSecToString(peer->getUploadRate() / 1024.0));
		it->setText(3,peer->isChoked() ? i18n("yes") : i18n("no"));
		it->setText(4,peer->isSnubbed() ? i18n("yes") : i18n("no"));
		i++;
	}
}

void PeerView::removeAll()
{
	items.clear();
	clear();
}
	
#include "peerview.moc"
