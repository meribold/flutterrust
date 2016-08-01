#include "main_frame.hpp"

#include <array>
#include <cstddef>     // size_t
#include <cstdint>     // int64_t
#include <functional>  // bind

#include <wx/colour.h>    // wxColour
#include <wx/dcbuffer.h>  // wxAutoBufferedPaintDC
#include <wx/filename.h>  // wxFileName
#include <wx/statline.h>  // wxStaticLine

#include "tuple_helpers.hpp"  // toUT

#include <cassert>  // assert
#ifdef DEBUG
#include <iostream>
#endif

MainFrame::MainFrame(const std::string& dataDir, const wxPoint& pos, const wxSize& size)
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
      contextMenu{new wxMenu{u8"Add creature"}},
      world{dataDir + static_cast<char>(wxFileName::GetPathSeparator()) +
            u8"CreatureTable.txt"} {
   {
      const std::array<std::string, 6> fileNames{
          u8"deep_sea", u8"shallow_water", u8"sand", u8"earth", u8"rocks", u8"snow"};
      wxFileName filePath{dataDir, "", u8"tga", wxPATH_NATIVE};
      filePath.AppendDir(u8"icons");
      filePath.AppendDir(u8"terrain");
      for (std::size_t i = 0; i < fileNames.size(); ++i) {
         filePath.SetName(fileNames[i]);
         terrainBitmaps[i].LoadFile(filePath.GetFullPath(), wxBITMAP_TYPE_TGA);
      }
   }
   // ...
   {
      creatureBitmaps.reserve(world.creatureTypes.size());
      // Construct a directory path.  The second argument would be the file name and only
      // makes sure the constructor that will consider dataDir to be a directory is
      // chosen.
      wxFileName filePath{dataDir, "", wxPATH_NATIVE};
      filePath.AppendDir(u8"icons");
      for (const auto& creatureType : world.creatureTypes) {
         // Construct a wxFileName from a unixy path string like 'wasser/algen.tga'.
         wxFileName subPath{std::get<cTFields::bitmap>(creatureType), wxPATH_UNIX};
         // Concatenate the paths and create a bitmap.  Both strings implicitly use the
         // platform's native format.
         creatureBitmaps.emplace_back(
             filePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) +
             subPath.GetFullPath());
         // Unrelated to the other code in this loop: set up the context menu for placing
         // creatures.
         creatureChoice->Append(std::get<cTFields::name>(creatureType));
      }
   }
   {
      auto* fileMenu = new wxMenu{};
      fileMenu->Append(wxID_EXIT, "&Quit\tCtrl+Q");
      menuBar->Append(fileMenu, "&File");
      SetMenuBar(menuBar);
   }
   for (std::size_t i = 0; i < world.creatureTypes.size(); ++i) {
      const auto& type = world.creatureTypes[i];
      contextMenu->Append(i, std::get<cTFields::name>(type));
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

   worldPanel->Bind(wxEVT_PAINT, &MainFrame::onPaint, this, worldPanel->GetId());
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

   worldPanel->Bind(wxEVT_CONTEXT_MENU, &MainFrame::onContextMenuRequested, this,
                    worldPanel->GetId());
   worldPanel->Bind(wxEVT_MENU, &MainFrame::onMenuItemSelected, this);
   // worldPanel->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::onMenuItemSelected, this);

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
   *propertyEntries[3] << static_cast<std::string>(std::get<cTFields::attributes>(type));
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

   // TODO: only repaint the invalidated areas.
   /*
   // Get the regions specifying which parts of the window should be repainted.
   wxRegionIterator regIt(worldPanel->GetUpdateRegion());
   while (regIt) {
      std::cout << regIt.GetX() << ", " << regIt.GetY() << ", " << regIt.GetHeight()
                << ", " << regIt.GetWidth() << '\n';
      // ...
      ++regIt;
   }
   */

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
         // Draw any creatures that are at {worldX, worldY}.
         auto range = world.creatures.equal_range({worldX, worldY});
         for (auto it = range.first; it != range.second; ++it) {
            // const auto& pos = it->first;
            const auto& creature = it->second;
            // std::cout << std::get<cTFields::bitmap>(creature.type) << '\n';
            dC.DrawBitmap(creatureBitmaps[creature.getTypeIndex()], drawOffsetX,
                          drawOffsetY);
         }
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
#ifdef DEBUG
   std::cerr << "Place\n";
#endif
}

void MainFrame::onPlayPause(wxCommandEvent&) {
#ifdef DEBUG
   if (true)  // TODO
      std::cerr << "Play\n";
   else
      std::cerr << "Pause\n";
#endif
}

void MainFrame::onStep(wxCommandEvent&) {
#ifdef DEBUG
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
#ifdef DEBUG
   assert(!HasCapture());
   bool didUnbind = Unbind(wxEVT_MOTION, &MainFrame::onMotion, this) &&
                    Unbind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);
   assert(didUnbind);
#endif
   event.Skip();
}

// Process a wxEVT_MOTION.
void MainFrame::onMotion(wxMouseEvent& event) {
   assert(HasCapture());
   scrollOffX -= event.GetX() - oldMousePos.x;
   scrollOffY -= event.GetY() - oldMousePos.y;
   oldMousePos.x = event.GetX();
   oldMousePos.y = event.GetY();
   worldPanel->Refresh(false);
   event.Skip();
}

// Process a wxEVT_LEFT_UP.
void MainFrame::onLeftUp(wxMouseEvent&) {
#ifdef DEBUG
   assert(HasCapture());
   bool didUnbind = Unbind(wxEVT_MOTION, &MainFrame::onMotion, this) &&
                    Unbind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);
   assert(didUnbind);
#endif
   ReleaseMouse();
   // Somehow skipping this event crashes the program  :/
   // event.Skip();
}

std::int64_t MainFrame::panelToWorldX(int panelX) {
   std::int64_t x = panelX + scrollOffX;
   if (x >= 0) {
      x /= tileSize;
   } else {
      x = (x + 1) / tileSize - 1;
   }
   return x;
}

std::int64_t MainFrame::panelToWorldY(int panelY) {
   std::int64_t y = panelY + scrollOffY;
   if (y >= 0) {
      y /= tileSize;
   } else {
      y = (y + 1) / tileSize - 1;
   }
   return y;
}

// Return a wxRect specifying the area of the tile the given point falls into.  Can be
// used with wxWindow::RefreshRect(const wxRect&).
wxRect MainFrame::getTileArea(int x, int y) {
   assert(x >= 0 && y >= 0);
   // Get the given coordinates (x, y) relative to the absolute origin.
   wxRect rect{static_cast<int>(scrollOffX + x), static_cast<int>(scrollOffY + y),
               tileSize, tileSize};
   // Get the top-left of the tile that contains the given coordinates relative to the
   // absolute origin.
   if (rect.x < 0) rect.x -= (tileSize - 1);
   if (rect.y < 0) rect.y -= (tileSize - 1);
   rect.x = rect.x / tileSize * tileSize;
   rect.y = rect.y / tileSize * tileSize;
   // Convert it to be relative to the worldPanel origin.
   rect.x -= scrollOffX;
   rect.y -= scrollOffY;
   /*
   if (rect.x < 0) {
      rect.width += rect.x;
      rect.x = 0;
   }
   if (top < 0) {
      rect.height += top;
      rect.y = 0;
   }
   */
   // Assert that (x, y) really falls into the wxRect and that it exactly lines up with
   // the area of a tile.
   assert(rect.Contains(x, y));
   assert((scrollOffX + rect.x) % tileSize == 0);
   assert((scrollOffY + rect.y) % tileSize == 0);
   return rect;
}

// Process a wxEVT_CONTEXT_MENU inside the worldPanel.
void MainFrame::onContextMenuRequested(wxContextMenuEvent& event) {
   contextMenuPos = worldPanel->ScreenToClient(event.GetPosition());
   worldPanel->PopupMenu(contextMenu);
   event.Skip();
}

// Process a wxEVT_MENU when an item from the worldPanel's context menu is selected.
void MainFrame::onMenuItemSelected(wxCommandEvent& event) {
   std::int64_t worldX = panelToWorldX(contextMenuPos.x);
   std::int64_t worldY = panelToWorldY(contextMenuPos.y);
   if (world.addCreature(event.GetId(), worldX, worldY)) {
      // Invalidate the area of the tile we added a creature to.  It will be repainted
      // during the next event loop iteration.
      worldPanel->RefreshRect(getTileArea(contextMenuPos.x, contextMenuPos.y), false);
   } else {
      // TODO: annoy the user with an annoying message window.  Alternatively, overlay a
      // non-intrusive message that disappears after a few moments.  Or only show eligible
      // creatures in the context menu.
   }
   event.Skip();
}

// vim: tw=90 sts=-1 sw=3 et
