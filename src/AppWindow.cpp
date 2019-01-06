//*****************************************************************
//
//  Copyright (C) 2018 Juan R. Gonzalez
//  Created on November 3, 2018, 2:45 PM
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
// 
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//*****************************************************************
#include <cairo/cairo.h>
#include <glibmm-2.4/glibmm/timer.h>
#include "AppWindow.h"
#include "../config.h"
#include "DockWindow.h"
#include "WindowControl.h"
#include "Animations.h"
#include "Utilities.h"

#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>


bool AppWindow::m_isfullscreenSet;
bool AppWindow::m_isfullscreen;

Screen AppWindow::m_screen;

static const gchar* type_name(WnckWindowType type)
{
    switch (type)
    {
        case WNCK_WINDOW_NORMAL: return "normal";
        case WNCK_WINDOW_DESKTOP: return "desktop";
        case WNCK_WINDOW_DOCK: return "dock";
        case WNCK_WINDOW_DIALOG: return "dialog";
        case WNCK_WINDOW_TOOLBAR: return "toolbar";
        case WNCK_WINDOW_MENU: return "menu";
        case WNCK_WINDOW_UTILITY: return "utility";
        case WNCK_WINDOW_SPLASHSCREEN: return "splashscreen";
        default: return "UNKNOWN";
    }
}

AppWindow::AppWindow()
{
    GdkScreen *screen;
    GdkVisual *visual;

    gtk_widget_set_app_paintable(GTK_WIDGET(gobj()), TRUE);
    screen = gdk_screen_get_default();
    visual = gdk_screen_get_rgba_visual(screen);

    if (visual != NULL && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(GTK_WIDGET(gobj()), visual);
    }
}

int AppWindow::init()
{



    this->set_title(PACKAGE_NAME);
    //std::string iconFile = Utilities::getExecPath(DEF_LOGONAME);
    //this->set_icon_from_file(iconFile);

    // A window to implement a docking bar.
    // Used for creating the dock panel.

    this->set_skip_taskbar_hint(true);
    this->set_skip_pager_hint(true);
    this->set_decorated(false);
    this->set_type_hint(Gdk::WindowTypeHint::WINDOW_TYPE_HINT_DOCK);


    //this->resize(300, 1000); //debug


    this->add(m_dockpanel);

    //this->add(dockBckPanel);
    //void Gtk::Window::set_transient_for (Window& parent)
    //  this->set_transient_for(&dockBckPanel);
    //    

    //https://developer.gnome.org/gtk-tutorial/stable/x2431.html
    this->add_events(
                     Gdk::PROPERTY_CHANGE_MASK |
                     Gdk::POINTER_MOTION_HINT_MASK |
                     Gdk::ENTER_NOTIFY_MASK |
                     Gdk::LEAVE_NOTIFY_MASK |
                     Gdk::POINTER_MOTION_MASK |
                     Gdk::STRUCTURE_MASK |
                     Gdk::PROPERTY_CHANGE_MASK);






    //
    GdkScreen *screen = gdk_screen_get_default();
    WnckScreen *wnckscreen = wnck_screen_get(0);
    //
    //    // needed for "configure-event"
    //    this->add_events(Gdk::STRUCTURE_MASK);
    //
    //    // The monitors-changed signal is emitted when the number,
    //    // size or position of the monitors attached to the screen change.


    g_signal_connect(G_OBJECT(screen), "size-changed",
                     G_CALLBACK(monitor_size_changed_callback),
                     (gpointer) this);

    g_signal_connect(G_OBJECT(screen), "monitors-changed",
                     G_CALLBACK(monitor_changed_callback),
                     (gpointer) this);
    //  g_signal_connect(G_OBJECT(wnckscreen), "property_change",
    //                 G_CALLBACK(AppWindow::LeaveFunc), NULL);
    /*
        g_signal_connect(wnckscreen, "active_window_changed",
                         G_CALLBACK(AppWindow::on_active_window_changed_callback), NULL);



        g_signal_connect(G_OBJECT(wnckscreen), "window_opened",
                         G_CALLBACK(window_opened_callback), NULL);

        g_signal_connect(G_OBJECT(wnckscreen), "window_opened",
                         G_CALLBACK(window_opened_callback), NULL);

     */

    // Launch timer every second
    // Glib::signal_timeout().connect(sigc::mem_fun(*this,&AppWindow::on_timeoutDraw), 1000);

    //   g_signal_connect(window, "geometry-changed",
    //                 G_CALLBACK(geometry_changed), NULL);
    // wnck_
    // Initialize the monitor geometry.
    //https://lazka.github.io/pgi-docs/Wnck-3.0/classes/Window.html

    this->show_all();
    this->m_screen.init(this);
    if (DockWindow::init(this) != 0) {
        return -1;
    }
    m_dockpanel.preInit(this);

    return 0;
}

bool AppWindow::fullScreenTimer()
{
    return true;
    //    if (Configuration::is_autoHide()) {
    //        return true;
    //    }
    //
    //    m_isfullscreen = WindowControl::FullscreenActive();
    //    if (m_isfullscreen && !m_isfullscreenSet && DockWindow::is_visible()) {
    //        //   g_print("m_isfullscreen\n");
    //        DockWindow::hide();
    //        m_isfullscreenSet = true;
    //
    //        return true;
    //    }
    //
    //    if (!m_isfullscreen && m_isfullscreenSet && !DockWindow::is_visible()/* && !Configuration::is_autoHide()*/) {
    //        m_isfullscreenSet = false;
    //        //     g_print("normal\n");
    //        DockWindow::show();
    //        m_isfullscreenSet = false;
    //    }
    //    //g_print("check full sccreen\n");
    //    return true;
}

void AppWindow::monitor_size_changed_callback(GdkScreen *screen, gpointer gtkwindow)
{
    g_print("...........!!!!!!!!!!!!!!!!!!!!!!!!!!....Monitor size changed\n");




    // Adapt the monitor geometry and repositioning the window
    //  MonitorGeometry::update((Gtk::Window*)gtkwindow);

    //DockWindow::removeStrut();
    // DockWindow::updateGeometry();
    //DockWindow::show();
    //DockWindow::updateStrut();





}

void AppWindow::on_active_window_changed_callback(WnckScreen *screen,
                                                  WnckWindow *previously_active_window, gpointer user_data)
{
    // m_currentMoveIndex = -1;

    // if (m_previewWindowActive || DockPanel::m_dragdropsStarts)
    //     return;

    WnckWindow *window = wnck_screen_get_active_window(screen);


    // g_print("\n");
    // g_print(wnck_window_get_name(window) );

}

void AppWindow::geometry_changed(WnckWindow *window)
{
    // if (wnck_window_is_maximized(window)) {
    g_print("A window has been maximized\n");
    // }
}

void AppWindow::window_opened_callback(WnckScreen* screen, WnckWindow* window, gpointer data){
    //g_print("open\n");
    // g_print(wnck_window_get_name(window) );

    //  g_signal_connect(window, "geometry-changed",
    //         G_CALLBACK(AppWindow::window_geometry_changed_callback), NULL);

    // if (wnck_window_is_maximized(window)) {
    //
    //    }
}

void AppWindow::monitor_changed_callback(GdkScreen *screen, gpointer gtkwindow)
{
    /*
    // Sleep for 2 seconds to give the monitor time to update the geometry
    using namespace std::chrono_literals;
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(2s);

    */ 
    DockWindow::removeStrut();
    DockWindow::updateGeometry();
    DockWindow::showDockWindow();
    
    if(Configuration::is_autoHide() == false){
        DockWindow::updateStrut();
    }

    
   
}

WnckWindow *fullscreen_window = NULL;

void AppWindow::window_geometry_changed_callback(WnckWindow *window, gpointer user_data)
{
    return;
    //    // if( wnck_window_is_fullscreen(window))
    //    g_print("geo change\n");
    //
    //
    //
    //
    //    m_isfullscreen = WindowControl::FullscreenActive();
    //
    //    if (m_isfullscreen) {
    //        fullscreen_window = window;
    //        DockWindow::hide();
    //        return;
    //    }
    //
    //    if (m_isfullscreen && window == fullscreen_window) {
    //        fullscreen_window = NULL;
    //        DockWindow::show();
    //    }
    //
    //    return;
    //
    //    if (m_isfullscreen && !m_isfullscreenSet) {
    //
    //        g_print("m_isfullscreen\n");
    //        DockWindow::hide();
    //
    //        m_isfullscreenSet = true;
    //
    //        return;
    //
    //    }
    //
    //    if (!m_isfullscreen && m_isfullscreenSet) {
    //        //  m_isfullscreenSet=false;
    //        m_isfullscreenSet = false;
    //        //        if(!m_isfullscreenSet)
    //        ////            return true;
    //        //        int x=0;
    //        //        int y=0;
    //        //        this->get_position(x,y);
    //
    //        //        if(y != DockWindow::GetGeometry().height)
    //        //        {
    //        //            return true;
    //        //        }
    //
    //        //    g_print("normal\n");
    //
    //        DockWindow::show();
    //        m_isfullscreenSet = false;
    //    }

}

bool AppWindow::on_enter_notify_event(GdkEventCrossing* crossing_event)
{
    // g_print("IN\n");
    m_mouseIn = true;
    return true;
}

bool AppWindow::on_leave_notify_event(GdkEventCrossing* crossing_event)
{
    //  g_print("OUT\n");
    m_mouseIn = false;
    return true;
}

/*
bool AppWindow::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {


    //Gtk::Allocation allocation = get_allocation();
    //const int width = allocation.get_width();
    //const int height = allocation.get_height();

    //Glib::RefPtr<Gdk::Pixbuf> icon = NULLPB;

    cr->set_source_rgba(0.0, 0.0, 0.8, 0.6); // partially translucent
    //  cr->rectangle(0, 0, double(width)/10, double(height)/10);
    cr->paint();

    return true;
}
 */


AppWindow::~AppWindow(){ }
