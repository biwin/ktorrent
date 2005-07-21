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
#include <kglobal.h>
#include <libtorrent/torrentcontrol.h>
#include <qdatetime.h>
#include <math.h>
#include "ktorrentviewitem.h"
#include "functions.h"

using namespace bt;





KTorrentViewItem::KTorrentViewItem(QListView* parent,bt::TorrentControl* tc)
	: KListViewItem(parent),tc(tc)
{
	update();
}


KTorrentViewItem::~KTorrentViewItem()
{}

void KTorrentViewItem::update()
{
	/*
	addColumn(i18n("File"));
	addColumn(i18n("Status"));
	addColumn(i18n("Dowloaded"));
	addColumn(i18n("Uploaded"));
	addColumn(i18n("Down Speed"));
	addColumn(i18n("Up Speed"));
	addColumn(i18n("Time Left"));
	addColumn(i18n("Peers"));
	addColumn(i18n("% Complete"));
	*/

	setText(0,tc->getTorrentName());
	setText(1,tc->getStatus());
	Uint32 nb = tc->getBytesDownloaded() > tc->getTotalBytes() ?
			tc->getTotalBytes() : tc->getBytesDownloaded();
	
	setText(2,BytesToString(nb) + " / " + BytesToString(tc->getTotalBytes()));
	setText(3,BytesToString(tc->getBytesUploaded()));
	if (tc->getBytesLeft() == 0)
		setText(4,KBytesPerSecToString(0));
	else
		setText(4,KBytesPerSecToString(tc->getDownloadRate() / 1024.0));
	setText(5,KBytesPerSecToString(tc->getUploadRate() / 1024.0));

	KLocale* loc = KGlobal::locale();
	if (tc->getBytesLeft() == 0)
	{
		setText(6,i18n("finished"));
	}
	else if (tc->getDownloadRate() != 0)
	{
		Uint32 secs = (int)floor((float)tc->getBytesLeft() / (float)tc->getDownloadRate());
		QTime t;
		t = t.addSecs(secs);
		setText(6,loc->formatTime(t,true,true));
	}
	else
	{
		setText(6,i18n("infinity"));
	}
	setText(7,QString::number(tc->getNumPeers()));
	double perc = ((double)tc->getBytesDownloaded() / tc->getTotalBytes()) * 100.0;
	if (perc > 100.0)
		perc = 100.0;
	setText(8,i18n("%1 %").arg(loc->formatNumber(perc,2)));

	/*
	setText(8,QString("%1 (%2) / %3")
			.arg(tc->getNumChunksDownloaded())
			.arg(tc->getNumChunksDownloading())
			.arg(tc->getTotalChunks()));*/
}

