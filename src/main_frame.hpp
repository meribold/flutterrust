#ifndef MAIN_FRAME_HPP_JJ6U3B49
#define MAIN_FRAME_HPP_JJ6U3B49

#include <array>
#include <cstddef>  // size_t

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
   // void onEnterControlsBox(wxMouseEvent&);
   // void onLeaveControlsBox(wxMouseEvent&);
   void onPaint(wxPaintEvent&);
   void onCreatureChoice(wxCommandEvent&);
   void onPlace(wxCommandEvent&);
   void onPlayPause(wxCommandEvent&);
   void onStep(wxCommandEvent&);

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

   World world;
};

#endif  // MAIN_FRAME_HPP_JJ6U3B49

// vim: tw=90 sts=-1 sw=3 et
