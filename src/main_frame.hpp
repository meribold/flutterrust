#ifndef MAIN_FRAME_HPP_JJ6U3B49
#define MAIN_FRAME_HPP_JJ6U3B49

#include <array>
#include <cstddef>  // size_t
#include <cstdint>  // int64_t
#include <vector>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/listbox.h>  // wxListBox
#include <wx/menu.h>     // wxMenuBar
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>  // wxStaticBox
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "world.hpp"

class MainFrame : public wxFrame {
  public:
   MainFrame(const std::string& dataDir, const wxPoint& = wxDefaultPosition,
             const wxSize& = wxDefaultSize);

  private:
   std::int64_t panelToWorldX(int panelX);
   std::int64_t panelToWorldY(int panelY);
   wxRect getTileArea(int x, int y);

   void updateAttributes(std::size_t creatureIndex);

   void toggleControlsBox(wxMouseEvent&);
   void onPaint(wxPaintEvent&);
   void onCreatureChoice(wxCommandEvent&);
   void onPlace(wxCommandEvent&);
   void onPlayPause(wxCommandEvent&);
   void onStep(wxCommandEvent&);

   // Process a wxEVT_LEFT_DOWN; captures the mouse.
   void onLeftDown(wxMouseEvent&);
   // Process a wxEVT_MOUSE_CAPTURE_LOST; handling this event is mandatory for an
   // application that captures the mouse.
   void onCaptureLost(wxMouseCaptureLostEvent&);
   void onMotion(wxMouseEvent&);  // Process a wxEVT_MOTION.
   void onLeftUp(wxMouseEvent&);  // Process a wxEVT_LEFT_UP.

   // Process a wxEVT_CONTEXT_MENU inside the worldPanel.
   void onContextMenuRequested(wxContextMenuEvent&);
   // Process a wxEVT_MENU when an item from the worldPanel's context menu is selected.
   void onMenuItemSelected(wxCommandEvent&);

   // All this is given in pixels.
   const int tileSize = 32;  // Signed, because using an unsigned type in operations with
                             // signed ones can cause the signed operands to be converted
                             // to unsigned types ("usual arithmetic conversions").
   std::int64_t scrollOffX = 0, scrollOffY = 0;
   wxPoint oldMousePos;

   wxPoint contextMenuPos;

   wxMenuBar* menuBar;
   wxPanel* topPanel;
   wxBoxSizer* topSizer;
   wxPanel* worldPanel;
   wxBoxSizer* worldPanelSizer;
   wxStaticBoxSizer* controlsSizer;
   wxStaticBox* controlsBox;
   wxChoice* creatureChoice;
   std::array<wxStaticText*, 3> propertyLabels;
   std::array<wxTextCtrl*, 3> propertyEntries;
   wxTextCtrl* attributeEntry;
   // FIXME: These are probably not deleted automatically
   // (http://docs.wxwidgets.org/trunk/classwx_menu.html)
   wxMenu* waterContextMenu;
   wxMenu* landContextMenu;

   std::array<wxBitmap, 6> terrainBitmaps;
   std::vector<wxBitmap> creatureBitmaps;
   World world;
};

#endif  // MAIN_FRAME_HPP_JJ6U3B49

// vim: tw=90 sts=-1 sw=3 et
