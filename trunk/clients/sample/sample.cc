/************************************************************************
 *   Copyright (C) Mark Thomas <markbt@efaref.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <Y/c++.h>
#include "sample.h"

#include <iostream>
#include <stdlib.h>

using namespace sigc;
using namespace std;

Sample::Sample (Y::Connection *y_) : y(y_)
{
  window = new Y::Window (y, "Sample Application");

  rowlayout = new Y::YRowLayout(y);
  //window -> setChild (rowlayout);

  grid = new Y::GridLayout (y);
  window -> setChild (grid);

  window -> requestClose.connect (bind (sigc::ptr_fun (&exit), EXIT_SUCCESS));

  samplelabel = new Y::Label (y, "The DyS Window system");
  samplelabel->setAlignment("center");

  samplebutton = new Y::YButton (y);
  samplebutton->setLabel ( "Sample YButton" );

  sampletoggle = new Y::YToggleButton (y);
  sampletoggle->setLabel ( "YToggleButton" );

  samplecheckbox = new Y::YCheckbox (y);
  samplecheckbox->setLabel ( "YCheckbox" );

  /*
   * Here is the example radiobutton stuff!
   */

  sampleradiogroup = new Y::YRadioGroup (y);

  sampleradiobutton1 = new Y::YRadioButton (y);
  sampleradiobutton1->setLabel ( "Radio1" );
  sampleradiobutton1->setRadioGroup (*sampleradiogroup);

  sampleradiobutton2 = new Y::YRadioButton (y);
  sampleradiobutton2->setLabel ( "Radio2" );
  sampleradiobutton2->setRadioGroup (*sampleradiogroup);

  sampleradiobutton3 = new Y::YRadioButton (y);
  sampleradiobutton3->setLabel ( "Radio3" );
  sampleradiobutton3->setRadioGroup (*sampleradiogroup);


  //dont use bind if you are not passing any parameters to mousePressed
  samplebutton->mousePressed.connect (mem_fun(*this, &Sample::mousePressed));//register the callback
  samplebutton->mouseReleased.connect (mem_fun(*this, &Sample::mouseReleased));//register the callback

  sampletoggle->buttonPressed.connect (mem_fun(*this, &Sample::togglePressed));//register the callback
  sampletoggle->buttonReleased.connect (mem_fun(*this, &Sample::toggleReleased));//register the callback

  samplecheckbox->buttonPressed.connect (mem_fun(*this, &Sample::togglePressed));//register the callback
  samplecheckbox->buttonReleased.connect (mem_fun(*this, &Sample::toggleReleased));//register the callback


  samplebutton1 = new Y::YButton (y);
  samplebutton1->setLabel ( "BEST_FIT" );

  samplebutton2 = new Y::YButton (y);
  samplebutton2->setLabel ( "STRETCH" );

  samplebutton3 = new Y::YButton (y);
  samplebutton3->setLabel ( "BEST_FIT" );


  rowlayout -> addWidget (*samplebutton1, 1);
  rowlayout -> addWidget (*samplebutton2, 0);
  rowlayout -> addWidget (*samplebutton3, 1);


  grid -> addWidget (samplelabel, 0, 0, 7, 1);
  grid -> addWidget (samplebutton, 0, 1, 3, 1);

  grid -> addWidget (samplecheckbox, 4, 1, 3, 1);
  grid -> addWidget (sampletoggle, 0, 2, 3, 1);

  grid -> addWidget (sampleradiobutton1, 3, 3, 3, 1);
  grid -> addWidget (sampleradiobutton2, 3, 4, 3, 1);
  grid -> addWidget (sampleradiobutton3, 3, 5, 3, 1);

  //column, row, colspan, rowspan
  grid -> addWidget (rowlayout, 0, 6, 7, 1);

  window -> show ();
}

Sample::~Sample ()
{
}

void Sample::mousePressed() {
  samplebutton->setLabel ( "pressed" );
}

void Sample::mouseReleased() {
  samplebutton->setLabel ( "Sample YButton" );
  std::string temp = samplebutton->getLabel();
  std::cout << temp << "\n";
}

void Sample::togglePressed() {
  samplebutton->setLabel ( "Disabled" );
  samplebutton->setEnabled (false);
}

void Sample::toggleReleased() {
  samplebutton->setLabel ( "Sample YButton" );
  samplebutton->setEnabled (true);
}

int
main (int argc, char **argv)
{
  Y::Connection y;
  Sample s(&y);
  y.run();
}

/* arch-tag: e5a4fbe8-beb9-4c65-8e48-28489a967a50
 */
