#include <wx/app.h>
#include <wx/frame.h>

class App : public wxApp {
  public:
   virtual bool OnInit() override;

  private:
   wxFrame* mainFrame;
};

wxIMPLEMENT_APP(App);

bool App::OnInit() {
   SetAppName(u8"flutterrust");
   SetAppDisplayName(u8"flutterrust");

   mainFrame = new wxFrame{nullptr, wxID_ANY, u8"flutterrust", wxDefaultPosition,
                           wxSize(640, 720)},

   mainFrame->Show(true);
   SetTopWindow(mainFrame);

   return true;
}

// vim: tw=90 sts=3 sw=3 et
