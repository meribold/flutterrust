#include "main_frame.hpp"

#include <cassert>     // assert
#include <functional>  // bind
#include <iostream>    // TODO: remove

#include <wx/colour.h>    // wxColour
#include <wx/dcbuffer.h>  // wxAutoBufferedPaintDC
#include <wx/statline.h>  // wxStaticLine

MainFrame::MainFrame(const wxPoint& pos, const wxSize& size)
    : wxFrame{nullptr, wxID_ANY, u8"flutterrust", pos, size},
      menuBar{new wxMenuBar{}},
      topPanel{new wxPanel{this}},
      topSizer{new wxBoxSizer{wxHORIZONTAL}},
      worldPanel{new wxPanel{topPanel}},
      worldPanelSizer{new wxBoxSizer{wxHORIZONTAL}},
      controlsSizer{new wxStaticBoxSizer{wxVERTICAL, worldPanel, u8"Controls"}},
      controlsBox{controlsSizer->GetStaticBox()},
      creatureChoice{new wxChoice{controlsBox, wxID_ANY}},
      propertyLabels{{new wxStaticText{controlsBox, wxID_ANY, u8"Strength"},
                      new wxStaticText{controlsBox, wxID_ANY, u8"Speed"},
                      new wxStaticText{controlsBox, wxID_ANY, u8"Lifetime"},
                      new wxStaticText{controlsBox, wxID_ANY, u8"Attributes"}}},
      propertyEntries{{new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
                       new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
                       new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
                       new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY}}},
      placeCreatureButton{new wxButton{controlsBox, wxID_ANY, u8"Place"}},
      playPauseButton{new wxButton{controlsBox, wxID_ANY, u8"Unpause"}},
      stepButton{new wxButton{controlsBox, wxID_ANY, u8"Step"}},
      world{u8"CreatureTable.txt"} {
   for (const auto& type : world.creatureTypes) {
      creatureChoice->Append(std::get<cTFields::name>(type));
   }
   {
      auto* fileMenu = new wxMenu{};
      fileMenu->Append(wxID_EXIT, "&Quit\tCtrl+Q");
      menuBar->Append(fileMenu, "&File");
      SetMenuBar(menuBar);
   }
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
   {
      auto* sizer = new wxBoxSizer{wxVERTICAL};
      sizer->Add(topPanel, 1, wxEXPAND);
      SetSizerAndFit(sizer);
      SetSize(wxDefaultCoord, wxDefaultCoord, 640, 480);
   }

   Bind(wxEVT_COMMAND_MENU_SELECTED, std::bind(&MainFrame::Close, this, false),
        wxID_EXIT);

   // ...
   controlsBox->Bind(wxEVT_LEFT_DCLICK, &MainFrame::toggleControlsBox, this);
   // controlsBox->Bind(wxEVT_ENTER_WINDOW, &MainFrame::onEnterControlsBox, this);
   // controlsBox->Bind(wxEVT_LEAVE_WINDOW, &MainFrame::onLeaveControlsBox, this);

   creatureChoice->Bind(wxEVT_CHOICE, &MainFrame::onCreatureChoice, this);
   placeCreatureButton->Bind(wxEVT_BUTTON, &MainFrame::onPlace, this);
   playPauseButton->Bind(wxEVT_BUTTON, &MainFrame::onPlayPause, this);
   stepButton->Bind(wxEVT_BUTTON, &MainFrame::onStep, this);

   creatureChoice->SetSelection(0);
   updateAttributes(0);
}

void MainFrame::updateAttributes(std::size_t creatureIndex) {
   const auto& type = world.creatureTypes[creatureIndex];
   for (const auto& textCtrl : propertyEntries) {
      textCtrl->Clear();
   }
   *propertyEntries[0] << std::get<cTFields::strength>(type);
   *propertyEntries[1] << std::get<cTFields::speed>(type);
   *propertyEntries[2] << std::get<cTFields::lifetime>(type);
   *propertyEntries[3] << std::get<cTFields::attributes>(type);
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

void MainFrame::onCreatureChoice(wxCommandEvent& event) {
   auto index = event.GetInt();
   updateAttributes(index);
}

void MainFrame::onPlace(wxCommandEvent&) { std::cerr << "Place\n"; }

void MainFrame::onPlayPause(wxCommandEvent&) {
   if (true)  // TODO
      std::cerr << "Play\n";
   else
      std::cerr << "Pause\n";
}

void MainFrame::onStep(wxCommandEvent&) { std::cerr << "Step\n"; }

// vim: tw=90 sts=-1 sw=3 et
