/***************************************************************************
 *   Copyright (C) 2005 by Ivan Vasić   								   *
 *   ivasic@gmail.com   												   *
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
#ifndef QUEUEDIALOG_H
#define QUEUEDIALOG_H

#include "queuedlg.h"
#include <interfaces/torrentinterface.h>
#include <torrent/queuemanager.h>

#include <qlistview.h>
#include <qstring.h>

class QueueItem: public QListViewItem
{
	public:
		QueueItem(kt::TorrentInterface* t, QListView* parent);
		
		int getPriority() { return torrentPriority; }
		void setPriority(int p);
		int compare(QListViewItem *i, int col, bool ascending ) const;
		
		void setTorrentPriority(int p);
		
	private:
		//void updatePriorities(QueueItem* to, bool from_end, int val);
		
		kt::TorrentInterface* tc;
		int torrentPriority;
};

class QueueDialog: public QueueDlg 
{
	Q_OBJECT
	public:
		QueueDialog(bt::QueueManager* qm, QWidget *parent = 0, const char *name = 0);
	public slots:
		virtual void btnMoveUp_clicked();
		virtual void btnClose_clicked();
		virtual void btnMoveDown_clicked();
   		virtual void btnDequeue_clicked();
    	virtual void btnEnqueue_clicked();
    	virtual void btnApply_clicked();
		
	private:
		///Enqueue item curr
		void enqueue(QueueItem* curr = 0);
		
		///Writes the queue order into QueueManager
		void writeQueue();
		
		QListView* torrentView;
		bt::QueueManager* qman;
};

#endif
