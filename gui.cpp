#include <iostream>

#include <giomm/socket.h>
#include <giomm/socketsource.h>
#include <glibmm/main.h>
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/hvpaned.h>
#include <gtkmm/listbox.h>
#include <gtkmm/textview.h>
#include <sigc++/functors/mem_fun.h>

#include "lib/commune.h"

#define APP_ID "au.id.tedp.commune.gtkmm"

namespace {

// XXX: Shouldn't inherit from Commune but it's easier for now.
class CommuneGui: public Gtk::Application, public commune::Commune {
public:
    CommuneGui(int argc, char *argv[]);
    int run();

protected:
    void on_startup() override;
    void receiveMessage(const std::string& sender, const std::string& msg) override;
    bool handleSocketEvent(Glib::IOCondition io_condition);

private:
    Gtk::ApplicationWindow window_;
    Glib::RefPtr<Gtk::TextBuffer> textbufferp_;

    Gtk::VBox vbox_;
    Gtk::HPaned hpane_;
    Gtk::ListBox userlist_;
    Gtk::Entry entry_;
    Gtk::TextView textview_;
};

CommuneGui::CommuneGui(int argc, char *argv[]):
    Gtk::Application(argc, argv, APP_ID),
    commune::Commune::Commune(),
    textbufferp_(Gtk::TextBuffer::create())
{
    window_.set_default_size(640, 480);

    hpane_.set_position(500);

    textview_.set_buffer(textbufferp_);
    textview_.set_editable(false);
    hpane_.pack1(textview_, true, true);
    hpane_.pack2(userlist_, true, true);
    vbox_.pack_start(hpane_, true, true);
    vbox_.pack_end(entry_, false, false);

    window_.add(vbox_);

    window_.show_all();
}

int CommuneGui::run() {
    return Gtk::Application::run(window_);
}

void CommuneGui::on_startup() {
    Gtk::Application::on_startup();

    Gio::signal_socket().connect(sigc::slot<bool, Glib::IOCondition>(sigc::mem_fun(this, &CommuneGui::handleSocketEvent)), Gio::Socket::create_from_fd(event_fd()), Glib::IO_IN);
}

bool CommuneGui::handleSocketEvent(Glib::IOCondition io_condition) {
    std::cerr << "got a socket event" << std::endl;
    commune::Commune::onEvent();
    return true;
}

void CommuneGui::receiveMessage(const std::string& sender, const std::string& msg) {
    std::cerr << "received a message" << std::endl;
    std::string display_message = sender + ": " + msg + '\n';
    textbufferp_->insert(textbufferp_->end(), display_message);
}

} // anonymous namespace

int main(int argc, char *argv[]) {
    return CommuneGui(argc, argv).run();
}
