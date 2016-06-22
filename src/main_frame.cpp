#include "main_frame.hpp"

#include <cassert>  // assert

#include <wx/colour.h>    // wxColour
#include <wx/dcbuffer.h>  // wxAutoBufferedPaintDC
#include <wx/statline.h>  // wxStaticLine

MainFrame::MainFrame(const wxPoint& pos, const wxSize& size)
    : wxFrame{nullptr, wxID_ANY, u8"flutterrust", pos, size},
      topPanel{new wxPanel{this}},
      topSizer{new wxBoxSizer{wxHORIZONTAL}},
      worldPanel{new wxPanel{topPanel}},
      worldPanelSizer{new wxBoxSizer{wxHORIZONTAL}},
      controlsSizer{new wxStaticBoxSizer{wxVERTICAL, worldPanel, u8"Controls"}},
      controlsBox{controlsSizer->GetStaticBox()},
      creatureChoice{new wxChoice{controlsBox, wxID_ANY}},
      propertyLabels{{new wxStaticText{controlsBox, wxID_ANY, u8"Strength"},
                      new wxStaticText{controlsBox, wxID_ANY, u8"Speed"},
                      new wxStaticText{controlsBox, wxID_ANY, u8"Life"},
                      new wxStaticText{controlsBox, wxID_ANY, u8"Properties"}}},
      propertyEntries{{new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
                       new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
                       new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
                       new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY}}},
      placeCreatureButton{new wxButton{controlsBox, wxID_ANY, u8"Place"}},
      playPauseButton{new wxButton{controlsBox, wxID_ANY, u8"Play"}},
      stepButton{new wxButton{controlsBox, wxID_ANY, u8"Step"}} {
   worldPanel->SetOwnBackgroundColour(wxColour{0x00, 0x00, 0x80});
   controlsBox->SetOwnForegroundColour(wxColour{0xff, 0xff, 0xff});
   topSizer->Add(worldPanel, 1, wxEXPAND);
   controlsSizer->Add(creatureChoice, 0, wxEXPAND);
   controlsSizer->Add(new wxStaticLine{controlsBox, wxID_ANY, wxDefaultPosition,
                                       wxSize{wxDefaultCoord, 12}},
                      0, wxEXPAND);
   assert(propertyEntries.size() == propertyEntries.size());
   for (std::size_t i = 0; i < propertyLabels.size(); ++i) {
      controlsSizer->Add(propertyLabels[i], 0, wxALIGN_CENTER_HORIZONTAL);
      controlsSizer->Add(propertyEntries[i], 0, wxEXPAND);
   }
   controlsSizer->AddSpacer(8);
   controlsSizer->Add(placeCreatureButton, 0, wxEXPAND);
   controlsSizer->Add(new wxStaticLine{controlsBox, wxID_ANY, wxDefaultPosition,
                                       wxSize{wxDefaultCoord, 12}},
                      0, wxEXPAND);
   {
      auto hBoxSizer = new wxBoxSizer{wxHORIZONTAL};
      hBoxSizer->Add(playPauseButton, 1);
      hBoxSizer->Add(stepButton, 1);
      controlsSizer->Add(hBoxSizer, 0, wxEXPAND);
   }
   worldPanelSizer->AddStretchSpacer();
   worldPanelSizer->Add(controlsSizer, 0, wxTOP | wxRIGHT, 4);
   worldPanel->SetSizer(worldPanelSizer);
   topPanel->SetSizerAndFit(topSizer);

   // ...
   controlsBox->Bind(wxEVT_LEFT_DCLICK, &MainFrame::toggleControlsBox, this);
   // controlsBox->Bind(wxEVT_ENTER_WINDOW, &MainFrame::onEnterControlsBox, this);
   // controlsBox->Bind(wxEVT_LEAVE_WINDOW, &MainFrame::onLeaveControlsBox, this);
}

void MainFrame::toggleControlsBox(wxMouseEvent&) {
   auto children = controlsBox->GetChildren();
   if (children[0]->IsShown()) {
      for (auto child : children) {
         child->Hide();
      }
      // controlsBox->SetTransparent(64);
   } else {
      for (auto child : children) {
         child->Show();
      }
   }
   worldPanel->Layout();
}

/*
void MainFrame::onEnterControlsBox(wxMouseEvent&) {
   controlsBox->HideWithEffect(wxSHOW_EFFECT_EXPAND);
}

void MainFrame::onLeaveControlsBox(wxMouseEvent&) {
   controlsBox->ShowWithEffect(wxSHOW_EFFECT_EXPAND);
}
*/

void MainFrame::onPaint(wxPaintEvent&) {
   wxAutoBufferedPaintDC dC{worldPanel};  // prevents tearing
}

// vim: tw=90 sts=-1 sw=3 et
