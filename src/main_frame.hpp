#ifndef MAIN_FRAME_HPP_JJ6U3B49
#define MAIN_FRAME_HPP_JJ6U3B49

#include <array>
#include <cstddef>  // size_t
#include <cstdint>  // int64_t

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/menu.h>  // wxMenuBar
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>  // wxStaticBox
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "world.hpp"

class MainFrame : public wxFrame {
  public:
   MainFrame(const wxPoint& = wxDefaultPosition, const wxSize& = wxDefaultSize);

  private:
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

   // All this is given in pixels.
   int tileSize = 32;  // Signed, because using an unsigned type in operations with signed
                       // ones can cause the signed operands to be converted to unsigned
                       // types ("usual arithmetic conversions").
   std::int64_t scrollOffX = 0, scrollOffY = 0;
   wxPoint oldMousePos;

   wxMenuBar* menuBar;
   wxPanel* topPanel;
   wxBoxSizer* topSizer;
   wxPanel* worldPanel;
   wxBoxSizer* worldPanelSizer;
   wxStaticBoxSizer* controlsSizer;
   wxStaticBox* controlsBox;
   wxChoice* creatureChoice;
   std::array<wxStaticText*, 4> propertyLabels;
   std::array<wxTextCtrl*, 4> propertyEntries;
   wxButton* placeCreatureButton;
   wxButton* playPauseButton;
   wxButton* stepButton;

   std::array<wxBitmap, 6> terrainBitmaps;
   World world;
};

#endif  // MAIN_FRAME_HPP_JJ6U3B49

// vim: tw=90 sts=-1 sw=3 et
