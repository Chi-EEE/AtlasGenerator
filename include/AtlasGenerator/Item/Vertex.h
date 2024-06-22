#pragma once

// SC CORE
#include <math/point.h>
#include <math/rect.h>

#include <stdint.h>

namespace sc
{
	namespace AtlasGenerator
	{
		class Vertex {
		public:
			Vertex() {
				xy = Point<int32_t>(0, 0);
				uv = Point<uint16_t>(0, 0);
			};
			Vertex(int32_t x, int32_t y, uint16_t u, uint16_t v) {
				xy = Point<int32_t>(x, y);
				uv = Point<uint16_t>(u, v);
			};

		public:
			Point<uint16_t> uv;
			Point<int32_t> xy;
		};
	}
}