/***************************************************************************************
 * Original Author:		Luke Wren (wren6991@gmail.com)
 * Created:				2013-04-30
 * Modified By:			Gabriele Giuseppini
 * Copyright:			Luke Wren (http://github.com/Wren6991), 
 *						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

//
// The main application. Everything begins from here.
//

#include "GameController.h"
#include "MainFrame.h"

#include <IL/il.h>
#include <IL/ilu.h>

#include <wx/app.h>

class MainApp : public wxApp
{
public:
	virtual bool OnInit() override;
};

IMPLEMENT_APP(MainApp);

bool MainApp::OnInit()
{
	ilInit();
	iluInit();

	wxInitAllImageHandlers();

	// Create Game controller
	// TODO: splash screen and game controller progress
	auto gameController = GameController::Create();

	MainFrame* frame = new MainFrame(std::move(gameController));
	frame->Show();
	SetTopWindow(frame);
	
	return true;
}