/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/dialog.h>

class AboutDialog : public wxDialog
{
public:

    AboutDialog(wxWindow* parent);
	
	virtual ~AboutDialog();

private:

	void OnClose(wxCloseEvent& event);

private:

	wxWindow * const mParent;

	DECLARE_EVENT_TABLE()
};
