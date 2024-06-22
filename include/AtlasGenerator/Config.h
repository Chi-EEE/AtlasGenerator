#pragma once

#include <stdint.h>
#include <functional>
#include <algorithm>

#include "AtlasGenerator/limits.h"

namespace sc
{
	namespace AtlasGenerator
	{
		class Config {
		public:
			enum class TextureType : int
			{
				RGBA = 0,
				RGB,
				LUMINANCE_ALPHA,
				LIMINANCE
			};

		public:
			Config(
				Config::TextureType type,
				uint16_t width, uint16_t height,
				float scale, uint8_t extrude)
				: m_texture_type(type),
				m_max_width(std::clamp<uint16_t>(width, MinTextureDimension, MaxTextureDimension)),
				m_max_height(std::clamp<uint16_t>(height, MinTextureDimension, MaxTextureDimension)),
				m_scale_factor(std::clamp<uint8_t>(scale, MinScaleFactor, MaxScaleFactor)),
				m_extrude(std::clamp<uint8_t>(extrude, MinExtrude, MaxExtrude))
			{}
			virtual ~Config() = default;

		public:
			virtual TextureType type() const { return m_texture_type; };
			virtual uint16_t width() const { return m_max_width; };
			virtual uint16_t height() const { return m_max_height; };
			virtual float scale() const { return m_scale_factor; };
			virtual uint8_t extrude() const { return m_extrude; };

		private:
			const TextureType m_texture_type = TextureType::RGBA;
			const uint16_t m_max_width;
			const uint16_t m_max_height;
			const float m_scale_factor;
			const uint8_t m_extrude = 2;

		public:
			std::function<void(unsigned)> progress;
		};
	}
}