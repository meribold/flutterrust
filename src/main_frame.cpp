#include "main_frame.hpp"

#include <algorithm>  // std::replace, std::max
#include <array>
#include <cstddef>     // size_t
#include <cstdint>     // int64_t
#include <functional>  // bind
#include <sstream>     // std::stringstream

#include <wx/colour.h>    // wxColour
#include <wx/dcbuffer.h>  // wxAutoBufferedPaintDC
#include <wx/filename.h>  // wxFileName
#include <wx/statline.h>  // wxStaticLine

#include "creature.hpp"
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
      controlsSizer{new wxStaticBoxSizer{wxVERTICAL, worldPanel, u8"Species"}},
      controlsBox{controlsSizer->GetStaticBox()},
      creatureChoice{new wxChoice{controlsBox, wxID_ANY}},
      propertyLabels{{new wxStaticText{controlsBox, wxID_ANY, u8"Strength"},
                      new wxStaticText{controlsBox, wxID_ANY, u8"Speed"},
                      new wxStaticText{controlsBox, wxID_ANY, u8"Lifetime"}}},
      propertyEntries{{new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
                       new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY},
                       new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, wxTE_READONLY}}},
      attributeEntry{new wxTextCtrl{controlsBox, wxID_ANY, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_READONLY | wxTE_MULTILINE | wxTE_NO_VSCROLL}},
      waterContextMenu{new wxMenu{}},
      landContextMenu{new wxMenu{}},
      stepTimer{this},
      world{} {
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
   // Load the graphics used for creatures.  The bitmaps can be accessed using the indices
   // returned by Creature::getTypeIndex().
   {
      creatureBitmaps.reserve(Creature::getTypes().size());
      // Construct a directory path.  The second argument would be the file name and only
      // makes sure the constructor that will consider dataDir to be a directory is
      // chosen.
      wxFileName filePath{dataDir, "", wxPATH_NATIVE};
      filePath.AppendDir(u8"icons");
      for (const auto& creatureType : Creature::getTypes()) {
         // Construct a wxFileName from a unixy path string like 'wasser/algen.tga'.
         wxFileName subPath{creatureType.getBitmapName(), wxPATH_UNIX};
         // Concatenate the paths and create a bitmap.  Both strings implicitly use the
         // platform's native format.
         creatureBitmaps.emplace_back(
             filePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) +
             subPath.GetFullPath());
         // Unrelated to the other code in this loop: set up context menus for placing
         // creatures.
         creatureChoice->Append(creatureType.getName());
      }
      // Load the bitmap used for carcasses.
      filePath.SetFullName(u8"dead.tga");
      carcassBitmap.LoadFile(filePath.GetFullPath());
      // creatureBitmaps.emplace_back(filePath.GetFullPath());
   }
   // Load the graphic used to visualize paths for testing.
   {
      wxFileName filePath{dataDir, u8"path.tga", wxPATH_NATIVE};
      filePath.AppendDir(u8"icons");
      pathBitmap.LoadFile(filePath.GetFullPath());
   }

   wxWindowID myID_VIEW_CREATURES = NewControlId();
   myID_PLAY_PAUSE = NewControlId();
   {
      auto* fileMenu = new wxMenu{};
      fileMenu->Append(wxID_EXIT, "&Quit\tCtrl+Q");
      menuBar->Append(fileMenu, "&File");
      auto* editMenu = new wxMenu{};
      editMenu->Append(wxID_FORWARD, "&Step\tF");
      editMenu->Append(myID_PLAY_PAUSE, "Un&pause\tSpace");
      menuBar->Append(editMenu, "&Edit");
      auto* viewMenu = new wxMenu{};
      viewMenu->AppendCheckItem(myID_VIEW_CREATURES, "&Species info\tS");
      menuBar->Append(viewMenu, "&View");
   }
   SetMenuBar(menuBar);
   {
      const auto creatureTypes = Creature::getTypes();
      for (std::size_t i = 0, size = creatureTypes.size(); i < size; ++i) {
         bool isAquatic = creatureTypes[i].isAquatic();
         if (isAquatic) {
            waterContextMenu->Append(i, creatureTypes[i].getName());
         } else {
            landContextMenu->Append(i, creatureTypes[i].getName());
         }
      }
   }

   // Change the background style to allow using an EVT_PAINT handler.
   // [1]: http://docs.wxwidgets.org/trunk/classwx_window.html#af14f8fd2ed2d30a9bbb5d4f9fd
   worldPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
   controlsBox->SetOwnForegroundColour(wxColour{0xff, 0xff, 0xff});
   controlsBox->Hide();
   topSizer->Add(worldPanel, 1, wxEXPAND);
   controlsSizer->Add(creatureChoice, 0, wxEXPAND);
   controlsSizer->Add(new wxStaticLine{controlsBox, wxID_ANY, wxDefaultPosition,
                                       wxSize{wxDefaultCoord, 10}},
                      0, wxEXPAND);
   assert(propertyEntries.size() == propertyLabels.size());
   for (std::size_t i = 0; i < propertyLabels.size(); ++i) {
      propertyEntries[i]->SetMinClientSize(wxSize{50, -1});
      auto hBoxSizer = new wxBoxSizer{wxHORIZONTAL};
      hBoxSizer->Add(propertyLabels[i], 0, wxALIGN_CENTER_VERTICAL);
      hBoxSizer->AddStretchSpacer(1);
      hBoxSizer->Add(propertyEntries[i], 0, wxEXPAND);
      controlsSizer->Add(hBoxSizer, 0, wxEXPAND | wxALL, 2);
   }
   controlsSizer->Add(attributeEntry, 0, wxEXPAND | wxALL, 2);
   // Make the text control just big enough to contain the text "Wasserbewohner" on one
   // line without horizontal scrolling.
   // [1]: http://docs.wxwidgets.org/trunk/classwx_control.html
   attributeEntry->SetInitialSize(attributeEntry->GetSizeFromTextSize(
       attributeEntry->GetTextExtent("Wasserbewohner")));
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
   Bind(wxEVT_COMMAND_MENU_SELECTED,
        [this](wxCommandEvent&) {
           if (controlsBox->IsShown()) {
              controlsBox->Hide();
           } else {
              controlsBox->Show();
              worldPanelSizer->Layout();
           }
        },
        myID_VIEW_CREATURES);
   Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::onStep, this, wxID_FORWARD);
   Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::onPlayPause, this, myID_PLAY_PAUSE);

   // ...
   controlsBox->Bind(wxEVT_LEFT_DCLICK, &MainFrame::toggleControlsBox, this);

   creatureChoice->Bind(wxEVT_CHOICE, &MainFrame::onCreatureChoice, this);

   worldPanel->Bind(wxEVT_LEFT_DOWN, &MainFrame::onLeftDown, this);
   worldPanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &MainFrame::onCaptureLost, this);

   worldPanel->Bind(wxEVT_CONTEXT_MENU, &MainFrame::onContextMenuRequested, this,
                    worldPanel->GetId());
   worldPanel->Bind(wxEVT_MENU, &MainFrame::onMenuItemSelected, this);
   // worldPanel->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::onMenuItemSelected, this);

   Bind(wxEVT_TIMER, &MainFrame::onTimer, this);

   creatureChoice->SetSelection(0);
   updateAttributes(0);
}

void MainFrame::updateAttributes(std::size_t creatureIndex) {
   const auto& type = Creature::getTypes()[creatureIndex];
   for (const auto& textCtrl : propertyEntries) {
      textCtrl->Clear();
   }
   *propertyEntries[0] << type.getStrength();
   *propertyEntries[1] << type.getSpeed();
   *propertyEntries[2] << type.getMaxLifetime();
   std::string attributes = type.getAttributeString();
   std::replace(attributes.begin(), attributes.end(), ' ', '\n');
   attributeEntry->Clear();
   *attributeEntry << attributes;

   // Make the attributeEntry text control just heigh enough to make all its text visible
   // without scrolling.
   attributeEntry->SetMinClientSize(wxSize{-1, 0});
   attributeEntry->SetClientSize(-1, 0);
   attributeEntry->SetMinClientSize(attributeEntry->GetBestVirtualSize());
   worldPanelSizer->Layout();
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
         if (world.carcasses.find({worldX, worldY}) != world.carcasses.end()) {
            dC.DrawBitmap(carcassBitmap, drawOffsetX, drawOffsetY);
         }
         // Draw any creatures that are at {worldX, worldY}.
         auto range = world.creatures.equal_range({worldX, worldY});
         for (auto it = range.first; it != range.second; ++it) {
            // const auto& pos = it->first;
            const auto& creature = it->second;
            dC.DrawBitmap(creatureBitmaps[creature.getTypeIndex()], drawOffsetX,
                          drawOffsetY);
         }
         ++worldX;
         drawOffsetX += tileSize;
      }
      ++worldY;
      drawOffsetY += tileSize;
   }

   // for (const auto& pos : world.carcasses) {
   //    dC.DrawBitmap(carcassBitmap, worldToPanelX(pos[0]), worldToPanelY(pos[1]));
   // }

   for (const auto& pos : testPath) {
      dC.DrawBitmap(pathBitmap, worldToPanelX(pos[0]), worldToPanelY(pos[1]));
   }
}

void MainFrame::onCreatureChoice(wxCommandEvent& event) {
   auto index = event.GetInt();
   updateAttributes(index);
}

void MainFrame::onPlayPause(wxCommandEvent&) {
   constexpr int interval = 1000;  // In milliseconds.
   if (!stepTimer.IsRunning()) {
      menuBar->SetLabel(myID_PLAY_PAUSE, "&Pause\tSpace");
      step();
      stepTimer.Start(interval);
   } else {
      menuBar->SetLabel(myID_PLAY_PAUSE, "Un&pause\tSpace");
      stepTimer.Stop();
   }
}

// FIXME: ensure that some time passes between finishing computing a step and starting to
// compute the next one.
void MainFrame::onTimer(wxTimerEvent&) { step(); }

void MainFrame::onStep(wxCommandEvent&) { step(); }

void MainFrame::onLeftDown(wxMouseEvent& event) {
   assert(!HasCapture());
   CaptureMouse();

   oldMousePos = event.GetPosition();

   if (event.ShiftDown()) {
      leftDownEvent = event;
      Bind(wxEVT_MOTION, &MainFrame::onShiftMotion, this);
      Bind(wxEVT_LEFT_UP, &MainFrame::onShiftLeftUp, this);
   } else {
      Bind(wxEVT_MOTION, &MainFrame::onMotion, this);
      Bind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);
   }

   // It is generally recommended to Skip() all non-command events [1].  This allows other
   // event handlers (including default ones) to react to the event.  Without it,
   // processing of the event stops.
   // [1]: http://docs.wxwidgets.org/trunk/classwx_event.html#a98eb20b76106f9a933c2eb3ee11
   event.Skip();
}

// Process a wxEVT_MOUSE_CAPTURE_LOST; handling this event is mandatory for an application
// that captures the mouse.  FIXME: this function doesn't do the correct thing when shift
// was pressed when we captured the mouse.
void MainFrame::onCaptureLost(wxMouseCaptureLostEvent& event) {
#ifdef DEBUG
   assert(!HasCapture());
   bool didUnbind = Unbind(wxEVT_MOTION, &MainFrame::onMotion, this) &&
                    Unbind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);
   assert(didUnbind);
#else
   Unbind(wxEVT_MOTION, &MainFrame::onMotion, this);
   Unbind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);
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

void MainFrame::onShiftMotion(wxMouseEvent& event) {
   assert(HasCapture());
   // FIXME: reduce redundant computations.
   if (panelToWorld(oldMousePos) != panelToWorld(event.GetPosition())) {
      World::Pos start{panelToWorldX(leftDownEvent.GetX()),
                       panelToWorldY(leftDownEvent.GetY())};
      World::Pos dest{panelToWorldX(event.GetX()), panelToWorldY(event.GetY())};
      refreshPath();
      testPath = world.getPath(std::move(start), std::move(dest));
      refreshPath();
      oldMousePos.x = event.GetX();
      oldMousePos.y = event.GetY();
   }
   event.Skip();
}

// Process a wxEVT_LEFT_UP.
void MainFrame::onLeftUp(wxMouseEvent&) {
#ifdef DEBUG
   assert(HasCapture());
   assert(Unbind(wxEVT_MOTION, &MainFrame::onMotion, this) &&
          Unbind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this));
#else
   Unbind(wxEVT_MOTION, &MainFrame::onMotion, this);
   Unbind(wxEVT_LEFT_UP, &MainFrame::onLeftUp, this);
#endif
   ReleaseMouse();
   // Somehow skipping this event crashes the program  :/
   // event.Skip();
}

void MainFrame::onShiftLeftUp(wxMouseEvent&) {
#ifdef DEBUG
   assert(HasCapture());
   assert(Unbind(wxEVT_MOTION, &MainFrame::onShiftMotion, this) &&
          Unbind(wxEVT_LEFT_UP, &MainFrame::onShiftLeftUp, this));
#else
   Unbind(wxEVT_MOTION, &MainFrame::onShiftMotion, this);
   Unbind(wxEVT_LEFT_UP, &MainFrame::onShiftLeftUp, this);
#endif
   refreshPath();
   testPath.clear();
   ReleaseMouse();
}

std::int64_t MainFrame::panelToWorldX(int panelX) const {
   std::int64_t x = panelX + scrollOffX;
   if (x >= 0) {
      x /= tileSize;
   } else {
      x = (x + 1) / tileSize - 1;
   }
   return x;
}

std::int64_t MainFrame::panelToWorldY(int panelY) const {
   std::int64_t y = panelY + scrollOffY;
   if (y >= 0) {
      y /= tileSize;
   } else {
      y = (y + 1) / tileSize - 1;
   }
   return y;
}

wxPoint MainFrame::panelToWorld(wxPoint point) const {
   point.x = panelToWorldX(point.x);
   point.y = panelToWorldY(point.y);
   return point;
}

int MainFrame::worldToPanelX(std::int64_t worldX) const {
   int panelX = worldX * tileSize - scrollOffX;
   assert(panelToWorldX(panelX) == worldX);
   return panelX;
}

int MainFrame::worldToPanelY(std::int64_t worldY) const {
   int panelY = worldY * tileSize - scrollOffY;
   assert(panelToWorldY(panelY) == worldY);
   return panelY;
}

// Return a wxRect specifying the area of the tile the given point falls into.  Can be
// used with wxWindow::RefreshRect(const wxRect&).
wxRect MainFrame::getTileArea(int x, int y) const {
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

void MainFrame::step() {
   world.step();
   worldPanel->Refresh(false);  // FIXME: only refresh areas that changed.
}

// Invalidate the area of all tiles corresponding to positions in testPath.  The
// invalidated area will be repainted during the next event loop iteration.
void MainFrame::refreshPath() {
   for (const auto& pos : testPath) {
      wxRect rect{worldToPanelX(pos[0]), worldToPanelY(pos[1]), tileSize, tileSize};
      worldPanel->RefreshRect(rect, false);
   }
}

// Process a wxEVT_CONTEXT_MENU inside the worldPanel.
void MainFrame::onContextMenuRequested(wxContextMenuEvent& event) {
   contextMenuPos = worldPanel->ScreenToClient(event.GetPosition());
   std::int64_t worldX = panelToWorldX(contextMenuPos.x);
   std::int64_t worldY = panelToWorldY(contextMenuPos.y);
   const TileType tileType = world.getTileType(worldX, worldY);
   if (tileType == TileType::deepWater || tileType == TileType::water) {
      worldPanel->PopupMenu(waterContextMenu);
   } else {
      worldPanel->PopupMenu(landContextMenu);
   }
   event.Skip();
}

// Process a wxEVT_MENU when an item from the worldPanel's context menu is selected.
void MainFrame::onMenuItemSelected(wxCommandEvent& event) {
   std::int64_t worldX = panelToWorldX(contextMenuPos.x);
   std::int64_t worldY = panelToWorldY(contextMenuPos.y);
   world.spawnCreature(event.GetId(), worldX, worldY);
   // Invalidate the area of the tile we added a creature to.  It will be repainted during
   // the next event loop iteration.
   worldPanel->RefreshRect(getTileArea(contextMenuPos.x, contextMenuPos.y), false);
   event.Skip();
}

// vim: tw=90 sts=-1 sw=3 et
