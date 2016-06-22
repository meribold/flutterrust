#include "main_frame.hpp"

#include <cassert>  // assert

#include <wx/colour.h>    // wxColour
#include <wx/dcbuffer.h>  // wxAutoBufferedPaintDC
#include <wx/statbox.h>   // wxStaticBox

MainFrame::MainFrame(const wxPoint& pos, const wxSize& size)
    : wxFrame{nullptr, wxID_ANY, u8"flutterrust", pos, size},
      topPanel{new wxPanel{this}},
      topSizer{new wxBoxSizer{wxHORIZONTAL}},
      worldPanel{new wxPanel{topPanel}},
      worldPanelSizer{new wxBoxSizer{wxHORIZONTAL}},
      controlsSizer{new wxStaticBoxSizer{wxVERTICAL, worldPanel, u8"Controls"}},
      // controlPanel{new wxPanel{topPanel}},
      creatureChoice{new wxChoice{controlsSizer->GetStaticBox(), wxID_ANY}},
      propertyLabels{
          {new wxStaticText{controlsSizer->GetStaticBox(), wxID_ANY, u8"Strength"},
           new wxStaticText{controlsSizer->GetStaticBox(), wxID_ANY, u8"Speed"},
           new wxStaticText{controlsSizer->GetStaticBox(), wxID_ANY, u8"Life"},
           new wxStaticText{controlsSizer->GetStaticBox(), wxID_ANY, u8"Properties"}}},
      propertyEntries{
          {new wxTextCtrl{controlsSizer->GetStaticBox(), wxID_ANY, wxEmptyString,
                          wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
           new wxTextCtrl{controlsSizer->GetStaticBox(), wxID_ANY, wxEmptyString,
                          wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
           new wxTextCtrl{controlsSizer->GetStaticBox(), wxID_ANY, wxEmptyString,
                          wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
           new wxTextCtrl{controlsSizer->GetStaticBox(), wxID_ANY, wxEmptyString,
                          wxDefaultPosition, wxDefaultSize, wxTE_READONLY}}} {
   worldPanel->SetOwnBackgroundColour(wxColour{0x00, 0x00, 0x80});
   topSizer->Add(worldPanel, 1, wxEXPAND);
   controlsSizer->Add(creatureChoice, 0, wxEXPAND);
   assert(propertyEntries.size() == propertyEntries.size());
   for (std::size_t i = 0; i < propertyLabels.size(); ++i) {
      controlsSizer->Add(propertyLabels[i], 0, wxALIGN_CENTER_HORIZONTAL);
      controlsSizer->Add(propertyEntries[i], 0, wxEXPAND);
   }
   worldPanelSizer->AddStretchSpacer();
   worldPanelSizer->Add(controlsSizer, 0, wxTOP | wxRIGHT, 4);
   worldPanel->SetSizer(worldPanelSizer);
   topPanel->SetSizerAndFit(topSizer);
}

void MainFrame::onPaint(wxPaintEvent&) {
   wxAutoBufferedPaintDC dC{worldPanel};  // prevents tearing
}

// vim: tw=90 sts=-1 sw=3 et
