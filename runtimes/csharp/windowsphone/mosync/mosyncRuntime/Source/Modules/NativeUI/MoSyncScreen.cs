﻿/* Copyright (C) 2011 MoSync AB

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License,
version 2, as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/
/**
 * @file MoSyncScreen.cs
 * @author Niklas Nummelin, Ciprian Filipas
 *
 * @brief This represents the Screen implementation for the NativeUI
 *        component on Windows Phone 7, language c#
 * @note The Icon property cannot be implemented on Windows Phone
 * @platform WP 7.1
 **/

using Microsoft.Phone.Controls;
using System.Windows.Controls;
using System.Windows;
using System.Windows.Navigation;
using System;
using System.Text.RegularExpressions;
using System.Reflection;
using System.Collections.Generic;

namespace MoSync
{
    namespace NativeUI
    {
        // Delegate used for shoew screen with transition.
        public delegate void Delegate_SwitchContentDelegate();

        public class Screen : WidgetBaseWindowsPhone, IScreen
        {
            //The container for the screen content.
            protected Grid mPage;

            //The screen title.
            private string mTitle;

            //The application bar associated to this screen.
            protected Microsoft.Phone.Shell.ApplicationBar mApplicationBar;

            //The delegate when switching the content with animated transition.
            Delegate_SwitchContentDelegate mSwitchContentDelegate = null;

            //The application bar visibility flag.
            protected bool ApplicationBarVisible
            {
                set;
                get;
            }

            //The application bar item indexes.
            //When a new button or menu item is added it gets
            //an index associated. That index is returned when the item is
            //succesfuly added.
            protected Dictionary<Object, int> mApplicationBarItemsIndexes;

            //The application bar item index seed.
            //This is a counter that gives the first
            //index value and it's incremented during
            //item additions.
            protected int mApplicationBarItemsIndexSeed = 0;

            //The constructor.
            public Screen()
            {
                //The container is a grid with 1 or 2 row definitions, and 1 column.
                //It may have 2 row definitions when an application bar is visible.
                mPage = new Grid();
                mPage.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
                mPage.RowDefinitions.Add(new RowDefinition { Height = new GridLength( 1, GridUnitType.Star) });
                mView = mPage;

                //Initialize the application bar and set its visibility to false.
                mApplicationBar = new Microsoft.Phone.Shell.ApplicationBar();
                mApplicationBar.IsVisible = false;
                ApplicationBarVisible = false;

                mApplicationBarItemsIndexes = new Dictionary<Object, int>();

                /**
                 * This will add a OrientationChanged event handler to the Application.Current.RootVisual, this is application wide.
                 */
                (Application.Current.RootVisual as Microsoft.Phone.Controls.PhoneApplicationFrame).OrientationChanged += new EventHandler<Microsoft.Phone.Controls.OrientationChangedEventArgs>(OrientationChangedHandler);
            }

            /**
             * Handles the back button pressed event.
             * @return true if the event has been consumed, false otherwise.
             */
            public virtual bool HandleBackButtonPressed()
            {
                return false;
            }

            /**
             * The Orientation changed event handler.
             * Currently it contains the functionality for the orientation changed event.
             * @param from Object the object that triggers the event.
             * @param args Microsoft.Phone.Controls.OrientationChangedEventArgs the event arguments.
             */
            public void OrientationChangedHandler(object from, Microsoft.Phone.Controls.OrientationChangedEventArgs args)
            {
                PhoneApplicationPage currentPage = (((PhoneApplicationFrame)Application.Current.RootVisual).Content as PhoneApplicationPage);

                // change the current page in regard to the current orientation.
                if (args.Orientation == PageOrientation.Landscape |
                    args.Orientation == PageOrientation.LandscapeLeft |
                    args.Orientation == PageOrientation.LandscapeRight)
                {
                    currentPage.Height = Application.Current.Host.Content.ActualWidth;
                    currentPage.Width = Application.Current.Host.Content.ActualHeight;
                }
                else if (args.Orientation == PageOrientation.Portrait |
                         args.Orientation == PageOrientation.PortraitDown |
                         args.Orientation == PageOrientation.PortraitUp)
                {
                    currentPage.Height = Application.Current.Host.Content.ActualHeight;
                    currentPage.Width = Application.Current.Host.Content.ActualWidth;
                }

                int mosyncScreenOrientation = MoSync.Constants.MA_SCREEN_ORIENTATION_PORTRAIT_UP;
                switch (currentPage.Orientation)
                {
                    case PageOrientation.Landscape:
                        mosyncScreenOrientation = MoSync.Constants.MA_SCREEN_ORIENTATION_LANDSCAPE;
                        break;
                    case PageOrientation.LandscapeLeft:
                        mosyncScreenOrientation = MoSync.Constants.MA_SCREEN_ORIENTATION_LANDSCAPE_LEFT;
                        break;
                    case PageOrientation.LandscapeRight:
                        mosyncScreenOrientation = MoSync.Constants.MA_SCREEN_ORIENTATION_LANDSCAPE_RIGHT;
                        break;
                    case PageOrientation.Portrait:
                        mosyncScreenOrientation = MoSync.Constants.MA_SCREEN_ORIENTATION_PORTRAIT;
                        break;
                    case PageOrientation.PortraitDown:
                        mosyncScreenOrientation = MoSync.Constants.MA_SCREEN_ORIENTATION_PORTRAIT_UPSIDE_DOWN;
                        break;
                    case PageOrientation.PortraitUp:
                        mosyncScreenOrientation = MoSync.Constants.MA_SCREEN_ORIENTATION_PORTRAIT_UP;
                        break;
                }

                // Post events handled by both NativeUI and Moblet.
                postOrientationEvent(mosyncScreenOrientation);
                postScreenOrientationEvent(mosyncScreenOrientation);
            }

            /**
             * Post orientation event to MoSync queue.
             * @param orientation The new orientation.
             */
            protected void postOrientationEvent(int orientation)
            {
                Memory eventData = new Memory(8);
                const int MAEventData_eventType = 0;
                const int MAEventData_orientation = 4;
                eventData.WriteInt32(MAEventData_eventType, MoSync.Constants.EVENT_TYPE_ORIENTATION_DID_CHANGE);
                eventData.WriteInt32(MAEventData_orientation, orientation);

                mRuntime.PostEvent(new Event(eventData));
            }

            /**
             * Post screen orientation event to MoSync queue.
             * @param orientation The new screen orientation.
             */
            protected void postScreenOrientationEvent(int orientation)
            {
                // send the event to the mosync runtime.
                Memory eventData = new Memory(12);
                const int MAWidgetEventData_eventType = 0;
                const int MAWidgetEventData_widgetHandle = 4;
                const int MAWidgetEventData_screenOrientation = 8;
                eventData.WriteInt32(MAWidgetEventData_eventType, MoSync.Constants.MAW_EVENT_SCREEN_ORIENTATION_DID_CHANGE);
                eventData.WriteInt32(MAWidgetEventData_widgetHandle, mHandle);
                eventData.WriteInt32(MAWidgetEventData_screenOrientation, orientation);

                mRuntime.PostCustomEvent(MoSync.Constants.EVENT_TYPE_WIDGET, eventData);
            }

            /*
             * The AddChild implementation.
             * @param child IWidget the "child" widget that will be added.
             *
             * Note:
             * Since the screen can have only one visible child, probably a layout,
             * each widget added as a direct child to the screen will fill the screen.
             */
            public override void AddChild(IWidget child)
            {
                base.AddChild(child);

                WidgetBaseWindowsPhone w = (WidgetBaseWindowsPhone)child;
                MoSync.Util.RunActionOnMainThreadSync(() =>
                {
                    mPage.Children.Add(w.View);
                    Grid.SetColumn((w.View as FrameworkElement), 0);
                    Grid.SetRow((w.View as FrameworkElement), 0);
                });
            }

            /**
             * The RemoveChild implementation
             * @param child IWidget the "child" widget that will be removed.
             */
            public override void RemoveChild(IWidget child)
            {
                base.RemoveChild(child);
                MoSync.Util.RunActionOnMainThreadSync(() =>
                {
                    mPage.Children.Remove((child as WidgetBaseWindowsPhone).View as FrameworkElement);
                    if (0 < mChildren.Count)
                    {
                        Grid.SetColumn((mPage.Children[mPage.Children.Count - 1] as FrameworkElement), 0);
                        Grid.SetRow((mPage.Children[mPage.Children.Count - 1] as FrameworkElement), 0);
                    }
                });
            }

            /**
             * The RemoveChild implementation.
             * @param index int the index of the "child" widget that will be removed
             */
            public override void RemoveChild(int index)
            {
                if (0 <= index && mChildren.Count > index)
                {
                    base.RemoveChild(index);
                    MoSync.Util.RunActionOnMainThreadSync(() =>
                    {
                        mPage.Children.RemoveAt(index);
                        if (0 < mChildren.Count)
                        {
                            Grid.SetColumn((mPage.Children[mPage.Children.Count - 1] as FrameworkElement), 0);
                            Grid.SetRow((mPage.Children[mPage.Children.Count - 1] as FrameworkElement), 0);
                        }
                    });
                }
            }

            /**
             * Show function implementation.
             */
            public void Show()
            {
                MoSync.Util.RunActionOnMainThreadSync(() =>
                {
                    PhoneApplicationFrame frame = (PhoneApplicationFrame)Application.Current.RootVisual;
                    //If the application bar visibility flag is set on true then the application bar is
                    //ready to be shown.
                    if (GetApplicationBarVisibility())
                    {
                        //Sets the application bar for the mainPage.xaml to our custom application bar.
                        (frame.Content as PhoneApplicationPage).ApplicationBar = mApplicationBar;
                    }
                    //Sets the content of the mainPage.xaml as our screen content.
                    (frame.Content as PhoneApplicationPage).Content = mPage;
                });
            }

            /**
             * ShowWithTansition function implementation. Shows next screen with transitions.
             *
             * @param screenTransitionType a transition type.
             */
            public void ShowWithTransition(int screenTransitionType)
            {
                MoSync.Util.RunActionOnMainThreadSync(() =>
                {
                    PhoneApplicationFrame frame = (PhoneApplicationFrame)Application.Current.RootVisual;
                    //If the application bar visibility flag is set on true then the application bar is
                    //ready to be shown.
                    if (GetApplicationBarVisibility())
                    {
                        //Sets the application bar for the mainPage.xaml to our custom application bar.
                        (frame.Content as PhoneApplicationPage).ApplicationBar = mApplicationBar;
                    }
                    mSwitchContentDelegate = delegate()
                    {
                        //Sets the content of the mainPage.xaml as our screen content.
                        (frame.Content as PhoneApplicationPage).Content = mPage;
                    };
                    MoSyncScreenTransitions.doScreenTransition(mSwitchContentDelegate, screenTransitionType);
                });
            }

            /**
             * MAW_SCREEN_TITLE property implementation.
             */
            [MoSyncWidgetProperty(MoSync.Constants.MAW_SCREEN_TITLE)]
            public String Title
            {
                set
                {
                   mTitle = value;
                }
            }

            /**
             * MAW_SCREEN_REMOVE_OPTIONS_MENU property implementation.
             */
            [MoSyncWidgetProperty(MoSync.Constants.MAW_SCREEN_REMOVE_OPTIONS_MENU)]
            public string RemoveOptionsMenu
            {
                set
                {

                    mApplicationBar = new Microsoft.Phone.Shell.ApplicationBar();

                    PhoneApplicationFrame frame = (PhoneApplicationFrame)Application.Current.RootVisual;
                    (frame.Content as PhoneApplicationPage).ApplicationBar = mApplicationBar;

                    //Sets the application bar visibility to false.
                    mApplicationBar.IsVisible = false;

                    //Sets the application bar visibility custom flag to false.
                    ApplicationBarVisible = false;

                    //removing the row definition that keeps the space for the application bar (78px).
                    mPage.RowDefinitions.RemoveAt(1);

                    //remove the button indexes and reset the seed
                    mApplicationBarItemsIndexSeed = 0;
                    mApplicationBarItemsIndexes.Clear();
                }
            }

            /*
             * Getter for the screen title.
             */
            public string getScreenTitle
            {
                get
                {
                    return mTitle;
                }
            }

            /*
             * Getter for the application bar associated to this sreen.
             */
            public Microsoft.Phone.Shell.ApplicationBar GetApplicationBar()
            {
                return mApplicationBar;
            }

            /*
             * Enables the application bar.
             */
            public void EnableApplicationBar()
            {
                MoSync.Util.RunActionOnMainThreadSync(() =>
                    {
                        //If we allready have a row definition which keeps 78px
                        //available for the application bar then only set its visibility.
                        //to true
                        if (mPage.RowDefinitions.Count < 2)
                        {
                            //Create a row definition used as spacer to keep the 78px for the application bar visibility.
                            mPage.RowDefinitions.Add(new RowDefinition { Height = new GridLength(78, GridUnitType.Pixel) });
                            //Reset the haight of the first row definition, this is where the actual content of the screen is
                            //drawn, to 1*, meaning, fill available space.
                            mPage.RowDefinitions[0].Height = new GridLength(1, GridUnitType.Star);
                        }

                        SetApplicationBarVisibility(true);
                    });
            }

            /*
             * Setter for the application bar visibility flag.
             */
            public void SetApplicationBarVisibility(bool value)
            {
                if (value != ApplicationBarVisible)
                {
                    ApplicationBarVisible = value;
                    mApplicationBar.IsVisible = value;

                    if (this.GetParent() is StackScreen)
                        (this.GetParent() as StackScreen).ToggleApplicationBar(this);
                }
            }

            /*
             * Getter for the application bar visibility flag.
             */
            public bool GetApplicationBarVisibility()
            {
                return ApplicationBarVisible;
            }

            /*
             * @brief: adds a application bar item to the dictionary member that keeps track
             *         of what it's added.
             * @param item Object the item
             *
             * @return int the asociated index
             */
            public int AddApplicationBarItemIndex(Object item)
            {
                //Associate an index to the native object.
                mApplicationBarItemsIndexes.Add(item, mApplicationBarItemsIndexSeed++);
                return (mApplicationBarItemsIndexSeed - 1);
            }

            /**
            * MAW_SCREEN_IS_SHOWN property implementation.
            */
            [MoSyncWidgetProperty(MoSync.Constants.MAW_SCREEN_IS_SHOWN)]
            public String IsShown
            {
                get
                {
                    return isScreenShown().ToString().ToLower();
                }
            }

            public bool isScreenShown()
            {
                PhoneApplicationFrame frame = (PhoneApplicationFrame)Application.Current.RootVisual;
                IWidget parentWidget = this.GetParent();
                if (parentWidget == null)
                {
                    return (frame.Content as PhoneApplicationPage).Content.Equals(mPage);
                }
                else if (parentWidget is IScreen)
                {
                    IScreen parentScreen = (IScreen)parentWidget;
                    return parentScreen.isChildShown(this);
                }
                return false;
            }

            /**
             * Check if a given child screen is shown.
             * A simple screen cannot have another screen as child.
             * @param child Given child.
             * @return false.
             */
            public virtual bool isChildShown(IScreen child)
            {
                return false;
            }
        }
    }
}
