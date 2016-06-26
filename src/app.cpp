#include <string>
#include <typeinfo>  // typeid

#include <wx/app.h>
#include <wx/msgdlg.h>  // wxMessageDialog

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

   try {
      mainFrame = new MainFrame{};
   } catch (const std::exception& e) {
      wxMessageDialog dialog{nullptr, u8"Exception caught.  Terminating.\n",
                             u8"Fatal error", wxICON_ERROR | wxOK};
      dialog.SetExtendedMessage(std::string{typeid(e).name()} + ": \"" + e.what() + '"');
      dialog.ShowModal();
      return false;  // Exit the application immediately.
   } catch (...) {
      wxMessageDialog(nullptr, u8"Unknown exception caught.  Terminating.",
                      u8"Fatal error", wxICON_ERROR | wxOK)
          .ShowModal();
      return false;
   }

   mainFrame->Show(true);
   SetTopWindow(mainFrame);

   return true;  // Continue processing.
}

// vim: tw=90 sts=3 sw=3 et
