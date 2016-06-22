#include <wx/app.h>

#include "main_frame.hpp"

class App : public wxApp {
  public:
   virtual bool OnInit() override;

  private:
   MainFrame* mainFrame;
};

wxIMPLEMENT_APP(App);

bool App::OnInit() {
   SetAppName(u8"flutterrust");
   SetAppDisplayName(u8"flutterrust");

   mainFrame = new MainFrame{};

   mainFrame->Show(true);
   SetTopWindow(mainFrame);

   return true;
}

// vim: tw=90 sts=3 sw=3 et
