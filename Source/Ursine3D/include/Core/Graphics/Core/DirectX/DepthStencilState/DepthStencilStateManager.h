#pragma once

#include <vector>

#include "D3D11Forward.h"
#include "DepthStencilStateList.h"

namespace ursine
{
  namespace DXCore
  {
    class DepthStencilStateManager
    {
    public:
      void Initialize( ID3D11Device *device, ID3D11DeviceContext *devicecontext );
      void Uninitialize( );

      ID3D11DepthStencilState *GetStencilState( DEPTH_STATES state );
      void SetDepthState( DEPTH_STATES state );
    private:
      ID3D11Device *m_device;
      ID3D11DeviceContext *m_deviceContext;

      std::vector<ID3D11DepthStencilState*> m_depthStateArray;

      DEPTH_STATES m_currentState;
    };
  }
}