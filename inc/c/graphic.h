// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Video
// Codifiers: @dosconio: 20240513
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef _INC_GRAPHIC
#define _INC_GRAPHIC

#include "stdinc.h"
#include "./graphic/color.h"

#ifdef _INC_CPP
namespace uni {
#endif

	typedef struct PointD {
		stduint x, y;
	public:
		PointD(stduint x = 0, stduint y = 0) : x(x), y(y) {}
	} Point, Size2;

	struct Position {
		stdint x, y, z, t;
	};

	typedef double Line2[3];// x*[0] + y*[1] + [2] {=, <, <=} 0

	struct Rectangle {
		stduint x, y;// of vertex
		stduint width, height;
		Color color;
		bool filled; // not hollow
	public:
		Rectangle() {}
		Rectangle(Point vertex, Size2 size, Color color = Color::FromRGB888(0), bool filled = true) : x(vertex.x), y(vertex.y), width(size.x), height(size.y), color(color), filled(filled) {}
		Point getVertex() const {
			return Point(x, y);
		}
		// Get Point Located at Right Bottom
		Point getVertexOpposite() { return Point(x + width, y + height); }
		Size2 getSize() const {
			return Size2(width, height);
		}
	};

	struct Circle {
		Point center;
		stduint radius;
		Color color;
		bool filled;
	public:
		Circle(Point centre, stduint r, Color color = Color::FromRGB888(0), bool filled = true) : center(centre), radius(r), color(color), filled(filled) {}
		Point getVertex() const {
			return center;
		}
		stdint getRadius() const { return radius; }
	};


#ifdef _INC_CPP
}
#endif
	
#endif

