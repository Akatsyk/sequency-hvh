#pragma once

#include <optional>
#include <d3d9.h>
#include "../libs/json.hpp"
#include "../security/string_obfuscation.h"

#define color(name, red, green, blue, alpha) static c_color name() { return c_color(red, green, blue, alpha); }
#define var_color(name, red, green, blue) static c_color name(int alpha) { return c_color(red, green, blue, alpha); }

class c_color
{
public:
	color(background, 33, 33, 33, 255)
	color(background_alternate, 55, 55, 55, 255)
	color(foreground, 238, 238, 238, 255)
	color(primary, 64, 190, 59, 255)
	color(accent, 122, 42, 151, 255)
	color(border, 186, 186, 186, 120)

	color(gradient1, 59, 177, 190, 255)
	color(gradient2, 190, 164, 59, 255)
	color(gradient3, 190, 59, 59, 255)
	color(gradient4, 217, 101, 2, 255)

	var_color(shadow, 0, 0, 0)
	color(none, 0, 0, 0, 0)

	int red, green, blue, alpha;

	c_color();
	c_color(int red, int green, int blue);
	c_color(int red, int green, int blue, int alpha);
	~c_color();


	


	D3DCOLOR direct() const;
	c_color fade_to(c_color color, float percent = 0.f, bool only_alpha = false) const;
	c_color adjust_hue(float delta) const;

	static int get_alpha_override();
	static void set_alpha_override(std::optional<int> alpha);
	float get_hue() const;
	std::tuple<float, float, float> to_hsv() const;
	c_color from_hsv(std::tuple<float, float, float>& hsv) const;
	static c_color from_hsb(float hue, float saturation, float brightness);
	c_color fuckloki(float h, float s, float v);
private:
	

	static std::optional<int> alpha_override;
};


namespace brain // не бейте 
{


#define NOCOLOR CColor(0, 0, 0, 0)
#define WHITE CColor(255, 255, 255, 255)
#define BLACK CColor(0, 0, 0, 255)
#define RED CColor(255, 0, 0, 255)
#define LIGHTRED CColor(255, 100, 100, 255)
#define WHITERED CColor(255, 65, 65, 255);
#define DARKRED CColor(120, 0, 0, 255)
#define GREEN CColor(0, 255, 0, 255)
#define BLUE CColor(0, 0, 255, 255)
#define LIGHTBLUE CColor(0, 140, 255, 255)
#define DARKGREY CColor(55, 55, 55, 255)
#define GREY CColor(70, 70, 70, 255)
#define LIGHTGREY CColor(150, 150, 150, 255)
#define HOTPINK CColor(255, 20, 147, 255)
#define CYAN CColor(0, 255, 255, 255)
#define YELLOW CColor(255, 255, 0, 255)


	class CColor
	{
	public:
		unsigned char RGBA[4];


		CColor()
		{
			RGBA[0] = 255;
			RGBA[1] = 255;
			RGBA[2] = 255;
			RGBA[3] = 255;

		}
		CColor(int r, int g, int b, int a = 255)
		{
			RGBA[0] = r;
			RGBA[1] = g;
			RGBA[2] = b;
			RGBA[3] = a;
		}

		bool operator!=(CColor color) { return RGBA[0] != color.RGBA[0] || RGBA[1] != color.RGBA[1] || RGBA[2] != color.RGBA[2] || RGBA[3] != color.RGBA[3]; }
		bool operator==(CColor color) { return RGBA[0] == color.RGBA[0] && RGBA[1] == color.RGBA[1] && RGBA[2] == color.RGBA[2] && RGBA[3] == color.RGBA[3]; }

		inline int r() const
		{
			return RGBA[0];
		}

		inline int g() const
		{
			return RGBA[1];
		}

		inline int b() const
		{
			return RGBA[2];
		}

		inline int a() const
		{
			return RGBA[3];
		}

		// returns the color from 0.f - 1.f
		static float Base(const unsigned char col)
		{
			return col / 255.f;
		}
		static CColor Inverse(const CColor color)
		{
			return CColor(255 - color.RGBA[0], 255 - color.RGBA[1], 255 - color.RGBA[2]);
		}

		static CColor Black()
		{
			return CColor(0, 0, 0);
		}

		int GetD3DColor() const
		{
			return ((int)((((RGBA[3]) & 0xff) << 24) | (((RGBA[0]) & 0xff) << 16) | (((RGBA[1]) & 0xff) << 8) | ((RGBA[2]) & 0xff)));
		}

		static CColor Red()
		{
			return CColor(255, 0, 0);
		}
		static CColor Green()
		{
			return CColor(0, 255, 26);
		}

		static CColor LightBlue()
		{
			return CColor(100, 100, 255);
		}


		static CColor White()
		{
			return CColor(255, 255, 255);
		}


		float Difference(const CColor color) const // from 0.f - 1.f with 1.f being the most different
		{
			float red_diff = fabs(Base(RGBA[0]) - Base(color.RGBA[0]));
			float green_diff = fabs(Base(RGBA[1]) - Base(color.RGBA[1]));
			float blue_diff = fabs(Base(RGBA[2]) - Base(color.RGBA[2]));
			float alpha_diff = fabs(Base(RGBA[3]) - Base(color.RGBA[3]));

			return (red_diff + green_diff + blue_diff + alpha_diff) * 0.25f;
		}

		// RGB -> HSB
		static float Hue(const CColor color)
		{
			float R = Base(color.RGBA[0]);
			float G = Base(color.RGBA[1]);
			float B = Base(color.RGBA[2]);

			float mx = max(R, max(G, B));
			float mn = min(R, min(G, B));
			if (mx == mn)
				return 0.f;

			float delta = mx - mn;

			float hue = 0.f;
			if (mx == R)
				hue = (G - B) / delta;
			else if (mx == G)
				hue = 2.f + (B - R) / delta;
			else
				hue = 4.f + (R - G) / delta;

			hue *= 60.f;
			if (hue < 0.f)
				hue += 360.f;

			return hue / 360.f;
		}
		static float Saturation(const CColor color)
		{
			float R = Base(color.RGBA[0]);
			float G = Base(color.RGBA[1]);
			float B = Base(color.RGBA[2]);

			float mx = max(R, max(G, B));
			float mn = min(R, min(G, B));

			float delta = mx - mn;

			if (mx == 0.f)
				return delta;

			return delta / mx;
		}
		static float Brightness(const CColor color)
		{
			float R = Base(color.RGBA[0]);
			float G = Base(color.RGBA[1]);
			float B = Base(color.RGBA[2]);

			return max(R, max(G, B));
		}

		float Hue() const
		{
			return Hue(*this);
		}
		float Saturation() const
		{
			return Saturation(*this);
		}
		float Brightness() const
		{
			return Brightness(*this);
		}

		static CColor FromHSB(float hue, float saturation, float brightness)
		{
			float h = hue == 1.0f ? 0 : hue * 6.0f;
			float f = h - (int)h;
			float p = brightness * (1.0f - saturation);
			float q = brightness * (1.0f - saturation * f);
			float t = brightness * (1.0f - (saturation * (1.0f - f)));

			if (h < 1)
			{
				return CColor(
					(unsigned char)(brightness * 255),
					(unsigned char)(t * 255),
					(unsigned char)(p * 255)
				);
			}
			else if (h < 2)
			{
				return CColor(
					(unsigned char)(q * 255),
					(unsigned char)(brightness * 255),
					(unsigned char)(p * 255)
				);
			}
			else if (h < 3)
			{
				return CColor(
					(unsigned char)(p * 255),
					(unsigned char)(brightness * 255),
					(unsigned char)(t * 255)
				);
			}
			else if (h < 4)
			{
				return CColor(
					(unsigned char)(p * 255),
					(unsigned char)(q * 255),
					(unsigned char)(brightness * 255)
				);
			}
			else if (h < 5)
			{
				return CColor(
					(unsigned char)(t * 255),
					(unsigned char)(p * 255),
					(unsigned char)(brightness * 255)
				);
			}
			else
			{
				return CColor(
					(unsigned char)(brightness * 255),
					(unsigned char)(p * 255),
					(unsigned char)(q * 255)
				);
			}
		}

		// HSB -> RGB




	};
}

#undef color
