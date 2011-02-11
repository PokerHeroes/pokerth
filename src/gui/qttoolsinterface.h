/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
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
#ifndef QTTOOLSINTERFACE_H
#define QTTOOLSINTERFACE_H

#include <string>

class QtToolsInterface
{
public:
	virtual ~QtToolsInterface();

	//qthelper.cpp
	virtual std::string stringToUtf8(const std::string &) =0;
	virtual std::string stringFromUtf8(const std::string &) =0;
	virtual std::string getDefaultLanguage() =0;
	virtual std::string getDataPathStdString(const char *argv0) =0;


};

QtToolsInterface *CreateQtToolsWrapper();

#endif
