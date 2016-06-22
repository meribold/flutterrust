#ifndef MAIN_FRAME_HPP_JJ6U3B49
#define MAIN_FRAME_HPP_JJ6U3B49

#include <array>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>  // wxStaticBox
#include <wx/stattext.h>
#include <wx/textctrl.h>

class MainFrame : public wxFrame {
  public:
   MainFrame(const wxPoint& = wxDefaultPosition, const wxSize& = wxDefaultSize);

  private:
   void toggleControlsBox(wxMouseEvent&);
   // void onEnterControlsBox(wxMouseEvent&);
   // void onLeaveControlsBox(wxMouseEvent&);
   void onPaint(wxPaintEvent&);

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
};

#endif  // MAIN_FRAME_HPP_JJ6U3B49

// vim: tw=90 sts=-1 sw=3 et
