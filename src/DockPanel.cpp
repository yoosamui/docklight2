/*
 * File:   DockPanel.cpp
 * Author: yoo
 *
 * Created on November 6, 2018, 7:31 PM
 */

#include "DockItem.h"
#include "DockPanel.h"
#include "DockWindow.h"
#include "Configuration.h"
#include "DockItemPositions.h"
#include "Launcher.h"
#include "Animations.h"
#include "AppWindow.h"
#include "DockWindow.h"
#include "WnckHandler.h"

#include <iostream>
#include <thread>

//#include <glibmm/i18n.h>

// static members
int DockPanel::m_currentMoveIndex;
bool DockPanel::m_forceDraw;    
Glib::RefPtr<Gdk::Pixbuf> DockPanel::m_AppRunImage;
bool DockPanel::m_AppThreadRunning;
guint DockPanel::m_widthDecrement;
guint DockPanel::m_heightDecrement;

/*
 * This class is the  main dock renderer.
 */
DockPanel::DockPanel(): m_homeiconFilePath(Utilities::getExecPath(DEF_ICONNAME))
{

    // Set event masks 
    add_events(Gdk::BUTTON_PRESS_MASK |
            Gdk::BUTTON_RELEASE_MASK |
            Gdk::SCROLL_MASK |
            Gdk::SMOOTH_SCROLL_MASK |
            Gdk::POINTER_MOTION_HINT_MASK |
            Gdk::FOCUS_CHANGE_MASK |
            Gdk::ENTER_NOTIFY_MASK |
            Gdk::LEAVE_NOTIFY_MASK |
            Gdk::POINTER_MOTION_MASK);

    // set the static member to the default values
    DockPanel::m_currentMoveIndex = -1;
    DockPanel::m_forceDraw = false;
    DockPanel::m_AppThreadRunning = true;
    DockPanel::m_widthDecrement = 0;
    DockPanel::m_heightDecrement = 0;

    // Gets the default WnckScreen on the default display.
    WnckScreen *wnckscreen = wnck_screen_get_default();

    // set the required signals handlers
    g_signal_connect(G_OBJECT(wnckscreen), "window-opened", G_CALLBACK(DockPanel::on_window_opened), NULL);
    g_signal_connect(wnckscreen, "window_closed", G_CALLBACK(DockPanel::on_window_closed), NULL);
    g_signal_connect(wnckscreen, "active_window_changed", G_CALLBACK(DockPanel::on_active_window_changed_callback), NULL);
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &DockPanel::on_timeoutDraw), DEF_FRAMERATE);

    // Menus
    m_HomeMenu.attach_to_widget(*this);
    m_HomeMenu.accelerate(*this);
    m_HomeMenu.signal_hide().connect(sigc::mem_fun(*this, &DockPanel::on_HideMenu_event));
    m_QuitMenuItem.signal_activate().connect(sigc::mem_fun(*this, &DockPanel::on_QuitMenu_event));

    m_ItemMenu.attach_to_widget(*this);
    m_ItemMenu.accelerate(*this);
    m_ItemMenu.signal_hide().connect(sigc::mem_fun(*this, &DockPanel::on_HideMenu_event));
    m_MenuItemNewApp.signal_activate().connect(sigc::mem_fun(*this, &DockPanel::on_NewMenu_event));
    m_MenuItemAttach.signal_activate().connect(sigc::mem_fun(*this, &DockPanel::on_AttachMenu_event));
    m_MenuItemDetach.signal_activate().connect(sigc::mem_fun(*this, &DockPanel::on_DettachMenu_event));
}

/**
 * preInit load the attached icons and initializes variables.
 * @param Gtk::Window*  window
 * @return return 0 is success or -1 is an error found
 */
int DockPanel::Init(Gtk::Window* window)
{
   this->m_AppWindow = window; 


   this->m_AppUpdater = new AppUpdater();

    const char* filename = m_homeiconFilePath.c_str();
    DockItem* dockItem = new DockItem();
    try {
        int iconsize = Configuration::get_CellWidth() - ICON_CELL_WIDTH_MARGIN;
        dockItem->m_image = Gdk::Pixbuf::create_from_file(filename, iconsize, iconsize, true);
    }
    catch (Glib::FileError fex) {
        g_critical("preInit: file %s could not be found!\n", filename);
        return -1;

    }
    catch (Gdk::PixbufError bex) {
        g_critical("preInit: file %s PixbufError!\n", filename);
        return -1;
    }

    // add the home icon
    dockItem->m_appname = _("Desktop");
    dockItem->m_realgroupname = _("Desktop");
    dockItem->m_dockitemtype = DockItemType::SingleDock;
    dockItem->m_index = 0;
    this->m_AppUpdater->m_dockitems.insert(this->m_AppUpdater->m_dockitems.begin(), dockItem);

    // Start the background thread for application start animation
    m_AppRunThreadLauncher = new std::thread(AppRunAnimation); 


    // testt separator
        
          //dockItem = new DockItem(12, Configuration::get_CellHeight());
          //dockItem->m_dockitemtype = DockItemType::Separator;
          //dockItem->m_isAttached = true;
          //this->m_AppUpdater->m_dockitems.push_back(dockItem);

          //dockItem = new DockItem(24, Configuration::get_CellHeight());
          //dockItem->m_dockitemtype = DockItemType::Separator;
          //dockItem->m_isAttached = true;
          //this->m_AppUpdater->m_dockitems.push_back(dockItem);
          


    return 0;
}

/*
 *  destructor
 */
DockPanel::~DockPanel()
{ 
    // tell the background thread to terminate.
    m_AppThreadRunning = false;

    // Detach 
    m_AppRunThreadLauncher->detach();

    // free memory
    delete m_AppRunThreadLauncher;

    // pointed dangling to ptr NULL
    m_AppRunThreadLauncher = NULL;

    delete m_AppUpdater;
    m_AppUpdater = NULL;


    g_print("DockPanel destroy.\n");
}

/**
 * This method will be call from the background tread to manage the application image animation.
 * The animation consists in invert the colors of the image. 
 */
void DockPanel::AppRunAnimation()
{
    gint x, y, i;
    while(m_AppThreadRunning){
        if (m_AppRunImage && m_AppRunImage->get_colorspace() == Gdk::COLORSPACE_RGB && m_AppRunImage->get_bits_per_sample() == 8){
            int w = m_AppRunImage->get_width();
            int h = m_AppRunImage->get_height();
            int channels = m_AppRunImage->get_n_channels();
            gint rowstride = m_AppRunImage->get_rowstride();
            gint pixel_offset;
            for (i = 0; i < 6; i++) {
                for (y = 0; y < h; y++) {
                    for (x = 0; x < w; x++) {
                        pixel_offset = y * rowstride + x * channels;
                        guchar* pixel = &m_AppRunImage->get_pixels()[pixel_offset];

                        pixel[0] = 255 - pixel[0];
                        pixel[1] = 255 - pixel[1];
                        pixel[2] = 255 - pixel[2];
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            }
            m_AppRunImage = NULLPB;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
/**
 * Emitted when a new Wnck.Window is opened on screen.
 * @param screen
 * @param window
 * @param data
 */
void DockPanel::on_window_opened(WnckScreen *screen, WnckWindow* window, gpointer data)
{
    update();
}

/**
 * Emitted when a Wnck.Window is closed on screen.
 * @param screen
 * @param window
 * @param data
 */
void DockPanel::on_window_closed(WnckScreen *screen, WnckWindow *window, gpointer data)
{
    update();
}
/**
 * Emitted when the active window on screen has changed.
 * @param screen
 * @param previously_active_window
 * @param user_data
 */
void DockPanel::on_active_window_changed_callback(WnckScreen *screen, WnckWindow *previously_active_window, gpointer user_data)
{
/*
    m_currentMoveIndex = -1;

    if (m_previewWindowActive || DockPanel::m_dragdropsStarts)
        return;

    WnckWindow * window = wnck_screen_get_active_window(screen);
    if (window == NULL)

        return;
    DockPanel::setItemImdexFromActiveWindow();
    */
}

/**
 * Emitted when the menu is clicked.
 */
void DockPanel::on_NewMenu_event()
{
    DockItem* item = this->get_CurrentItem();
    if (item == nullptr)
        return;

    if (item->m_dockitemSesssionGrpId > 0) {
        //  createSessionWindow();
        return;

    }

    // start application launcher
    if (!Launcher::Launch(item->m_realgroupname)) {

        //   createLauncher(item);
    }

    // start the animation
    m_AppRunImage = item->m_image;

}

/**
 * Close application and release resources.
 */
void DockPanel::on_QuitMenu_event()
{
    m_AppWindow->close();
}

/**
 * Emitted when the menu hides.
 */
void DockPanel::on_HideMenu_event()
{
    this->m_popupMenuOn = false;    
}

/**
 *Emitted when the menu Attach to Dock is selected.
 */
void DockPanel::on_AttachMenu_event()
{
     this->m_AppUpdater->AttachItem(this->m_currentMoveIndex);
}

void DockPanel::on_DettachMenu_event()
{
    if (!this->m_AppUpdater->DettachItem(this->m_currentMoveIndex)){
        return;
    }

    this->update();       

    // force resize the dock window.   
    DockWindow::update();

}

/**
 * bool DockPanel::on_button_press_event(GdkEventButton *event)
 *
 * handles Mouse button press : process mouse button event true to stop other handlers from being invoked for the event.
 * false to propagate the event further.
 */
bool DockPanel::on_button_press_event(GdkEventButton *event)
{

    if ((event->type == GDK_BUTTON_PRESS)) {

        m_mouseclickEventTime = gtk_get_current_event_time();

        // Check if the event is a left button click.
        if (event->button == 1 && !m_mouseLeftButtonDown) {
            m_mouseLeftButtonDown = true;

            // start drag & drop timer
            if (this->m_currentMoveIndex > 0){

                //int x, y;
                //Utilities::getMousePosition(x, y);
                //// sets the relative item mouse coordinates         
                //m_dragdropMousePoint.set_x((int) x);
                //m_dragdropMousePoint.set_y((int) y);
                //int x, y;
                //DockWindow::get_DockWindowPosition(x, y);

                m_dragdropMousePoint.set_x((int) event->x);
                m_dragdropMousePoint.set_y((int) event->y);
                m_dragdropTimer.start();
                g_print("Mose %d/%d \n", (int)event->x, (int)event->y);
            }

            // The event has been handled.
          //  return true;
        }



                //g_print("START DRAG.......................\n");
        //m_currentMoveIndex = get_Index(event->x, event->y);

        //// Set Drag and drop variables and Starts the timer
        //if (event->button == 1 &&  m_currentMoveIndex > 0) {

            
        ////    if (currentMillis - previousMillis >= mouseDelay) {


            //if (gtk_get_current_event_time() - m_mouseclickEventTime > 2000) {

            //}
            //else{
                //// remember the time to check it later
                //m_mouseclickEventTime = gtk_get_current_event_time();

            //}
        //}
        
        
        
        // g_print("MDOWN %d\n", m_currentMoveIndex);
        //      m_mouseRightClick = false;
        /*
        // Set Drag and drop variables and Starts the timer
        if (event->button == 1 && !m_dragdropTimerSet && m_currentMoveIndex > 0) {

        // remember the item we want to drag
        m_dragdropItemIndex = m_currentMoveIndex;
        int relativex = DockPosition::getDockItemRelativeMouseXPos(
        (int) m_dockitems.size(), m_dragdropItemIndex,
        m_cellwidth, (int) event->x);

        // sets the relative item mouse coordinates
        m_dragdropMousePoint.set_x(relativex);
        m_dragdropMousePoint.set_y((int) event->y);

        // Variables are set
        m_dragdropTimerSet = true;
        // Start the timer
        m_dragdropMouseDown = true;
        m_dragdropTimer.start();
        // g_print("Drag timer starts\n");
        }
        */


        // Check if the event is a right button click.
        if (event->button == 3 && !m_mouseRightButtonDown) {
            m_mouseRightButtonDown = true;
            // The event has been handled.

            // Items
            if (m_currentMoveIndex > 0) {
                //DockItem *dockitem = m_AppUpdater->m_dockitems[m_currentMoveIndex];
                //dockitem->m_isAttached = true;
                //dockitem->m_attachedIndex = m_currentMoveIndex;
                //this->m_AppUpdater->Save();
            }

            return true;
        }
    }
    return false;
}

/**
 * Returning TRUE means we handled the event, so the signal emission should be stopped (don’t call any further callbacks
 * that may be connected). Return FALSE to continue invoking callbacks. handles Mouse button released : process mouse button event
 * true to stop other handlers from being invoked for the event. false to propagate the event further.
 */
bool DockPanel::on_button_release_event(GdkEventButton *event)
{
    // Check if a item was drop
    if (m_DragDropBegin){

        delete m_DragAndDropWindow;
        m_DragAndDropWindow = nullptr;

        m_DragDropBegin = false;

    //    if(this->m_mouseIn){
            this->m_DragDropTargetIndex = this->m_currentMoveIndex;
            this->m_AppUpdater->SwapItems(this->m_DragDropSourceIndex, this->m_DragDropTargetIndex);
      //  }

        this->m_DragDropSourceIndex = this->m_DragDropTargetIndex = 0;
      }




    // Taking Too Long To Release the mouse.
    // That is not a valid Click.
    if ((gtk_get_current_event_time() - m_mouseclickEventTime) > 200) {
        m_mouseLeftButtonDown = false;
        m_mouseRightButtonDown = false;
        return true;
    }

    if (!m_mouseIn)
        return true;

    if (m_mouseLeftButtonDown) {
        m_mouseLeftButtonDown = false;
        this->ExecuteApp(event);
        //  SelectWindow(m_currentMoveIndex, event);
        return TRUE;
    }

    // Right mouse click
    if (m_mouseRightButtonDown) {
        // m_preview.hideMe();
        //  m_mouseRightClick = true;
        m_mouseRightButtonDown = false;

        // Menus
        if (m_currentMoveIndex == 0) {
            m_HomeMenu.popup(sigc::mem_fun(*this, &DockPanel::on_popup_homemenu_position), 1, event->time);
            this->m_popupMenuOn = true;
        }
        else  if (m_currentMoveIndex > 0) {
            m_ItemMenu.popup(sigc::mem_fun(*this,&DockPanel::on_popup_itemmenu_position), 1, event->time);
            this->m_popupMenuOn = true;
        }

        DockItem* item = this->get_CurrentItem();

        if (item != nullptr){
            m_MenuItemAttach.set_sensitive(item->m_isAttached == false);
            m_MenuItemDetach.set_sensitive(item->m_isAttached /*&& item->m_items.size() == 0*/);
        }
    }
// mouse right       

/*
// Items
if (m_currentMoveIndex > 0) {
DockItem *dockitem = m_dockitems.at(m_currentMoveIndex);
int isExitstWindows =
WindowControl::isExitstWindowsByDockItem(dockitem);
bool isExitstActiveWindow =
WindowControl::isExitsActivetWindowByDockItem(dockitem);
bool maximizedexistst =
WindowControl::isExistsUnMaximizedWindowsByDockItem(dockitem);


m_MenuItemNewApp.set_label(_("Open new"));
m_MenuItemNewApp.set_sensitive(true);
if (dockitem->m_dockitemSesssionGrpId > 0) {

m_MenuItemNewApp.set_label(_("Configure"));
m_MenuItemNewApp.set_sensitive(true);

}


m_MenuItemMinimizedAll.set_sensitive(isExitstWindows > 0 && maximizedexistst);

m_MenuItemMinimizedAllExceptActive.set_sensitive(isExitstWindows > 1
&& maximizedexistst && isExitstActiveWindow);
m_MenuItemUnMinimizedAll.set_sensitive(isExitstWindows);


m_MenuItemCloseAllExceptActive.set_sensitive(isExitstWindows > 1 && isExitstActiveWindow);
m_MenuItemCloseAll.set_sensitive(isExitstWindows);

m_MenuItemAttach.set_sensitive(dockitem->m_isAttached == false);
m_MenuItemDetach.set_sensitive(dockitem->m_isAttached && dockitem->m_items.size() == 0);


if (!m_Menu_Popup.get_attach_widget())
m_Menu_Popup.attach_to_widget(*this);


//m_HomeMenu_Popup.resize_children();
m_Menu_Popup.set_halign(Gtk::Align::ALIGN_CENTER);
m_Menu_Popup.popup(sigc::mem_fun(*this,
&DockPanel::on_popup_menu_position), 1, event->time);

m_popupMenuOn = true;
return true;
}

//Home
if (m_currentMoveIndex == 0) {

int wincount = WindowControl::windowscount();
//int minimizedexitst = WindowControl::isExistsMinimizedWindows();
int unminimized = WindowControl::unMinimizedWindowsCount();

m_HomeCloseAllWindowsMenuItem.set_sensitive(wincount > 0);
m_HomeCloseAllWindowsExceptActiveMenuItem.set_sensitive(wincount > 1);
m_HomeMinimizeAllWindowsMenuItem.set_sensitive(unminimized > 0);
m_HomeUnMinimizeAllWindowsMenuItem.set_sensitive(wincount > 0);

m_HomeMinimizeAllWindowsExceptActiveMenuItem.set_sensitive(unminimized > 1);

if (!m_HomeMenu_Popup.get_attach_widget())
m_HomeMenu_Popup.attach_to_widget(*this);

//m_HomeMenu_Popup.resize_children();
m_HomeMenu_Popup.set_halign(Gtk::Align::ALIGN_CENTER);
m_HomeMenu_Popup.popup(sigc::mem_fun(*this,
            &DockPanel::on_popup_homemenu_position), 1, event->time);

m_popupMenuOn = true;

return true;
}

return TRUE;
}
*/
return TRUE;
}

bool DockPanel::on_scroll_event(GdkEventScroll* e)
{
    int index = m_currentMoveIndex;
    if (index == -1 || index == 0)
        return true;

    DockItem * item = this->m_AppUpdater->m_dockitems[index]->get_Next();
    if (item == nullptr)
        return true;

    WnckWindow * window = item->m_window;
    if ((int)e->delta_y == (int)1) {
        WnckHandler::ActivateWindow(window);
    }
    else if ((int)e->delta_y == (int) - 1) {
        WnckHandler::ActivateWindow(window);
    }

    // Event has been handled
    return true;
}
/**
 * Handle the home menu popup position.
 * @param x
 * @param y
 * @param push_in
 */
void DockPanel::on_popup_homemenu_position(int& x, int& y, bool& push_in)
{
    
    DockItemPositions::get_CenterPosition(m_currentMoveIndex, x, y, m_HomeMenu.get_width(), m_HomeMenu.get_height());
}

void DockPanel::on_popup_itemmenu_position(int& x, int& y, bool& push_in)
{
    DockItemPositions::get_CenterPosition(m_currentMoveIndex, x, y, m_ItemMenu.get_width(), m_ItemMenu.get_height());
}

void DockPanel::ExecuteApp(GdkEventButton* event)
{
    if (m_currentMoveIndex < 1)
        return;

    DockItem * item = this->m_AppUpdater->m_dockitems[m_currentMoveIndex];
    if (item->m_dockitemtype == DockItemType::Separator) {
        return;
    }

    if (item->m_items.size() == 0) {
        on_NewMenu_event();
        return;
    }

    // TEST must be item->m_items.size() == 1
    // must be item->m_items.size() >1 // open preview
    if (item->m_items.size() >= 1) {
        WnckWindow *window = nullptr;
        window = WnckHandler::get_ActiveWindowIfAny(item);
        if (window == nullptr) {
            DockItem* firstChild = item->m_items[0];
            window = firstChild->m_window;
        }

        WnckHandler::ActivateWindow(window);
    }
}

/**
 * Gets the current dock Item.
 */
inline DockItem* DockPanel::get_CurrentItem()
{
    if (m_currentMoveIndex < 0 || m_currentMoveIndex > this->m_AppUpdater->m_dockitems.size()) {
        return nullptr;
    }

    return this->m_AppUpdater->m_dockitems[m_currentMoveIndex];
}



bool DockPanel::get_AutohideAllow()
{
    return this->m_popupMenuOn == false;
}

/**
 * Force redraw. 
 */
void DockPanel::update()
{
    m_forceDraw = true;
}

/**
 * Run at defined FRAMERATE
 */
bool DockPanel::on_timeoutDraw()
{
    if (!m_DragDropBegin && this->m_currentMoveIndex > 0 &&  m_mouseLeftButtonDown && m_dragdropTimer.elapsed() >= 0.20 ){
        g_print("timer mouse down %f\n", m_dragdropTimer.elapsed());
        m_dragdropTimer.stop();
        m_dragdropTimer.reset();
        
        DockItem* item =  this->get_CurrentItem();
        if (item != nullptr){
            m_DragDropBegin = true;
            this->m_DragDropSourceIndex = this->m_currentMoveIndex;

            this->m_DragAndDropWindow = new DragAndDropWindow();
            this->m_DragAndDropWindow->Show(item->m_image, item, m_dragdropMousePoint);
        }
    }


    if (m_mouseIn || m_forceDraw || m_AppRunImage) {
        Gtk::Widget::queue_draw();
        m_forceDraw = false;
    }
    return true;
}

/**
 * A
 * calculate the item index from mouse coordinates.
 * @param x
 * @param y
 * @return the item index or -1 if the item could not be found.
 */
inline int DockPanel::get_Index(const int& mouseX, const int& mouseY)
{
    Gdk::Point mouse(mouseX, mouseY);
    int idx = 0;
    int x = DockWindow::get_dockWindowStartEndMargin() / 2;
    int y = x;

    if (DockWindow::is_Horizontal())
    {
        for (auto item : this->m_AppUpdater->m_dockitems) 
        {
            if (mouse.get_x() >= x && mouse.get_x() <= x + item->get_Width())                 
            {
                return idx;
            }

            x += item->get_Width() + Configuration::get_separatorMargin();
            idx++;
        }
    }
    else 
    {   
        int height;
        for (DockItem* item : this->m_AppUpdater->m_dockitems) 
        {
            height = item->m_dockitemtype == DockItemType::Separator ? item->get_Width() : item->get_Height();

            if (mouse.get_y() >= y && mouse.get_y() <= y + height) 
            {
                return idx;
            }

            y += height + Configuration::get_separatorMargin();
            idx++;
        }
    }

    return -1;
}

/**
 * handles Mouse motion: process mouse button event
 * true to stop other handlers from being invoked for the event.
 * false to propagate the event further.
 * @param event
 * @return false
 */
bool DockPanel::on_motion_notify_event(GdkEventMotion * event)
{
    m_currentMoveIndex = this->get_Index(event->x, event->y);
    return false;
}

void DockPanel::RoundedRectangle(const Cairo::RefPtr<Cairo::Context>& cr,
        double x, double y, double width, double height, double radius)
{
    // radius can be no larger than half our height or width
    radius = std::min(radius, std::min(width / 2, height / 2));
    cr->move_to(x + radius, y);
    cr->arc(x + width - radius, y + radius, radius, M_PI * 1.5, M_PI * 2);
    cr->arc(x + width - radius, y + height - radius, radius, 0, M_PI * .5);
    cr->arc(x + radius, y + height - radius, radius, M_PI * .5, M_PI);
    cr->arc(x + radius, y + radius, radius, M_PI, M_PI * 1.5);

}

//obsolete use DockItemPosition instead
/*
   unsigned int DockPanel::get_dockItemsWidthUntilIndex(int idx)
   {
   unsigned int size = 0;
   int count = 0;
   for (DockItem* item:this->m_AppUpdater->m_dockitems) {
   if (count == idx) {
   break;
   }
   size += item->get_Width();

   count++;
   }

   return size;
   }
   */
//obsolete use DockItemPosition instead
/*
   unsigned int DockPanel::get_dockItemsHeightUntilIndex(int idx)
   {
   unsigned int size = 0;
   int count = 0;
   for (DockItem* item:this->m_AppUpdater->m_dockitems) {
   if (count == idx) {
   break;
   }
   if (item->m_dockitemtype == DockItemType::Separator) {
   size += item->get_Width();
   count++;
   continue;
   }
   size += item->get_Height();

   count++;
   }

   return size;
   }
   */


bool DockPanel::on_enter_notify_event(GdkEventCrossing * crossing_event)
{
    //  g_print("Mp_IN\n");
    m_mouseIn = true;
    m_HomeMenu.hide();
    m_ItemMenu.hide();
//    Gtk::Menu::signal_hide(
    return true;
}

bool DockPanel::on_leave_notify_event(GdkEventCrossing * crossing_event)
{
    // g_print("Mp_OUT\n");
    m_mouseIn = false;

    m_titlewindow.hide();
    m_infowindow.hide();

    return true;
}

/**
 * Calculate the draw position for all items.
 */
inline void DockPanel::get_ItemPosition(const DockItemType dockType, int& x, int& y, int& width, int& height)
{
    static int nextsize = 0;

    if (DockWindow::is_Horizontal())
    {
        if (x == 0 ) {
            y = (DockWindow::get_DockWindowHeight() / 2) - Configuration::get_CellHeight() / 2;
            x = DockWindow::get_dockWindowStartEndMargin() / 2;
            nextsize = width;
            return;
        }

        // if the item is a separator the width is probably not equal.
        // in this case wie remeber the size for use it in the next item.
        if( dockType == DockItemType::Separator){
            x +=  nextsize + Configuration::get_separatorMargin();
            nextsize = width;
            return;
        }

        x +=  nextsize + Configuration::get_separatorMargin();
        nextsize = width;
    }
    else
    {
        if (y == 0 ){
            x = (DockWindow::get_DockWindowWidth() / 2) - Configuration::get_CellWidth() / 2;
            y = DockWindow::get_dockWindowStartEndMargin() / 2;
            nextsize = height;
            return;
        }

        // if the item is a separator the wisth is probably not equal.
        // in this case wie remeber the size for use it in the next item.
        if( dockType == DockItemType::Separator){
            y +=  nextsize + Configuration::get_separatorMargin();
            height = nextsize = width;
            width = Configuration::get_CellWidth();
            return;
        }

        y +=  nextsize + Configuration::get_separatorMargin();
        nextsize = height;
    }
}
/**
 * The cairo main renderer
 * @param the cairo contex.
 */
bool DockPanel::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    if (Configuration::is_allowDraw() == false) {
        return true;
    }

    this->draw_Panel(cr);
    this->draw_Items(cr);
    this->show_Title();

    return true;
}

/**
 * Draw the panel in all locations
 * @param cr cairo context
 * @param x current position x
 * @param y current position y
 */
void DockPanel::draw_Panel(const Cairo::RefPtr<Cairo::Context>& cr)
{
    if (Configuration::is_panelMode()) {

        cr->set_source_rgba(0.0, 0.0, 0.0, 0.8);
        cr->paint();
        //RoundedRectangle(cr, x, y, DockWindow::get_DockWindowWidth(), DockWindow::get_DockWindowHeight(), 6);
    }
    else {
        cr->set_source_rgba(0.0, 0.0, 175.8, 0.4);
        RoundedRectangle(cr, 0, 0, DockWindow::get_DockWindowWidth(), DockWindow::get_DockWindowHeight(), 6);
        cr->fill();
    }

}

/**
 * Render all dock items
 * @param cr cairo context
 */
void DockPanel::draw_Items(const Cairo::RefPtr<Cairo::Context>& cr)
{
    static guint widthDecrement = 0;
    static guint heightDecrement = 0; 
    
    int idx = 0;
    int center = 0;
    int iconsizeWidth = 0;
    int iconsizeHeight = 0;

    int y = 0;
    int x = 0; 
    int width = 0;
    int height = 0;

    int itemsCount = this->m_AppUpdater->m_dockitems.size();

    this->m_widthDecrement = 0;
    this->m_heightDecrement = 0;

    if (DockWindow::is_Horizontal()){

        this->m_widthDecrement = DockItemPositions::get_ResizeWidthDecrement();
        if (widthDecrement != this->m_widthDecrement){

            widthDecrement = this->m_widthDecrement;
            DockWindow::update();
        }

    }else{

        this->m_heightDecrement = DockItemPositions::get_ResizeHeightDecrement();
        if (heightDecrement != this->m_heightDecrement){

            heightDecrement = this->m_heightDecrement;
            DockWindow::update();
        }
    }

    g_print("Draw\n");
    // Draw all items with cairo
    for (idx = 0; idx < itemsCount; idx++) {

        DockItem* item = this->m_AppUpdater->m_dockitems[idx];
        width = item->get_Width();
        height = item->get_Height();

        this->get_ItemPosition(item->m_dockitemtype, x, y, width, height);
      
        item->m_posX = x;
        item->m_posY = y;

        // Draw cells
        cr->set_source_rgba(0.00, 0.50, 0.66, 1);
        RoundedRectangle(cr, x, y, width, height, 3);
        cr->fill();

        cr->set_line_width(1.5);
        RoundedRectangle(cr, x, y, width, height, 3);
        cr->set_source_rgba(255.0, 255.0, 255.0, 1);
        cr->stroke();

        // Draw dots and icons if the item is not a user separator
        if (item->m_dockitemtype != DockItemType::Separator) {
            // Dots
            center = (width / 2);
            if (item->m_items.size() > 0) {
                cr->set_source_rgb(1.0, 1.0, 1.0);
                if (item->m_items.size() == 1) {
                    cr->arc(x + center, y + height - 5, 2.0, 0, 2 * M_PI);
                }
                else if (item->m_items.size() > 1) {
                    cr->arc(x + center - 4, y + height - 5, 2.0, 0, 2 * M_PI);
                    cr->arc(x + center + 4, y + height - 5, 2.0, 0, 2 * M_PI);
                }
                cr->fill();
            }
            // icons
            if (item->m_image != NULLPB) {
                iconsizeWidth = width - CELL_MARGIN;
                iconsizeHeight = height - CELL_MARGIN;
                auto icon = item->m_image->scale_simple(iconsizeWidth, iconsizeHeight, Gdk::INTERP_BILINEAR);
                Gdk::Cairo::set_source_pixbuf(cr, icon, x + center - iconsizeWidth / 2, y + 1);
                cr->paint();
            }
        }

        // Selector
        if (idx == m_currentMoveIndex) {
            cr->set_source_rgba(255.0, 255.0, 255.0, 0.4);
            RoundedRectangle(cr, x, y, width, height, 3);
            cr->fill();

            cr->set_line_width(2.5);
            cr->set_source_rgba(255.0, 255.0, 255.0, 1);
            RoundedRectangle(cr, x, y, width, height, 3);
            cr->stroke();
        }
    }
}

/**
 * Show the application title.
 */
void DockPanel::show_Title()
{

    if (Configuration::is_autoHide() && !DockWindow::is_Visible()){
        m_titlewindow.hide();
        m_infowindow.hide();
        return;
    }


    DockItem* item = this->get_CurrentItem();
    if (item == nullptr/* || item->m_dockitemtype == DockItemType::Separator*/){
        m_titlewindow.hide();
        m_infowindow.hide();
        return;
    }

    // title window
    if (m_titleItemOldindex != m_currentMoveIndex) {

        m_titleItemOldindex = m_currentMoveIndex;
        m_titleElapsedSeconds = 0;
        m_titleTimer.start();
        m_titleShow = false;

        m_titlewindow.hide();
        m_infowindow.hide();
        //            if (m_previewWindowActive)
        //                m_preview.hideMe();

    }

    if (m_titleItemOldindex == m_currentMoveIndex) {

        if (m_titleElapsedSeconds > 0.3 && m_titleShow == false /* && !m_previewWindowActive*/) {
            
            int x, y;

            
            std::string title = item->get_Title();
           // if (item->m_items.size() > 1) {
                char buff[NAME_MAX];
                //sprintf(buff, "%s (%d)", title.c_str(), (int)item->m_items.size());
                sprintf(buff, "%s idx: (%d)", title.c_str(), (int)item->m_index);
                title = buff;
          //  }

            m_titlewindow.setText(title);
            DockItemPositions::get_CenterPosition(m_currentMoveIndex, x, y, m_titlewindow.get_width(), m_titlewindow.get_height());
            m_titlewindow.move(x, y);
            m_titleShow = true;

        }

        m_titleElapsedSeconds = m_titleTimer.elapsed();
    }
}

