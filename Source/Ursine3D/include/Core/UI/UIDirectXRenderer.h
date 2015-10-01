#pragma once

#include <cef_render_handler.h>

#include "GfxAPI.h"

namespace ursine
{
  class UIDirectXRenderer : public CefRenderHandler
  {
  public:
      UIDirectXRenderer(void);
      virtual ~UIDirectXRenderer(void);

      void Draw(GFXHND camera);
      void DrawMain(void);

  protected:
      CefRect m_viewport;

      void resize(const int width, const int height);

  private:
      GfxAPI *m_gfxMgr;
      GFXHND m_uiHandle;
      int m_width, m_height;

      CefRect m_popupBounds;

      bool GetViewRect(CefRefPtr<CefBrowser> browser,
        CefRect &bounds) override;

      void OnPopupShow(CefRefPtr<CefBrowser> browser,
        bool show) override;

      void OnPopupSize(CefRefPtr<CefBrowser> browser,
        const CefRect &bounds) override;

      void OnPaint(CefRefPtr<CefBrowser> browser,
        PaintElementType type, const RectList &regions,
        const void *buffer, int width, int height) override;

      void paintView(CefRefPtr<CefBrowser> browser,
        PaintElementType type, const RectList &regions,
        const void *buffer, int width, int height);

      void paintPopup(CefRefPtr<CefBrowser> browser,
        PaintElementType type, const RectList &regions,
        const void *buffer, int width, int height);

      IMPLEMENT_REFCOUNTING(UIDirectXRenderer);
  };
}