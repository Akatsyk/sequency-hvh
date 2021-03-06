#pragma once
#include "macros.h"
#include "c_color.h"
#include "c_vector2d.h"
#include "c_vector3d.h"
#include <winnt.h>
#include "c_material_system.h"
#include "c_panel.h"

enum FontDrawType
{
	FONT_DRAW_DEFAULT = 0,
	FONT_DRAW_NONADDITIVE,
	FONT_DRAW_ADDITIVE,
	FONT_DRAW_TYPE_COUNT = 2,
};

enum FontFlags
{
	FONTFLAG_NONE,
	FONTFLAG_ITALIC = 0x001,
	FONTFLAG_UNDERLINE = 0x002,
	FONTFLAG_STRIKEOUT = 0x004,
	FONTFLAG_SYMBOL = 0x008,
	FONTFLAG_ANTIALIAS = 0x010,
	FONTFLAG_GAUSSIANBLUR = 0x020,
	FONTFLAG_ROTARY = 0x040,
	FONTFLAG_DROPSHADOW = 0x080,
	FONTFLAG_ADDITIVE = 0x100,
	FONTFLAG_OUTLINE = 0x200,
	FONTFLAG_CUSTOM = 0x400,
	FONTFLAG_BITMAP = 0x800,
};

enum FontRenderFlag_t
{
	FONT_LEFT = 0,
	FONT_RIGHT = 1 << 1,
	FONT_CENTER = 1 << 2,
	FONT_OUTLINE = 1 << 3
};

using VPANEL = unsigned int;

namespace vgui
{
	typedef unsigned long HFont;
	typedef unsigned int VPANEL;
};

namespace F
{
	extern vgui::HFont Menu;
	extern vgui::HFont ESPInfo;
	extern vgui::HFont ESPWeapon;
	extern vgui::HFont ESP;
	extern vgui::HFont Events;
	extern vgui::HFont BombTimer;
	extern vgui::HFont Indicators;
	extern c_vector2d screen_size;
}

// Refactor these two
struct CharRenderInfo
{
	// Text pos
	int				x, y;
	// Top left and bottom right
	// This is now a pointer to an array maintained by the surface, to avoid copying the data on the 360
	void* verts;
	int				textureId;
	int				abcA;
	int				abcB;
	int				abcC;
	int				fontTall;
	vgui::HFont			currentFont;
	// In:
	FontDrawType	drawType;
	wchar_t			ch;

	// Out
	bool			valid;
	// In/Out (true by default)
	bool			shouldclip;
};

struct Vertex_t
{
	c_vector2d	mPosition;
	c_vector2d	m_TexCoord;

	Vertex_t() {}

	Vertex_t(const c_vector2d& pos, const c_vector2d& coord = c_vector2d(0, 0))
	{
		mPosition = pos;
		m_TexCoord = coord;
	}

	void Init(const c_vector2d& pos, const c_vector2d& coord = c_vector2d(0, 0))
	{
		mPosition = pos;
		m_TexCoord = coord;
	}
};

class IEngineVGui
{
public:
	virtual					~IEngineVGui(void) { }

	virtual vgui::VPANEL	GetPanel(int type) = 0;

	virtual bool			IsGameUIVisible() = 0;
};

struct IntRect
{
	int x0;
	int y0;
	int x1;
	int y1;
};
template<typename T, typename ...Args>
constexpr auto callVirtualM2ethod2(void* classBase, int index, Args... args) noexcept
{
	return ((*reinterpret_cast<T(__thiscall***)(void*, Args...)>(classBase))[index])(classBase, args...);
}


struct Vertex_t2
{
	c_vector2d	mPosition;
	c_vector2d	m_TexCoord;

	Vertex_t2() {}
	Vertex_t2(const c_vector2d& pos, const c_vector2d& coord = c_vector2d(0, 0))
	{
		mPosition = pos;
		m_TexCoord = coord;
	}
	void Init(const c_vector2d& pos, const c_vector2d& coord = c_vector2d(0, 0))
	{
		mPosition = pos;
		m_TexCoord = coord;
	}
};
typedef  Vertex_t2 FontVertex_t2;
template <class T>
static T GetFunction1(void* instance, int index)
{
	const auto vtable = *static_cast<void***>(instance);
	return reinterpret_cast<T>(vtable[index]);
}
class Surface_dyb {
public:


	static constexpr unsigned font{ 0x1d }; // builtin font from vgui_spew_fonts

	enum EFontFlags
	{
		FONTFLAG_NONE,
		FONTFLAG_ITALIC = 0x001,
		FONTFLAG_UNDERLINE = 0x002,
		FONTFLAG_STRIKEOUT = 0x004,
		FONTFLAG_SYMBOL = 0x008,
		FONTFLAG_ANTIALIAS = 0x010,
		FONTFLAG_GAUSSIANBLUR = 0x020,
		FONTFLAG_ROTARY = 0x040,
		FONTFLAG_DROPSHADOW = 0x080,
		FONTFLAG_ADDITIVE = 0x100,
		FONTFLAG_OUTLINE = 0x200,
		FONTFLAG_CUSTOM = 0x400,
		FONTFLAG_BITMAP = 0x800,
	};
	
	constexpr void setDrawColor(int r, int g, int b, int a = 255) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 15, r, g, b, a);
	}

	constexpr void setDrawColor(const float color[3], int a = 255) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 15, static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255), a);
	}

	constexpr void setDrawColor(std::tuple<float, float, float> color, int a = 255) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 15, static_cast<int>(std::get<0>(color) * 255), static_cast<int>(std::get<1>(color) * 255), static_cast<int>(std::get<2>(color) * 255), a);
	}

	template <typename T>
	constexpr void drawFilledRect(T x0, T y0, T x1, T y1) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 16, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
	}
	void DrawSetColor(brain::CColor col)
	{
		typedef void(__thiscall* Fn)(void*, brain::CColor);
		return ((Fn)GetFunction1<Fn>(this, 14))(this, col);
	}
	void DrawTexturedPolygon(int vtxCount, FontVertex_t2* vtx, bool bClipVertices = true)
	{
		typedef void(__thiscall* Fn)(PVOID, int, FontVertex_t2*, bool);
		return GetFunction1< Fn >(this, 106)(this, vtxCount, vtx, bClipVertices);
	}
	void DrawFilledRect1(int x, int y, int x2, int y2)
	{
		typedef void(__thiscall* Fn)(void*, int, int, int, int);
		return ((Fn)GetFunction1<Fn>(this, 16))(this, x, y, x2, y2);
	}

	void DrawFilledRect2(int x1, int y1, int x2, int y2, brain::CColor color)
	{
		DrawSetColor(color);
		DrawFilledRect1(x1, y1, x2, y2);
	}

	template <typename T>
	constexpr void drawOutlinedRect(T x0, T y0, T x1, T y1) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 18, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
	}

	template <typename T>
	constexpr void drawLine(T x0, T y0, T x1, T y1) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 19, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
	}

	constexpr void drawPolyLine(int* xs, int* ys, int pointCount) noexcept
	{
		callVirtualM2ethod2<void, int*, int*, int>(this, 20, xs, ys, pointCount);
	}

	constexpr void setTextFont(unsigned font) noexcept
	{
		callVirtualM2ethod2<void, unsigned>(this, 23, font);
	}

	constexpr void setTextColor(int r, int g, int b, int a = 255) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 25, r, g, b, a);
	}

	constexpr void setTextColor(const float color[3], int a = 255) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 25, static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255), a);
	}


	void DrawSetTextColor(brain::CColor Color) noexcept
	{
		typedef void(__thiscall* Fn)(void*, brain::CColor);
		return ((Fn)GetFunction1<Fn>(this, 24))(this, Color);
	}
	void DrawSetTextFont(unsigned int Font)
	{
		typedef void(__thiscall* Fn)(void*, unsigned int);
		return ((Fn)GetFunction1<Fn>(this, 23))(this, Font);
	}
	void DrawSetTextPos(int x, int y)
	{
		typedef void(__thiscall* Fn)(void*, int, int);
		return ((Fn)GetFunction1<Fn>(this, 26))(this, x, y);
	}
	void DrawPrintText(const wchar_t* Text, int Len, int DrawType = 0)
	{
		typedef void(__thiscall* Fn)(void*, wchar_t const*, int, int);
		return ((Fn)GetFunction1<Fn>(this, 28))(this, Text, Len, DrawType);
	}

	constexpr void setTextColor(std::tuple<float, float, float> color, int a = 255) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 25, static_cast<int>(std::get<0>(color) * 255), static_cast<int>(std::get<1>(color) * 255), static_cast<int>(std::get<2>(color) * 255), a);
	}

	template <typename T>
	constexpr void setTextPosition(T x, T y) noexcept
	{
		callVirtualM2ethod2<void, int, int>(this, 26, static_cast<int>(x), static_cast<int>(y));
	}

	constexpr void printText(const std::wstring_view text, int drawType = 0) noexcept
	{
		callVirtualM2ethod2<void, const wchar_t*, int, int>(this, 28, text.data(), text.length(), drawType);
	}

	constexpr auto getScreenSize() noexcept
	{
		int width{ }, height{ };
		callVirtualM2ethod2<void, int&, int&>(this, 44, width, height);
		return std::make_pair(width, height);
	}

	constexpr void unlockCursor() noexcept
	{
		callVirtualM2ethod2<void>(this, 66);
	}
	unsigned long SCreateFont()
	{
		typedef unsigned int(__thiscall* OriginalFn)(PVOID);
		return GetFunction1< OriginalFn >(this, 71)(this);
	}
	void SetFontGlyphSet(unsigned long& font, const char* windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, unsigned long, const char*, int, int, int, int, int, int, int);
		GetFunction1< OriginalFn >(this, 72)(this, font, windowsFontName, tall, weight, blur, scanlines, flags, 0, 0);
	}
	unsigned int CreateF(std::string font_name, int size, int weight, int blur, int scanlines, int flags)
	{
		auto font = SCreateFont();
		SetFontGlyphSet(font, font_name.c_str(), size, weight, blur, scanlines, flags);

		return font;
	}

	constexpr unsigned createFont() noexcept
	{
		return callVirtualM2ethod2<unsigned>(this, 71);
	}

	constexpr bool setFontGlyphSet(unsigned font, const char* fontName, int tall, int weight, int blur, int scanlines, int flags, int rangeMin = 0, int rangeMax = 0) noexcept
	{
		return callVirtualM2ethod2<bool, unsigned, const char*, int, int, int, int, int, int, int>(this, 72, font, fontName, tall, weight, blur, scanlines, flags, rangeMin, rangeMax);
	}

	constexpr auto getTextSize(unsigned font, const wchar_t* text) noexcept
	{
		int width{ }, height{ };
		callVirtualM2ethod2<void, unsigned, const wchar_t*, int&, int&>(this, 79, font, text, width, height);
		return std::make_pair(width, height);
	}
	bool GetTextSize3112(int Font, const wchar_t* _Input, int& Wide, int& Tall)
	{
		typedef bool(__thiscall* Fn)(void*, int, const wchar_t*, int&, int&);
		return ((Fn)GetFunction1<Fn>(this, 79))(this, Font, _Input, Wide, Tall);
	}
	c_vector2d GetTextSize31(unsigned int Font, std::string Input)
	{
		/* char -> wchar */
		size_t size = Input.size() + 1;
		auto wide_buffer = std::make_unique<wchar_t[]>(size);
		mbstowcs_s(0, wide_buffer.get(), size, Input.c_str(), size - 1);

		int width, height;
		GetTextSize3112(Font, wide_buffer.get(), width, height);

		return c_vector2d(width, height);
	}



	void GetTextSize_glup(int font, const wchar_t* text, int& wide, int& tall)
	{
		typedef void(__thiscall* OriginalFn)(void*, int, const wchar_t*, int&, int&);
		return GetFunction1<OriginalFn>(this, 79)(this, font, text, wide, tall);
	}


	template <typename T>
	constexpr void drawOutlinedCircle(T x, T y, int r, int seg) noexcept
	{
		callVirtualM2ethod2<void, int, int, int, int>(this, 103, static_cast<int>(x), static_cast<int>(y), r, seg);
	}

};


//-----------------------------------------------------------------------------
// Purpose: Wraps contextless windows system functions
//-----------------------------------------------------------------------------
class c_surface : public IAppSystem
{
public:
	virtual void          RunFrame() = 0;
	virtual vgui::VPANEL  GetEmbeddedPanel() = 0;
	virtual void          SetEmbeddedPanel(vgui::VPANEL pPanel) = 0;
	virtual void          PushMakeCurrent(vgui::VPANEL panel, bool useInsets) = 0;
	virtual void          PopMakeCurrent(vgui::VPANEL panel) = 0;
	virtual void          draw_set_color2(int r, int g, int b, int a) = 0;
	virtual void          draw_set_color2(c_color col) = 0;
	virtual void          draw_filled_rect2(int x0, int y0, int x1, int y1) = 0;
	virtual void          DrawFilledRectArray(IntRect* pRects, int numRects) = 0;
	virtual void          DrawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void          DrawLine(int x0, int y0, int x1, int y1) = 0;
	virtual void          DrawPolyLine(int* px, int* py, int numPoints) = 0;
	virtual void          DrawSetApparentDepth(float f) = 0;
	virtual void          DrawClearApparentDepth(void) = 0;
	virtual void          DrawSetTextFont(vgui::HFont font) = 0;
	virtual void          DrawSetTextColor(int r, int g, int b, int a) = 0;
	virtual void          DrawSetTextColor(c_color col) = 0;
	virtual void          DrawSetTextPos(int x, int y) = 0;
	virtual void          DrawGetTextPos(int& x, int& y) = 0;
	virtual void          DrawPrintText(const wchar_t* text, int textLen, FontDrawType drawType = FontDrawType::FONT_DRAW_DEFAULT) = 0;
	virtual void          DrawUnicodeChar(wchar_t wch, FontDrawType drawType = FontDrawType::FONT_DRAW_DEFAULT) = 0;
	virtual void          DrawFlushText() = 0;
	virtual void* CreateHTMLWindow(void* events, vgui::VPANEL context) = 0;
	virtual void          PaintHTMLWindow(void* htmlwin) = 0;
	virtual void          DeleteHTMLWindow(void* htmlwin) = 0;
	virtual int           DrawGetTextureId(char const* filename) = 0;
	virtual bool          DrawGetTextureFile(int id, char* filename, int maxlen) = 0;
	virtual void          DrawSetTextureFile(int id, const char* filename, int hardwareFilter, bool forceReload) = 0;
	virtual void          DrawSetTextureRGBA(int id, const unsigned char* rgba, int wide, int tall) = 0;
	virtual void          DrawSetTexture(int id) = 0;
	virtual void          DeleteTextureByID(int id) = 0;
	virtual void          DrawGetTextureSize(int id, int& wide, int& tall) = 0;
	virtual void          DrawTexturedRect(int x0, int y0, int x1, int y1) = 0;
	virtual bool          IsTextureIDValid(int id) = 0;
	virtual int           CreateNewTextureID(bool procedural = false) = 0;
	virtual void          GetScreenSize(int& wide, int& tall) = 0;
	virtual void          SetAsTopMost(vgui::VPANEL panel, bool state) = 0;
	virtual void          BringToFront(vgui::VPANEL panel) = 0;
	virtual void          SetForegroundWindow(vgui::VPANEL panel) = 0;
	virtual void          SetPanelVisible(vgui::VPANEL panel, bool state) = 0;
	virtual void          SetMinimized(vgui::VPANEL panel, bool state) = 0;
	virtual bool          IsMinimized(vgui::VPANEL panel) = 0;
	virtual void          FlashWindow(vgui::VPANEL panel, bool state) = 0;
	virtual void          SetTitle(vgui::VPANEL panel, const wchar_t* title) = 0;
	virtual void          SetAsToolBar(vgui::VPANEL panel, bool state) = 0;
	virtual void          CreatePopup(vgui::VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true, bool kbInput = true) = 0;
	virtual void          SwapBuffers(vgui::VPANEL panel) = 0;
	virtual void          Invalidate(vgui::VPANEL panel) = 0;
	virtual void          SetCursor(unsigned long cursor) = 0;
	virtual bool          IsCursorVisible() = 0;
	virtual void          ApplyChanges() = 0;
	virtual bool          IsWithin(int x, int y) = 0;
	virtual bool          HasFocus() = 0;
	virtual bool          SupportsFeature(int /*SurfaceFeature_t*/ feature) = 0;
	virtual void          RestrictPaintToSinglePanel(vgui::VPANEL panel, bool bForceAllowNonModalSurface = false) = 0;
	virtual void          SetModalPanel(vgui::VPANEL) = 0;
	virtual vgui::VPANEL  GetModalPanel() = 0;
	virtual void          UnlockCursor() = 0;
	virtual void          LockCursor() = 0;
	virtual void          SetTranslateExtendedKeys(bool state) = 0;
	virtual vgui::VPANEL  GetTopmostPopup() = 0;
	virtual void          SetTopLevelFocus(vgui::VPANEL panel) = 0;
	virtual vgui::HFont   CreateFont_() = 0;
	virtual bool          SetFontGlyphSet(vgui::HFont font, const char* windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0) = 0;
	virtual bool          AddCustomFontFile(const char* fontFileName) = 0;
	virtual int           GetFontTall(vgui::HFont font) = 0;
	virtual int           GetFontAscent(vgui::HFont font, wchar_t wch) = 0;
	virtual bool          IsFontAdditive(vgui::HFont font) = 0;
	virtual void          GetCharABCwide(vgui::HFont font, int ch, int& a, int& b, int& c) = 0;
	virtual int           GetCharacterWidth(vgui::HFont font, int ch) = 0;
	virtual void          GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall) = 0;
	virtual vgui::VPANEL  GetNotifyPanel() = 0;
	virtual void          SetNotifyIcon(vgui::VPANEL context, unsigned long icon, vgui::VPANEL panelToReceiveMessages, const char* text) = 0;
	virtual void          PlaySound_(const char* fileName) = 0;
	virtual int           GetPopupCount() = 0;
	virtual vgui::VPANEL  GetPopup(int index) = 0;
	virtual bool          ShouldPaintChildPanel(vgui::VPANEL childPanel) = 0;
	virtual bool          RecreateContext(vgui::VPANEL panel) = 0;
	virtual void          AddPanel(vgui::VPANEL panel) = 0;
	virtual void          ReleasePanel(vgui::VPANEL panel) = 0;
	virtual void          MovePopupToFront(vgui::VPANEL panel) = 0;
	virtual void          MovePopupToBack(vgui::VPANEL panel) = 0;
	virtual void          SolveTraverse(vgui::VPANEL panel, bool forceApplySchemeSettings = false) = 0;
	virtual void          PaintTraverse(vgui::VPANEL panel) = 0;
	virtual void          EnableMouseCapture(vgui::VPANEL panel, bool state) = 0;
	virtual void          GetWorkspaceBounds(int& x, int& y, int& wide, int& tall) = 0;
	virtual void          GetAbsoluteWindowBounds(int& x, int& y, int& wide, int& tall) = 0;
	virtual void          GetProportionalBase(int& width, int& height) = 0;
	virtual void          CalculateMouseVisible() = 0;
	virtual bool          NeedKBInput() = 0;
	virtual bool          HasCursorPosFunctions() = 0;
	virtual void          SurfaceGetCursorPos(int& x, int& y) = 0;
	virtual void          SurfaceSetCursorPos(int x, int y) = 0;
	virtual void          DrawTexturedLine(const Vertex_t& a, const Vertex_t& b) = 0;
	virtual void          DrawOutlinedCircle(int x, int y, int radius, int segments) = 0;
	virtual void          DrawTexturedPolyLine(const Vertex_t* p, int n) = 0;
	virtual void          DrawTexturedSubRect(int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1) = 0;
	virtual void          DrawTexturedPolygon(int n, Vertex_t* pVertice, bool bClipVertices = true) = 0;
	virtual const wchar_t* GetTitle(VPANEL panel) = 0;
	virtual bool IsCursorLocked(void) const = 0;
	virtual void SetWorkspaceInsets(int left, int top, int right, int bottom) = 0;

	// Lower level char drawing code, call DrawGet then pass in info to DrawRender
	virtual bool DrawGetUnicodeCharRenderInfo(wchar_t ch, CharRenderInfo& info) = 0;
	virtual void DrawRenderCharFromInfo(const CharRenderInfo& info) = 0;

	// global alpha setting functions
	// affect all subsequent draw calls - shouldn't normally be used directly, only in Panel::PaintTraverse()
	virtual void DrawSetAlphaMultiplier(float alpha /* [0..1] */) = 0;
	virtual float DrawGetAlphaMultiplier() = 0;

	// web browser
	virtual void SetAllowHTMLJavaScript(bool state) = 0;

	// video mode changing
	virtual void OnScreenSizeChanged(int nOldWidth, int nOldHeight) = 0;

	virtual void CreateCursorFromFile(char const* curOrAniFile, char const* pPathID = 0) = 0;

	// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
	virtual void* DrawGetTextureMatInfoFactory(int id) = 0;

	virtual void PaintTraverseEx(VPANEL panel, bool paintPopups = false) = 0;

	virtual float GetZPos() const = 0;

	// From the Xbox
	virtual void SetPanelForInput(VPANEL vpanel) = 0;
	virtual void DrawFilledRectFastFade(int x0, int y0, int x1, int y1, int fadeStartPt, int fadeEndPt, unsigned int alpha0, unsigned int alpha1, bool bHorizontal) = 0;
	virtual void DrawFilledRectFade(int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal) = 0;
	virtual void DrawSetTextureRGBAEx(int id, const unsigned char* rgba, int wide, int tall, ImageFormat imageFormat) = 0;
	virtual void DrawSetTextScale(float sx, float sy) = 0;
	virtual bool SetBitmapFontGlyphSet(vgui::HFont font, const char* windowsFontName, float scalex, float scaley, int flags) = 0;
	// adds a bitmap font file
	virtual bool AddBitmapFontFile(const char* fontFileName) = 0;
	// sets a symbol for the bitmap font
	virtual void SetBitmapFontName(const char* pName, const char* pFontFilename) = 0;
	// gets the bitmap font filename
	virtual const char* GetBitmapFontName(const char* pName) = 0;
	virtual void ClearTemporaryFontCache(void) = 0;
	virtual void* GetIconImageForFullPath(char const* pFullPath) = 0;
	virtual void DrawUnicodeString(const wchar_t* pwString, FontDrawType drawType = FONT_DRAW_DEFAULT) = 0;
	virtual void PrecacheFontCharacters(vgui::HFont font, const wchar_t* pCharacters) = 0;
	// Console-only.  Get the string to use for the current video mode for layout files.
	virtual const char* GetResolutionKey(void) const = 0;

	virtual const char* GetFontName(vgui::HFont font) = 0;
	virtual const char* GetFontFamilyName(vgui::HFont font) = 0;
	virtual void GetKernedCharWidth(vgui::HFont font, wchar_t ch, wchar_t chBefore, wchar_t chAfter, float& wide, float& abcA) = 0;

	virtual bool ForceScreenSizeOverride(bool bState, int wide, int tall) = 0;
	// LocalToScreen, ParentLocalToScreen fixups for explicit PaintTraverse calls on Panels not at 0, 0 position
	virtual bool ForceScreenPosOffset(bool bState, int x, int y) = 0;
	virtual void OffsetAbsPos(int& x, int& y) = 0;


	// Causes fonts to get reloaded, etc.
	virtual void ResetFontCaches() = 0;

	void DrawT(int X, int Y, c_color Color, int Font, bool Center, const char* _Input, ...)
	{
		int apple = 0;
		char Buffer[2048] = { '\0' };
		va_list Args;

		va_start(Args, _Input);
		vsprintf_s(Buffer, _Input, Args);
		va_end(Args);

		size_t Size = strlen(Buffer) + 1;
		wchar_t* WideBuffer = new wchar_t[Size];

		mbstowcs_s(0, WideBuffer, Size, Buffer, Size - 1);

		int Width = 0, Height = 0;

		if (Center)
			GetTextSize(Font, WideBuffer, Width, Height);

		DrawSetTextColor(Color.red, Color.green, Color.blue, Color.alpha);
		DrawSetTextFont(Font);
		DrawSetTextPos(X - (Width / 2), Y);
		DrawPrintText(WideBuffer, wcslen(WideBuffer));
	}

	vfunc(14, DrawSetColor(c_color color), void(__thiscall*)(c_surface*, c_color))(color)
	
	vfunc(15, DrawSetColor(int r, int g, int b, int a), void(__thiscall*)(c_surface*, int, int, int, int))(r, g, b, a)
	vfunc(16, draw_filled_rect(int x, int y, int x1, int x2), void(__thiscall*)(c_surface*, int, int, int, int))(x, y, x1, x2)

	void filled_rect(int x, int y, int w, int h, c_color color)
	{
		DrawSetColor(color.red, color.green, color.blue, color.alpha);
		draw_filled_rect(x, y, x + w, y + h);
	}

	void gain_mouse_pos(c_vector2d& last, c_vector2d& new_) {
		POINT p_mouse_pos;
		GetCursorPos(&p_mouse_pos);
		ScreenToClient(FindWindow(0, "Counter-Strike: Global Offensive"), &p_mouse_pos);
		last = new_;
		new_ = c_vector2d(static_cast<int>(p_mouse_pos.x), static_cast<int>(p_mouse_pos.y));
	}


	bool GetTextSize3112(int Font, const wchar_t* _Input, int& Wide, int& Tall)
	{
		typedef bool(__thiscall* Fn)(void*, int, const wchar_t*, int&, int&);
		return ((Fn)GetFunction1<Fn>(this, 79))(this, Font, _Input, Wide, Tall);
	}
	c_vector2d GetTextSize31(unsigned int Font, std::string Input)
	{
		/* char -> wchar */
		size_t size = Input.size() + 1;
		auto wide_buffer = std::make_unique<wchar_t[]>(size);
		mbstowcs_s(0, wide_buffer.get(), size, Input.c_str(), size - 1);

		int width, height;
		GetTextSize3112(Font, wide_buffer.get(), width, height);

		return c_vector2d(width, height);
	}


	


};

namespace Drawing
{
	extern void CreateFonts();
	extern void DrawString(vgui::HFont font, int x, int y, c_color color, DWORD alignment, const char* msg, ...);
	extern void DrawStringFont(vgui::HFont font, int x, int y, c_color clrColor, bool bCenter, const char* szText, ...);
	extern void DrawStringUnicode(vgui::HFont font, int x, int y, c_color color, bool bCenter, const wchar_t* msg, ...);
	extern void DrawRect(int x, int y, int w, int h, c_color col);
	extern void Rectangle(float x, float y, float w, float h, float px, c_color col);
	extern void Border(int x, int y, int w, int h, int line, c_color col);
	extern void DrawRectRainbow(int x, int y, int width, int height, float flSpeed, float flRainbow);
	//extern void DrawRectRainbow(int x, int y, int w, int h, float flSpeed, float& flRainbow);
	extern void DrawRectGradientVertical(int x, int y, int w, int h, c_color color1, c_color color2);
	extern void DrawRectGradientHorizontal(int x, int y, int w, int h, c_color color1, c_color color2);
	extern void DrawPixel(int x, int y, c_color col);
	extern void DrawOutlinedRect(int x, int y, int w, int h, c_color col);
	extern void DrawOutlinedCircle(int x, int y, int r, int s, c_color col);
	extern void DrawLine(int x0, int y0, int x1, int y1, c_color col);
	extern void DrawCorner(int iX, int iY, int iWidth, int iHeight, bool bRight, bool bDown, c_color colDraw);
	extern void DrawRoundedBox(int x, int y, int w, int h, int r, int v, c_color col);
	extern void Triangle(c_vector3d ldcorner, c_vector3d rucorner, c_color col);
	extern void DrawPolygon(int count, Vertex_t* Vertexs, c_color color);
	extern void DrawBox(int x, int y, int w, int h, c_color c_color);
	extern bool ScreenTransform(const c_vector3d& point, c_vector3d& screen);
	extern bool WorldToScreen(const c_vector3d& origin, c_vector3d& screen);
	extern RECT GetViewport();
	extern int	GetStringWidth(vgui::HFont font, const char* msg, ...);
	extern RECT GetTextSize(vgui::HFont font, const char* text);
	extern void Draw3DBox(c_vector3d* boxVectors, c_color color);
	extern void rotate_point(c_vector2d& point, c_vector2d origin, bool clockwise, float angle);
	extern void DrawFilledCircle(int x, int y, int radius, int segments, c_color color);
	extern void TexturedPolygon(int n, std::vector<Vertex_t> vertice, c_color color);
	extern void TexturedPolygon(int n, Vertex_t* vertice, c_color color);
	extern void filled_tilted_triangle(c_vector2d position, c_vector2d size, c_vector2d origin, bool clockwise, float angle, c_color color, bool rotate = true);
	extern void Triangle(c_vector2d one, c_vector2d two, c_vector2d three, c_color fill);
	extern void DrawCircle(float x, float y, float r, float s, c_color color);
	extern const char* LastFontName;
}


template <typename T>
static auto find(const wchar_t* module, const char* name) noexcept
{
	if (HMODULE moduleHandle = GetModuleHandleW(module))
		if (const auto createInterface = reinterpret_cast<std::add_pointer_t<T * (const char* name, int* returnCode)>>(GetProcAddress(moduleHandle, "CreateInterface")))
			if (T* foundInterface = createInterface(name, nullptr))
				return foundInterface;
	std::exit(EXIT_FAILURE);
}


class c_vgui_panel
{
	typedef void(__thiscall* engine_vgui_t)(IEngineVGui*, int);
public:
	static void hook();
private:
	inline static engine_vgui_t _engine_vgui;
	static void __stdcall EngineVGUI_Paint(int mode);
};

interface_var(c_surface, surface, "vguimatsurface.dll", "VGUI_Surface")

interface_var(Surface_dyb, surface_idiota, "vguimatsurface.dll", "VGUI_Surface")
interface_var(IEngineVGui, engine_vgui, "engine.dll", "VEngineVGui")