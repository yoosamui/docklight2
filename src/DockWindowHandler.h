/* 
 * File:   MonitorGeometry.h
 * Author: yoo
 *
 * Created on November 4, 2018, 12:45 PM
 */

#ifndef MONITORGEOMETRY_H
#define	MONITORGEOMETRY_H

#define WNCK_I_KNOW_THIS_IS_UNSTABLE  1

#include <libwnck/libwnck.h>
#include <gtkmm.h>
#include "Configuration.h"

namespace DockWindow {
    /**
     * Reserve screen (STRUT) space and dock the window
     * @return 0 = success or -1 error
     */
    int updateStrut(int size);
    int updateStrut();
    void removeStrut();
    /**
     * Place the window in the curremt strut.
     * @return 
     */
    void show();
    
    /**
     * Hide the window from the current strut.
     */
    void hide();
    
//   namespace Monitor
//    {
//       
//        GdkRectangle get_geometry();
//       
//        
//        
//    }
    void move(int x, int y);
    unsigned int get_dockSize();
    GdkRectangle get_geometry();
    
    /**
     * Initialize the window
     * @param window the main window.
     * @return 0 is success -1 if any error.
     */
    int init(Gtk::Window* window);

      unsigned int getDockItemWidth();
        unsigned int getDockItemHeight();
      
         bool is_visible();
   
    void set_visible(bool visible);
   
        
   
    
    
    
    namespace DockWindow
    {
        unsigned int get_center();
        unsigned int get_size();
        
        
        
    }
        
       
        
        
    
}
#endif	/* MONITORGEOMETRY_H */

