#include "idaboss.h"


namespace FEATURES
{
	namespace MISC
	{
		void DrawF(int X, int Y, unsigned int Font, bool center_width, bool center_height, c_color Color, std::string Input)
		{
			/* char -> wchar */
			size_t size = Input.size() + 1;
			auto wide_buffer = std::make_unique<wchar_t[]>(size);
			mbstowcs_s(0, wide_buffer.get(), size, Input.c_str(), size - 1);

			/* check center */
			int width = 0, height = 0;
			surface()->GetTextSize(Font, wide_buffer.get(), width, height);
			if (!center_width)
				width = 0;
			if (!center_height)
				height = 0;

			/* call and draw*/
			surface()->DrawSetTextColor(c_color(255,255,255));
			surface()->DrawSetTextFont(Font);
			surface()->DrawSetTextPos(X - (width * .5), Y - (height * .5));
			surface()->DrawPrintText(wide_buffer.get(), wcslen(wide_buffer.get()), FontDrawType::FONT_DRAW_DEFAULT);
		}

		
		auto draw_text = [](const char* text, c_vector2d pos, int x_offset, c_color Color) {
			Drawing::DrawString(F::ESP, pos.x + x_offset, pos.y + 2, Color, false, text);
		};

		InGameLogger in_game_logger;
		void ColorLine::Draw(int x, int y, unsigned int font)
		{
			for (int i = 0; i < texts.size(); i++)
		//	for (int i = texts.size() - 1; i > -1; i--) 
			{

			//	draw_text(texts[i], x, y, c_color(255, 255, 255));

				Drawing::DrawString(F::ESP, x, y, colors[i], false, texts[i].c_str());
				x += surface()->GetTextSize31(F::ESP, texts[i]).x;


				/*DrawF(x, y, font, false, false, colors[i], texts[i]);
				x += surface_idiota()->GetTextSize31(font, texts[i]).x;

				*/
			}
		}

		void InGameLogger::Do()
		{
			if (log_queue.size() > max_lines_at_once)
				log_queue.erase(log_queue.begin() + max_lines_at_once, log_queue.end());

			for (int i = 0; i < log_queue.size(); i++)
			{
				auto log = log_queue[i];
				float time_delta = fabs(global_vars_base->curtime - log.time);

				int height = ideal_height + (16 * i);

				/// erase dead logs
				if (time_delta > text_time)
				{
					log_queue.erase(log_queue.begin() + i);
					break;
				}
				if (time_delta > text_time - slide_out_speed)
					height = height + (((slide_out_speed - (text_time - time_delta)) / slide_out_speed) * slide_out_distance);

				/// fade out
				if (time_delta > text_time - text_fade_out_time)
					log.color_line.ChangeAlpha(255 - (((time_delta - (text_time - text_fade_out_time)) / text_fade_out_time) * 255.f));
				/// fade in
				if (time_delta < text_fade_in_time)
					log.color_line.ChangeAlpha((time_delta / text_fade_in_time) * 255.f);

				int width = ideal_width;

				/// slide from left
				if (time_delta < text_fade_in_time)
					width = (time_delta / text_fade_in_time) * static_cast<float>(slide_in_distance) + (ideal_width - slide_in_distance);
				/// slider from right
				if (time_delta > text_time - text_fade_out_time)
					width = ideal_width + (((time_delta - (text_time - text_fade_out_time)) / text_fade_out_time) * static_cast<float>(slide_out_distance));

				//cvar()->console_color_printf(false, c_color(255, 255, 255), "popal");

			//	message2("log");

				log.color_line.Draw(width, height, fonts::ESP);
			}
		}
	}
}

