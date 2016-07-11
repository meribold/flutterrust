#include "main_frame.hpp"

#include <cstddef>     // size_t
#include <cstdint>     // int64_t
#include <functional>  // bind

#include <cassert>  // assert
#ifndef NDEBUG
#include <iostream>
#endif

#include <wx/colour.h>    // wxColour
#include <wx/dcbuffer.h>  // wxAutoBufferedPaintDC
#include <wx/statline.h>  // wxStaticLine

#include "tuple_helpers.hpp"  // toUT

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
   // for (auto i = toUT(TileType::deepWater); i <= toUT(TileType::snow); ++i) {
   //    terrainBitmaps[i];
   // }
   terrainBitmaps[0].LoadFile(u8"icons/terrain/deep_sea.tga", wxBITMAP_TYPE_TGA);
   terrainBitmaps[1].LoadFile(u8"icons/terrain/shallow_water.tga", wxBITMAP_TYPE_TGA);
   terrainBitmaps[2].LoadFile(u8"icons/terrain/sand.tga", wxBITMAP_TYPE_TGA);
   terrainBitmaps[3].LoadFile(u8"icons/terrain/earth.tga", wxBITMAP_TYPE_TGA);
   terrainBitmaps[4].LoadFile(u8"icons/terrain/rocks.tga", wxBITMAP_TYPE_TGA);
   terrainBitmaps[5].LoadFile(u8"icons/terrain/snow.tga", wxBITMAP_TYPE_TGA);

   for (const auto& type : world.creatureTypes) {
      creatureChoice->Append(std::get<cTFields::name>(type));
   }
   {
      auto* fileMenu = new wxMenu{};
      fileMenu->Append(wxID_EXIT, "&Quit\tCtrl+Q");
      menuBar->Append(fileMenu, "&File");
      SetMenuBar(menuBar);
   }
   // Change the background style to allow using an EVT_PAINT handler.
   // [1]: http://docs.wxwidgets.org/trunk/classwx_window.html#af14f8fd2ed2d30a9bbb5d4f9fd
   worldPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
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

   worldPanel->Bind(wxEVT_PAINT, &MainFrame::onPaint, this);
   Bind(wxEVT_COMMAND_MENU_SELECTED, std::bind(&MainFrame::Close, this, false),
        wxID_EXIT);

   // ...
   controlsBox->Bind(wxEVT_LEFT_DCLICK, &MainFrame::toggleControlsBox, this);

   creatureChoice->Bind(wxEVT_CHOICE, &MainFrame::onCreatureChoice, this);
   placeCreatureButton->Bind(wxEVT_BUTTON, &MainFrame::onPlace, this);
   playPauseButton->Bind(wxEVT_BUTTON, &MainFrame::onPlayPause, this);
   stepButton->Bind(wxEVT_BUTTON, &MainFrame::onStep, this);

   worldPanel->Bind(wxEVT_LEFT_DOWN, &MainFrame::onLeftDown, this);
   worldPanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &MainFrame::onCaptureLost, this);

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

// Process a wxEVT_PAINT event.
void MainFrame::onPaint(wxPaintEvent&) {
   wxAutoBufferedPaintDC dC{worldPanel};  // Prevents tearing.
   int panelWidth, panelHeight;
   dC.GetSize(&panelWidth, &panelHeight);

   std::int64_t initialWorldX;
   if (scrollOffX >= 0) {
      initialWorldX = scrollOffX / tileSize;
   } else {
      initialWorldX = (scrollOffX + 1) / tileSize - 1;
   }
   std::int64_t worldY;
   if (scrollOffY >= 0) {
      worldY = scrollOffY / tileSize;
   } else {
      worldY = (scrollOffY + 1) / tileSize - 1;
   }

   // Make sure the area we are about to access is available.  This will update the
   // terrain cache if necessary.  Querying terrain outside of the cached area results in
   // invalid array accesses and probably crashes the program (no bounds-checking is
   // performed).  Creatures outside of the cached area are not simulated.
   world.assertCached(initialWorldX, worldY, (panelWidth + tileSize - 1) / tileSize,
                      (panelHeight + tileSize - 1) / tileSize);

   // Example: assume scrollOffX is (-33).  That means we scrolled 33 pixels to the left
   // (by moving the mouse to the right).  The value of initialWorldX is (-2), but we can
   // only show one pixel of the leftmost column of tiles: start drawing at (-31).
   std::int64_t initialDrawOffsetX = (-scrollOffX) % tileSize;
   std::int64_t drawOffsetY = (-scrollOffY) % tileSize;
   if (initialDrawOffsetX > 0) initialDrawOffsetX -= tileSize;
   if (drawOffsetY > 0) drawOffsetY -= tileSize;

   while (drawOffsetY < panelHeight) {
      auto worldX = initialWorldX;
      auto drawOffsetX = initialDrawOffsetX;
      while (drawOffsetX < panelWidth) {
         auto bitmapIndex = toUT(world.getTileType(worldX, worldY));
         assert(bitmapIndex < terrainBitmaps.size());
         dC.DrawBitmap(terrainBitmaps[bitmapIndex], drawOffsetX, drawOffsetY);
         ++worldX;
         drawOffsetX += tileSize;
      }
      ++worldY;
      drawOffsetY += tileSize;
   }
}

void MainFrame::onCreatureChoice(wxCommandEvent& event) {
   auto index = event.GetInt();
   updateAttributes(index);
}

void MainFrame::onPlace(wxCommandEvent&) {
#ifndef NDEBUG
   std::cerr << "Place\n";
#endif
}

void MainFrame::onPlayPause(wxCommandEvent&) {
#ifndef NDEBUG
   if (true)  // TODO
      std::cerr << "Play\n";
   else
      std::cerr << "Pause\n";
#endif
}

void MainFrame::onStep(wxCommandEvent&) {
#ifndef NDEBUG
   std::cerr << "Step\n";
#endif
   world.step();
}

void MainFrame::onLeftDown(wxMouseEvent& event) {
   assert(!HasCapture());
   CaptureMouse();

   oldMousePos = event.GetPosition();

   Bind(wxEVT_MOTION, &MainFrame::onMotion, this);
   Bind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);

   // It is generally recommended to Skip() all non-command events [1].  This allows other
   // event handlers (including default ones) to react to the event.  Without it,
   // processing of the event stops.
   // [1]: http://docs.wxwidgets.org/trunk/classwx_event.html#a98eb20b76106f9a933c2eb3ee11
   event.Skip();
}

// Process a wxEVT_MOUSE_CAPTURE_LOST; handling this event is mandatory for an application
// that captures the mouse.
void MainFrame::onCaptureLost(wxMouseCaptureLostEvent& event) {
   assert(!HasCapture());
   bool didUnbind = Unbind(wxEVT_MOTION, &MainFrame::onMotion, this) &&
                    Unbind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);
   assert(didUnbind);
   event.Skip();
}

// Process a wxEVT_MOTION.
void MainFrame::onMotion(wxMouseEvent& event) {
   assert(HasCapture());
   scrollOffX -= event.GetX() - oldMousePos.x;
   scrollOffY -= event.GetY() - oldMousePos.y;
   oldMousePos.x = event.GetX();
   oldMousePos.y = event.GetY();
   Refresh(false);
   event.Skip();
}

// Process a wxEVT_LEFT_UP.
void MainFrame::onLeftUp(wxMouseEvent&) {
   assert(HasCapture());
   bool didUnbind = Unbind(wxEVT_MOTION, &MainFrame::onMotion, this) &&
                    Unbind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);
   assert(didUnbind);
   ReleaseMouse();
   // Somehow skipping this event crashes the program  :/
   // event.Skip();
}

// vim: tw=90 sts=-1 sw=3 et
