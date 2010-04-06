/* Copyright (C) 2010 Mobile Sorcery AB

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include "PointerType.h"

using namespace std;

PointerType::PointerType() : Base(EPointerType) {
}

void PointerType::fromParseNode(const ParseNode& node) {
	mType = parseType(node, mIsConst);
	if(!mType) {
		int a = 2;
	}

	mSize = node.getIntAttr("size");
	mAlign = node.getIntAttr("align");
}

const Base* PointerType::getType() const {
	return mType;
}

bool PointerType::isConst() const {
	return mIsConst;
}

string PointerType::toString() const {
	return System::genstr("%s%s*", (mIsConst?"const ":""), mType->toString().c_str());
}