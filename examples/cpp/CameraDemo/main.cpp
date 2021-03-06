/*
Copyright (C) 2011 MoSync AB

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
 * @file main.cpp
 * @author Ali Sarrafi
 *
 * This application provides a very basic example of how to work
 * with Native UI and the Camera API
 */

// Include MoSync syscalls.
#include <maapi.h>
#include <mastdlib.h>

// Include header file for Moblets.
#include <MAUtil/Moblet.h>

// Include NativeUI class interface
#include <NativeUI/Widgets.h>

// Include widget utility functions. These functions simplify
// getting and setting widget properties.
#include "WidgetUtil.h"

#include <conprint.h>
#include <MAUtil/String.h>
#include "SettingsScreen.h"
#include "ImageScreen.h"

#define FONT_HEIGHT_WP7 17

using namespace MAUtil;
using namespace NativeUI;

/**
 * Class that wraps a NativeUI screen widget. We implement
 * the CustomEventListener interface to be able to process
 * widget events.
 */
class AppScreen :
	public ButtonListener,
	public StackScreenListener
{
public:
	/**
	 * In the constructor, we create the user interface.
	 */
	AppScreen()
	{
		setScreenOrientation();
		initializeCamera();
		createUI();
		setCameraProperties();
	}

	/**
	 * In the destructor, we destroy the widgets to release memory.
	 * Destroying a widget automatically destroys all child widgets.
	 */
	virtual ~AppScreen()
	{
		// Destroying the screen widget destroys all of its children.
		delete mScreen;
	}

	void setScreenOrientation()
	{
		// Set screen orientations.
		int mSupportedOrientations = MA_SCREEN_ORIENTATION_PORTRAIT |
				MA_SCREEN_ORIENTATION_LANDSCAPE_RIGHT |
				MA_SCREEN_ORIENTATION_LANDSCAPE_LEFT;
		maScreenSetSupportedOrientations(mSupportedOrientations);
	}

	/**
	 * Here we create the user interface widgets.
	 */
	void createUI()
	{
		createStackScreen();
		createSettingsScreen();
		createMainScreen();
		createImageScreen();
	}

	void createStackScreen()
	{
		mStackScreen = new StackScreen();
		mStackScreen->addStackScreenListener(this);
	}

	void createSettingsScreen()
	{
		mSettingsScreen = new SettingsScreen();
		mSettingsScreen->initializeUI(mStackScreen);
	}

	void createImageScreen()
	{
		mImageScreen = new ImageScreen();
		mImageScreen->initializeUI(mStackScreen);
	}

	void createMainScreen()
	{
		// Create a Native UI screen. As the screen is a member
		// variable (also called instance variable) we have
		// prefixed the variable name with "m".
		mScreen = new Screen();

		mScreen->setTitle("Camera Demo");

		// Create the screen's main layout widget.
		mMainLayoutWidget = new VerticalLayout();

		// Make the layout fill the entire screen.
		mMainLayoutWidget->fillSpaceHorizontally();
		mMainLayoutWidget->fillSpaceVertically();

		// Add the layout as the root of the screen's widget tree.
		mScreen->setMainWidget(mMainLayoutWidget);

		//A wrapper that creates the preview widget that holds the
		//live view from the camera
		createCameraWidget();

		//Camera Control buttons are created here
		createCameraControlButtons();
	}

	/**
	 * A Wrapper function that creates the camera widget
	 * and binds it to the default camera, The binding
	 * can be changed through the settings screen
	 */
	void createCameraWidget()
	{
		mCameraPreview = new CameraPreview();

		mCameraPreview->fillSpaceHorizontally();
		mCameraPreview->fillSpaceVertically();

		mMainLayoutWidget->addChild(mCameraPreview);
	}

	/**
	 * Function for creating the camera control buttons
	 * and the layout that holds them.
	 */
	void createCameraControlButtons()
	{
		// Create buttons.
		mZoomInButton = new Button();
		mZoomInButton->setText("Zoom +");
		mZoomInButton->fillSpaceHorizontally();
		mZoomInButton->addButtonListener(this);

		mShowLastImageButton = new Button();
		mShowLastImageButton->setText("Image");
		mShowLastImageButton->fillSpaceHorizontally();
		mShowLastImageButton->addButtonListener(this);

		mSettingsButton = new Button();
		mSettingsButton->setText("Settings");
		mSettingsButton->fillSpaceHorizontally();
		mSettingsButton->addButtonListener(this);

		mZoomOutButton = new Button();
		mZoomOutButton->setText("Zoom -");
		mZoomOutButton->fillSpaceHorizontally();
		mZoomOutButton->addButtonListener(this);

		if (isWP7())
		{
			mZoomInButton->setFontSize(FONT_HEIGHT_WP7);
			mZoomOutButton->setFontSize(FONT_HEIGHT_WP7);
			mSettingsButton->setFontSize(FONT_HEIGHT_WP7);
			mShowLastImageButton->setFontSize(FONT_HEIGHT_WP7);
		}

		// Create layout to hold buttons.
		mSecondLayoutWidget = new HorizontalLayout();
		mSecondLayoutWidget->fillSpaceHorizontally();
		mSecondLayoutWidget->wrapContentVertically();

		// Make buttons wrap content vertically.
		mZoomInButton->wrapContentVertically();
		mZoomOutButton->wrapContentVertically();
		mSettingsButton->wrapContentVertically();
		mShowLastImageButton->wrapContentVertically();

		// Add buttons to the horizontal Layout.
		mSecondLayoutWidget->addChild(mZoomInButton);
		mSecondLayoutWidget->addChild(mShowLastImageButton);
		mSecondLayoutWidget->addChild(mSettingsButton);
		mSecondLayoutWidget->addChild(mZoomOutButton);

		// Then we add the button layout to its parent.
		mMainLayoutWidget->addChild(mSecondLayoutWidget);

		// We create the capture button as a larger button
		// so it is easier to be touched.
		mCaptureButton = new Button();
		mCaptureButton->setText("Take snapshot");
		mCaptureButton->fillSpaceHorizontally();
		mCaptureButton->wrapContentVertically();
		mCaptureButton->addButtonListener(this);

		// Add the capture button to the main layout so
		// that it will be larger than others.
		mMainLayoutWidget->addChild(mCaptureButton);
	}

	/**
	 * Method that makes the screen visible.
	 */
	void show()
	{
		// Display the NativeUI screen.
		// Note: This would hide any previously visible screen.
		mStackScreen->push(mScreen);
		mStackScreen->show();

		// Start the camera.
		maCameraStart();
	}

	/**
	 * Initialize the camera and camera properties.
	 */
	void initializeCamera()
	{
		// Use first camera by default.
		maCameraSelect(0);

		// Initialize variables.
		mLastImage = 0;
		mCurrentZoomIndex = 0;
	}

	/**
	 * A function to setup the camera properties whenever they are
	 * changed and we come back from the settings screen.
	 */
	void setCameraProperties()
	{
		char buffer[256];

		// Select the currently selected camera.
		maCameraSelect(mSettingsScreen->getCurrentCamera());

		// Bind preview widget to selected camera.
		mCameraPreview->bindToCurrentCamera();

		// Choose the smallest camera snapshot format.
		mCameraFormat = maCameraFormatNumber() - 1;

		// Set flash supported mode of the settings screen.
		maCameraGetProperty(MA_CAMERA_FLASH_SUPPORTED, buffer, 256);
		bool flashSupported = strcmp(buffer, "true") == 0;
		lprintfln("@@@@@ flash supported: %s", buffer);
		mSettingsScreen->setFlashSupported(flashSupported);

		// Set currently selected flash mode.
		if (flashSupported)
		{
			lprintfln("@@@@@ flash is supported");
			maCameraSetProperty(
				MA_CAMERA_FLASH_MODE,
				mSettingsScreen->getFlashMode()
				);
		}

		// Obtain max zoom level.
		maCameraGetProperty(MA_CAMERA_MAX_ZOOM, buffer, 256);
		mMaxZoom = atoi(buffer);

		// Disable the zoom buttons if zoom is not supported.
		if (mMaxZoom == 0)
		{
			mZoomInButton->setEnabled(false);
			mZoomInButton->setFontColor(0x969696);

			mZoomOutButton->setEnabled(false);
			mZoomOutButton->setFontColor(0x969696);
		}
		else // Or enable show them if it's supported
		{
			mZoomInButton->setEnabled(true);
			mZoomInButton->setFontColor(0x000000);

			mZoomOutButton->setEnabled(true);
			mZoomOutButton->setFontColor(0x000000);
		}
	}

	/**
	 * This method implements a button event listener.
	 * Button click events are sent here.
	 */
	void buttonClicked(Widget* button)
	{
		if (mCaptureButton == button)
		{
			captureButtonClicked();
		}
		else if (mSettingsButton == button)
		{
			maCameraStop();
			mSettingsScreen->pushSettingsScreen();
		}
		else if (mShowLastImageButton == button)
		{
			showImageButtonClicked();
		}
		else if (mZoomInButton == button)
		{
			//Increase the zoom level if it is more
			//than the maximum supported zoom
			if (mCurrentZoomIndex < mMaxZoom)
			{
				mCurrentZoomIndex++;
			}
			char buffer[256];
			sprintf(buffer, "%i", mCurrentZoomIndex);
			maCameraSetProperty(MA_CAMERA_ZOOM, buffer);

		}
		else if (mZoomOutButton == button)
		{
			//Decrease the zoom index if it is more than 0
			if (mCurrentZoomIndex > 0)
			{
				mCurrentZoomIndex--;
			}
			char buffer[256];
			sprintf(buffer, "%i", mCurrentZoomIndex);
			maCameraSetProperty(MA_CAMERA_ZOOM, buffer);
		}
	}

	/**
	 * This method implements a stack screen event listener.
	 * Pop events are sent here.
	 */
	void stackScreenScreenPopped(
		StackScreen* stackScreen,
		Screen* fromScreen,
		Screen* toScreen)
	{
		if (toScreen == mScreen)
		{
			setCameraProperties();
			maCameraStart();
		}
	}

	/**
	 * This method is called when the Clear button is clicked.
	 */
	void settingsButtonClicked()
	{
		mSettingsScreen->pushSettingsScreen();
	}

	/**
	 * This method is called when the Submit button is clicked.
	 */
	void captureButtonClicked()
	{
		if (mLastImage != 0)
		{
			maDestroyPlaceholder(mLastImage);
		}

		mLastImage = maCreatePlaceholder();

		int snapshotResult = maCameraSnapshot(mCameraFormat, mLastImage);
		if (snapshotResult < 0)
		{
			// An error occurred while taking the snapshot.
			maDestroyPlaceholder(mLastImage);
			mLastImage = 0;
		}

		// Restart camera.
		maCameraStop();
		setCameraProperties();
		maCameraStart();
	}

	/**
	 * This method is called when the Submit button is clicked.
	 */
	void showImageButtonClicked()
	{
		if (mLastImage == 0)
		{
			// Do nothing when there is no image taken.
			return;
		}
		maCameraStop();
		mImageScreen->setImageDataHandle(mLastImage);
		mImageScreen->pushImageScreen();
	}

	/**
	 * This function verifies if the platform is Windows Phone 7
	 * @return true if we are running on WP7 otherwise false
	 */
	bool isWP7()
	{
		char platform[64];
		maGetSystemProperty("mosync.device.OS", platform, 20);
		return
			strcmp(platform, "iPhone OS") != 0 &&
			strcmp(platform, "Android") != 0;
	}

private:
	/**
	 * The Settings screen class that
	 * creates and handles the settings view.
	 */
	SettingsScreen* mSettingsScreen;

	/**
	 * The Image screen class that
	 * creates and handles the Image view.
	 */
	ImageScreen* mImageScreen;

	/**
	 * Stack Screen used to handle screen changes.
	 */
	StackScreen* mStackScreen;

	/**
	 * The screen widget.
	 */
	Screen* mScreen;

	/**
	 * The main layout that holds the other widgets.
	 */
	VerticalLayout* mMainLayoutWidget;

	/**
	 * The main layout that holds the other widgets.
	 */
	HorizontalLayout* mSecondLayoutWidget;

	/**
	 * Text editor box for user input.
	 */
	CameraPreview* mCameraPreview;

	/**
	 * The Settings button.
	 */
	Button* mSettingsButton;

	/**
	 * The Show Image button.
	 */
	Button* mShowLastImageButton;

	/**
	 * The Capture button.
	 */
	Button* mCaptureButton;

	/**
	 * The Zoom In button.
	 */
	Button* mZoomInButton;

	/**
	 * The Zoom Out button.
	 */
	Button* mZoomOutButton;

	/**
	 * Placeholder used for keeping the last image.
	 */
	MAHandle mLastImage;

	/**
	 * Index of the current zoom level.
	 */
	int mCurrentZoomIndex;

	/**
	 * Maximum zoom supported by the camera.
	 */
	int mMaxZoom;

	/**
	 * Size of the taken picture.
	 */
	int mCameraFormat;
};

// That's the screen class finished, now we move on to the Moblet that
// is the main object in the application.

/**
 * A Moblet is the main object of MoSync application. In the Moblet
 * we manage the application and handle events. Our Moblet inherits
 * the Moblet base class in the MAUtil library.
 */
class CameraDemoMoblet :
	public Moblet,
	public FocusListener
{
public:
	/**
	 * The user interface is created in the constructor method.
	 */
	CameraDemoMoblet()
	{
		// Create the main (and only) screen used in the application.
		mAppScreen = new AppScreen();

		// Display the screen.
		mAppScreen->show();

		// Register to get focus notifications.
		addFocusListener(this);
	}

	/**
	 * The destructor deletes the screen object.
	 */
	virtual ~CameraDemoMoblet()
	{
		delete mAppScreen;
	}

	/**
	 * This method implements a focus listener.
	 * We stop the camera when focus is lost.
	 */
	void focusLost()
	{
		maCameraStop();
	}

	/**
	 * This method implements a focus listener.
	 * We start the camera when focus is gained.
	 */
	void focusGained()
	{
		maCameraStart();
	}

	/**
	 * This method is called when a key is pressed. The method
	 * is inherited from the Moblet class, and is overridden here.
	 */
	void keyPressEvent(int keyCode, int nativeCode)
	{
		// Close the application if the back key is pressed.
		if (MAK_BACK == keyCode)
		{
			// Call close to exit the application.
			close();
		}
	}

private:
	/**
	 * The screen of our application.
	 */
	AppScreen* mAppScreen;
};

/**
 * Main function that is called when the program starts.
 */
extern "C" int MAMain()
{
	// Create and run the Moblet to start the application.
	MAUtil::Moblet::run(new CameraDemoMoblet());

	// The Moblet will run until it is closed by the user.
	// When it is closed we end our program in a well-behaved
	// way, returning zero.
	return 0;
}
