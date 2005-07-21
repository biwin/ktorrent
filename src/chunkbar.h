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
#ifndef CHUNKBAR_H
#define CHUNKBAR_H

#include <qwidget.h>


class QPainter;


namespace bt
{
	class TorrentControl;
}


/**
@author Joris Guisson
*/
class ChunkBar : public QWidget
{
	Q_OBJECT
public:
	ChunkBar(QWidget *parent = 0, const char *name = 0);
	virtual ~ChunkBar();

	void setTC(bt::TorrentControl* tc);
	
	virtual void paintEvent(QPaintEvent* arg1);

private:
	void drawEqual(QPainter & p);
	void drawMoreChunksThenPixels(QPainter & p);
	void drawMorePixelsThenChunks(QPainter & p);
	
private:
	bt::TorrentControl* curr_tc;

	
};

#endif
