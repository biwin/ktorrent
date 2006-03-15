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
#ifndef UTESTUNITTEST_H
#define UTESTUNITTEST_H

#include <qstring.h>

namespace utest
{

	/**
		@author Joris Guisson <joris.guisson@gmail.com>
		
		Base class for all unit tests
	*/
	class UnitTest
	{
		QString name;
	public:
		UnitTest(const QString & name);
		virtual ~UnitTest();
		
		QString getName() const {return name;}

		virtual bool doTest() = 0;
	};

}

#endif
