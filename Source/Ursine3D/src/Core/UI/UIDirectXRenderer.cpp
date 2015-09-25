#include "UrsinePrecompiled.h"

#if defined(URSINE_GRAPHICS_DIRECTX)

#include "UIDirectXRenderer.h"


namespace ursine
{
  UIDirectXRenderer::UIDirectXRenderer(void)
      : m_width( 0 )
      , m_height( 0 )
  {
    m_gfxMgr = Application::Instance->GetCoreSystem<ursine::GfxAPI>( );
    m_uiHandle = m_gfxMgr->UIMgr.CreateUI( );
  }

  UIDirectXRenderer::~UIDirectXRenderer(void)
  {
      //@UI
      //m_gfxMgr->UIMgr.DestroyUI( m_uiHandle );
      //m_uiHandle = 0;
  }

  void UIDirectXRenderer::Draw(GFXHND viewport)
  {
      //@UI
      m_gfxMgr->UIMgr.GetUI( m_uiHandle ).Draw( viewport );
  }

  bool UIDirectXRenderer::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &bounds)
  {
      bounds = m_viewport;

      return true;
  }

  void UIDirectXRenderer::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
  {
      m_gfxMgr->UIMgr.GetUI( m_uiHandle ).OnPopupShow( browser, show );
  }

  void UIDirectXRenderer::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &bounds)
  {
      m_gfxMgr->UIMgr.GetUI( m_uiHandle ).OnPopupSize( browser, bounds );
  }

  void UIDirectXRenderer::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& regions, const void *buffer, int width, int height)
  {
      m_gfxMgr->UIMgr.GetUI( m_uiHandle ).OnPaint( browser, type, regions, buffer, width, height );
  }

  void UIDirectXRenderer::paintView(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& regions, const void *buffer, int width, int height)
  {
      //@UI
      m_gfxMgr->UIMgr.GetUI( m_uiHandle ).paintPopup( browser, type, regions, buffer, width, height );
  }

  void UIDirectXRenderer::paintPopup(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &regions, const void *buffer, int width, int height)
  {
      //@UI
      m_gfxMgr->UIMgr.GetUI( m_uiHandle ).paintPopup( browser, type, regions, buffer, width, height );
  }
}

#endif // defined(URSINE_GRAPHICS_DIRECTX)