#include <shlobj.h>
#include <shlwapi.h>
#include "LDViewPreferences.h"
#include <LDLib/LDrawModelViewer.h>
#include <LDLoader/LDLPalette.h>
#include <TCFoundation/TCMacros.h>
#include "LDVExtensionsSetup.h"
#include "AppResources.h"
#include "UserDefaultsKeys.h"
#include "ModelWindow.h"

#include <TCFoundation/TCUserDefaults.h>
#include <TCFoundation/mystring.h>
#include <TCFoundation/TCStringArray.h>
#include <Commctrl.h>
#include <stdio.h>

#ifndef IDC_HARDWARE_STEREO
#define IDC_HARDWARE_STEREO 1030
#endif

#if (_WIN32_WINNT < 0x0500)
#define ODS_NOFOCUSRECT 0x0200
#endif // (_WIN32_WINNT < 0x0500)

#define DEFAULT_PREF_SET "<Default>"

char LDViewPreferences::ldviewPath[MAX_PATH] = "";

LDViewPreferences::LDViewPreferences(HINSTANCE hInstance,
									 LDrawModelViewer* modelViewer)
	:CUIPropertySheet("LDView Preferences", hInstance),
	modelViewer(modelViewer ? ((LDrawModelViewer*)modelViewer->retain()) :
		NULL),
	generalPageNumber(0),
	geometryPageNumber(1),
	effectsPageNumber(2),
	primitivesPageNumber(3),
	prefSetsPageNumber(4),
	hGeneralPage(NULL),
	hBackgroundColorBitmap(NULL),
	hBackgroundColorButton(NULL),
	hDefaultColorBitmap(NULL),
	hDefaultColorButton(NULL),
	hDefaultColorTheme(NULL),
	hMouseOverButton(NULL),
	previousButtonWindowProc(NULL),
	hButtonColorDC(NULL),
	hBackgroundColorTheme(NULL),
	hGeometryPage(NULL),
	hEffectsPage(NULL),
	hPrimitivesPage(NULL),
	hPrefSetsPage(NULL),
	setActiveWarned(false),
	checkAbandon(true)
{
	CUIThemes::init();
	loadSettings();
	applySettings();
}

LDViewPreferences::~LDViewPreferences(void)
{
}

void LDViewPreferences::dealloc(void)
{
	if (modelViewer)
	{
		modelViewer->release();
	}
	if (hBackgroundColorTheme)
	{
		CUIThemes::closeThemeData(hBackgroundColorTheme);
		hBackgroundColorTheme = NULL;
	}
	if (hDefaultColorTheme)
	{
		CUIThemes::closeThemeData(hDefaultColorTheme);
		hDefaultColorTheme = NULL;
	}
	CUIPropertySheet::dealloc();
}

void LDViewPreferences::applySettings(void)
{
	applyGeneralSettings();
	applyGeometrySettings();
	applyEffectsSettings();
	applyPrimitivesSettings();
	if (modelViewer)
	{
		modelViewer->setZoomMax(zoomMax);
		modelViewer->setDistanceMultiplier(1.0f / defaultZoom);
		setupDefaultRotationMatrix();
	}
}

void LDViewPreferences::applyGeneralSettings(void)
{
	if (modelViewer)
	{
		int r, g, b;

		// FSAA taken care of automatically.
		getRGB(backgroundColor, r, g, b);
//		modelViewer->setBackgroundRGBA(r, g, b, 255);
		modelViewer->setBackgroundRGBA(r, g, b, 0);
		getRGB(defaultColor, r, g, b);
		modelViewer->setProcessLDConfig(processLDConfig);
		// showFrameRate taken care of automatically.
		// showErrors taken care of automatically.
		// fullScreenRefresh taken care of automatically.
		modelViewer->setFov(fov);
		modelViewer->setDefaultRGB((BYTE)r, (BYTE)g, (BYTE)b);
		modelViewer->setDefaultColorNumber(defaultColorNumber);
		modelViewer->setLineSmoothing(lineSmoothing);
	}
}

void LDViewPreferences::applyGeometrySettings(void)
{
	if (modelViewer)
	{
		if (useSeams)
		{
			modelViewer->setSeamWidth(seamWidth / 100.0f);
		}
		else
		{
			modelViewer->setSeamWidth(0.0f);
		}
		modelViewer->setDrawWireframe(drawWireframe);
		modelViewer->setUseWireframeFog(useWireframeFog);
		modelViewer->setRemoveHiddenLines(removeHiddenLines);
		modelViewer->setWireframeLineWidth((GLfloat)wireframeThickness);
		modelViewer->setShowsHighlightLines(showsHighlightLines);
		modelViewer->setEdgesOnly(edgesOnly);
		modelViewer->setDrawConditionalHighlights(drawConditionalHighlights);
		modelViewer->setShowAllConditionalLines(showAllConditionalLines);
		modelViewer->setShowConditionalControlPoints(
			showConditionalControlPoints);
		modelViewer->setUsePolygonOffset(usePolygonOffset);
		modelViewer->setBlackHighlights(blackHighlights);
		modelViewer->setHighlightLineWidth((GLfloat)edgeThickness);
	}
}

void LDViewPreferences::applyEffectsSettings(void)
{
	if (modelViewer)
	{
		modelViewer->setUseLighting(useLighting);
		modelViewer->setQualityLighting(qualityLighting);
		modelViewer->setSubduedLighting(subduedLighting);
		modelViewer->setUsesSpecular(usesSpecular);
		modelViewer->setOneLight(oneLight);
		modelViewer->setStereoMode(stereoMode);
		modelViewer->setStereoEyeSpacing((GLfloat)stereoEyeSpacing);
		modelViewer->setCutawayMode(cutawayMode);
		modelViewer->setCutawayAlpha((float)cutawayAlpha / 100.0f);
		modelViewer->setCutawayLineWidth((float)cutawayThickness);
		modelViewer->setSortTransparent(sortTransparent);
		modelViewer->setUseStipple(useStipple);
		modelViewer->setUsesFlatShading(usesFlatShading);
		modelViewer->setPerformSmoothing(performSmoothing);
	}
}

void LDViewPreferences::applyPrimitivesSettings(void)
{
	if (modelViewer)
	{
		modelViewer->setAllowPrimitiveSubstitution(allowPrimitiveSubstitution);
		modelViewer->setTextureStuds(textureStuds);
		modelViewer->setTextureFilterType(textureFilterType);
		modelViewer->setCurveQuality(curveQuality);
		modelViewer->setQualityStuds(qualityStuds);
		modelViewer->setHiResPrimitives(hiResPrimitives);
	}
}

void LDViewPreferences::loadSettings(void)
{
	loadGeneralSettings();
	loadGeometrySettings();
	loadEffectsSettings();
	loadPrimitivesSettings();

	zoomMax = TCUserDefaults::longForKey(ZOOM_MAX_KEY, 199, false) / 100.0f;
	defaultZoom = TCUserDefaults::floatForKey(DEFAULT_ZOOM_KEY, 1.0f);
}

void LDViewPreferences::loadDefaultGeneralSettings(void)
{
	int i;

	fsaaMode = 0;
	lineSmoothing = false;
	backgroundColor = (COLORREF)0;
	defaultColor = (COLORREF)0x999999;
	defaultColorNumber = -1;
	processLDConfig = true;
	showsFPS = false;
	showErrors = true;
	fullScreenRefresh = 0;
	fov = 45.0f;
	for (i = 0; i < 16; i++)
	{
		int r, g, b, a;

		LDLPalette::getDefaultRGBA(i, r, g, b, a);
		// Windows XP doesn't like the upper bits to be set, so mask those out.
		customColors[i] = LDLPalette::colorForRGBA(r, g, b, a) & 0xFFFFFF;
	}
}

void LDViewPreferences::loadDefaultGeometrySettings(void)
{
	useSeams = -1;
	seamWidth = 50;
	drawWireframe = false;
	useWireframeFog = false;
	removeHiddenLines = false;
	wireframeThickness = 1;
	showsHighlightLines = false;
	edgesOnly = false;
	drawConditionalHighlights = false;
	showAllConditionalLines = false;
	showConditionalControlPoints = false;
	usePolygonOffset = true;
	blackHighlights = false;
	edgeThickness = 1;
}

void LDViewPreferences::loadDefaultEffectsSettings(void)
{
	useLighting = true;
	qualityLighting = false;
	subduedLighting = false;
	usesSpecular = true;
	oneLight = false;
	stereoMode = LDVStereoNone;
	stereoEyeSpacing = 50;
	cutawayMode = LDVCutawayNormal;
	cutawayAlpha = 100;
	cutawayThickness = 1;
	sortTransparent = true;
	useStipple = false;
	usesFlatShading = false;
	performSmoothing = true;
}

void LDViewPreferences::loadDefaultPrimitivesSettings(void)
{
	allowPrimitiveSubstitution = true;
	textureStuds = true;
	textureFilterType = GL_LINEAR_MIPMAP_LINEAR;
	curveQuality = 2;
	qualityStuds = false;
	hiResPrimitives = false;
}

void LDViewPreferences::loadGeneralSettings(void)
{
	int i;

	loadDefaultGeneralSettings();
	fsaaMode = TCUserDefaults::longForKey(FSAA_MODE_KEY, (long)fsaaMode);
	lineSmoothing = TCUserDefaults::longForKey(LINE_SMOOTHING_KEY,
		lineSmoothing) != 0;
	backgroundColor =
		(COLORREF)TCUserDefaults::longForKey(BACKGROUND_COLOR_KEY,
		(long)backgroundColor);
	defaultColor = (COLORREF)TCUserDefaults::longForKey(DEFAULT_COLOR_KEY,
		(long)defaultColor);
	defaultColorNumber = TCUserDefaults::longForKey(DEFAULT_COLOR_NUMBER_KEY,
		defaultColorNumber);
	processLDConfig = TCUserDefaults::longForKey(PROCESS_LDCONFIG_KEY,
		processLDConfig) != 0;
	showsFPS = TCUserDefaults::longForKey(SHOW_FPS_KEY, (long)showsFPS) != 0;
	showErrors = TCUserDefaults::longForKey(SHOW_ERRORS_KEY, (long)showErrors,
		false) != 0;
	fullScreenRefresh = TCUserDefaults::longForKey(FULLSCREEN_REFRESH_KEY,
		fullScreenRefresh);
	fov = TCUserDefaults::floatForKey(FOV_KEY, fov);
	for (i = 0; i < 16; i++)
	{
		char key[128];

		sprintf(key, "%s/Color%02d", CUSTOM_COLORS_KEY, i);
		// Windows XP doesn't like the upper bits to be set, so mask those out.
		customColors[i] = TCUserDefaults::longForKey(key, customColors[i],
			false) & 0xFFFFFF;
	}
}

void LDViewPreferences::loadGeometrySettings(void)
{
	loadDefaultGeometrySettings();
	useSeams = TCUserDefaults::longForKey(SEAMS_KEY, useSeams);
	seamWidth = TCUserDefaults::longForKey(SEAM_WIDTH_KEY, seamWidth);
	if (useSeams == -1)
	{
		if (seamWidth)
		{
			useSeams = 1;
		}
		else
		{
			useSeams = 0;
		}
	}
	drawWireframe = TCUserDefaults::longForKey(WIREFRAME_KEY,
		(long)drawWireframe) != 0;
	useWireframeFog = TCUserDefaults::longForKey(WIREFRAME_FOG_KEY,
		(long)useWireframeFog) != 0;
	removeHiddenLines = TCUserDefaults::longForKey(REMOVE_HIDDEN_LINES_KEY,
		(long)removeHiddenLines) != 0;
	wireframeThickness = TCUserDefaults::longForKey(WIREFRAME_THICKNESS_KEY,
		wireframeThickness);
	showsHighlightLines = TCUserDefaults::longForKey(SHOWS_HIGHLIGHT_LINES_KEY,
		(long)showsHighlightLines) != 0;
	edgesOnly = TCUserDefaults::longForKey(EDGES_ONLY_KEY, (long)edgesOnly)
		!= 0;
	drawConditionalHighlights =
		TCUserDefaults::longForKey(CONDITIONAL_HIGHLIGHTS_KEY,
		(long)drawConditionalHighlights) != 0;
	showAllConditionalLines = TCUserDefaults::longForKey(SHOW_ALL_TYPE5_KEY, 0,
		(long)showAllConditionalLines) != 0;
	showConditionalControlPoints =
		TCUserDefaults::longForKey(SHOW_TYPE5_CONTROL_POINTS_KEY, 0,
		(long)showConditionalControlPoints)
		!= 0;
	usePolygonOffset = TCUserDefaults::longForKey(POLYGON_OFFSET_KEY,
		(long)usePolygonOffset) != 0;
	blackHighlights = TCUserDefaults::longForKey(BLACK_HIGHLIGHTS_KEY,
		(long)blackHighlights) != 0;
	edgeThickness = TCUserDefaults::longForKey(EDGE_THICKNESS_KEY,
		edgeThickness);
}

void LDViewPreferences::loadEffectsSettings(void)
{
	loadDefaultEffectsSettings();
	useLighting = TCUserDefaults::longForKey(LIGHTING_KEY, (long)useLighting)
		!= 0;
	qualityLighting = TCUserDefaults::longForKey(QUALITY_LIGHTING_KEY,
		(long)qualityLighting) != 0;
	subduedLighting = TCUserDefaults::longForKey(SUBDUED_LIGHTING_KEY,
		(long)subduedLighting) != 0;
	usesSpecular = TCUserDefaults::longForKey(SPECULAR_KEY, (long)usesSpecular)
		!= 0;
	oneLight = TCUserDefaults::longForKey(ONE_LIGHT_KEY, (long)oneLight) != 0;
	stereoMode = (LDVStereoMode)TCUserDefaults::longForKey(STEREO_MODE_KEY,
		(long)stereoMode);
	stereoEyeSpacing = TCUserDefaults::longForKey(STEREO_SPACING_KEY,
		stereoEyeSpacing);
	cutawayMode = (LDVCutawayMode)TCUserDefaults::longForKey(CUTAWAY_MODE_KEY,
		(long)cutawayMode);
	cutawayAlpha = TCUserDefaults::longForKey(CUTAWAY_ALPHA_KEY, cutawayAlpha);
	cutawayThickness = TCUserDefaults::longForKey(CUTAWAY_THICKNESS_KEY,
		cutawayThickness);
	sortTransparent = TCUserDefaults::longForKey(SORT_KEY,
		(long)sortTransparent) != 0;
	useStipple = TCUserDefaults::longForKey(STIPPLE_KEY, (long)useStipple) != 0;
	usesFlatShading = TCUserDefaults::longForKey(FLAT_SHADING_KEY,
		(long)usesFlatShading) != 0;
	performSmoothing = TCUserDefaults::longForKey(PERFORM_SMOOTHING_KEY,
		(long)performSmoothing) != 0;
}

void LDViewPreferences::loadPrimitivesSettings(void)
{
	loadDefaultPrimitivesSettings();
	allowPrimitiveSubstitution = TCUserDefaults::longForKey(
		PRIMITIVE_SUBSTITUTION_KEY, (long)allowPrimitiveSubstitution) != 0;
	textureStuds = TCUserDefaults::longForKey(TEXTURE_STUDS_KEY,
		(long)textureStuds) != 0;
	textureFilterType = TCUserDefaults::longForKey(TEXTURE_FILTER_TYPE_KEY,
		textureFilterType);
	curveQuality = TCUserDefaults::longForKey(CURVE_QUALITY_KEY, curveQuality);
	qualityStuds = TCUserDefaults::longForKey(QUALITY_STUDS_KEY,
		(long)qualityStuds) != 0;
	hiResPrimitives = TCUserDefaults::longForKey(HI_RES_PRIMITIVES_KEY,
		(long)hiResPrimitives) != 0;
}

int LDViewPreferences::run(void)
{
	bool wasPaused = true;
	int retValue;

	if (modelViewer)
	{
		wasPaused = modelViewer->getPaused() != 0;
		if (!wasPaused)
		{
			modelViewer->pause();
		}
	}
	addPage(IDD_GENERAL_PREFS);
	addPage(IDD_GEOMETRY_PREFS); 
	addPage(IDD_EFFECTS_PREFS);
	addPage(IDD_PRIMITIVES_PREFS);
	addPage(IDD_PREFSETS_PREFS);
	checkAbandon = true;
	retValue = CUIPropertySheet::run();
	if (!wasPaused)
	{
		modelViewer->unpause();
	}
	return retValue;
}

BOOL LDViewPreferences::doDialogNotify(HWND hDlg, int controlId,
									   LPNMHDR notification)
{
//	debugPrintf("LDViewPreferences::doDialogNotify: %d 0x%08X\n",
//		notification->code, notification->code);
	if (notification->code == NM_RELEASEDCAPTURE)
	{
		if (hDlg == hEffectsPage)
		{
			if (controlId == IDC_STEREO_SPACING ||
				controlId == IDC_CUTAWAY_OPACITY ||
				controlId == IDC_CUTAWAY_THICKNESS)
			{
				enableApply(hEffectsPage);
				return FALSE;
			}
		}
		else if (hDlg == hGeometryPage)
		{
			if (controlId == IDC_WIREFRAME_THICKNESS ||
				controlId == IDC_EDGE_THICKNESS)
			{
				enableApply(hGeometryPage);
				return FALSE;
			}
		}
		else if (hDlg == hPrimitivesPage)
		{
			if (controlId == IDC_CURVE_QUALITY)
			{
				enableApply(hPrimitivesPage);
				return FALSE;
			}
		}
	}
	return CUIPropertySheet::doDialogNotify(hDlg, controlId, notification);
}

char* LDViewPreferences::getLDViewPath(bool useQuotes)
{
	char tmpPath[MAX_PATH];

	if (!ldviewPath[0])
	{
		char *commandLine = copyString(GetCommandLine());

		PathRemoveArgs(commandLine);
		PathUnquoteSpaces(commandLine);
		PathRemoveFileSpec(commandLine);
		GetCurrentDirectory(MAX_PATH, tmpPath);
		if (SetCurrentDirectory(commandLine))
		{
			GetCurrentDirectory(MAX_PATH, ldviewPath);
			PathUnquoteSpaces(ldviewPath);
			SetCurrentDirectory(tmpPath);
		}
		else
		{
			strcpy(ldviewPath, tmpPath);
		}
		delete commandLine;
	}
	strcpy(tmpPath, ldviewPath);
	if (useQuotes)
	{
		PathQuoteSpaces(tmpPath);
	}
	return copyString(tmpPath);
}

char* LDViewPreferences::getLDViewPath(char* helpFilename, bool useQuotes)
{
	char *programPath = getLDViewPath();
	char tmpPath[MAX_PATH];

	strcpy(tmpPath, programPath);
	delete programPath;
	if (helpFilename)
	{
		PathAppend(tmpPath, helpFilename);
	}
	if (useQuotes)
	{
		PathQuoteSpaces(tmpPath);
	}
	return copyString(tmpPath);
}

DWORD LDViewPreferences::getPageDialogID(HWND hDlg)
{
	if (hDlg == hGeneralPage)
	{
		return IDD_GENERAL_PREFS;
	}
	else if (hDlg == hGeometryPage)
	{
		return IDD_GEOMETRY_PREFS;
	}
	else if (hDlg == hEffectsPage)
	{
		return IDD_EFFECTS_PREFS;
	}
	else if (hDlg == hPrimitivesPage)
	{
		return IDD_PRIMITIVES_PREFS;
	}
	else if (hDlg == hPrefSetsPage)
	{
		return IDD_PREFSETS_PREFS;
	}
	return 0;
}

BOOL LDViewPreferences::doDialogHelp(HWND hDlg, LPHELPINFO helpInfo)
{
	DWORD dialogId = getPageDialogID(hDlg);
	BOOL retValue = FALSE;

	if (dialogId)
	{
		char* helpPath = getLDViewPath("LDView.hlp");
		DWORD helpId;

		helpId = 0x80000000 | (dialogId << 16) | (DWORD)helpInfo->iCtrlId;
		WinHelp((HWND)helpInfo->hItemHandle, helpPath, HELP_CONTEXTPOPUP,
			helpId);
		retValue = TRUE;
		delete helpPath;
	}
	return retValue;
}

char *LDViewPreferences::getPrefSet(int index)
{
	int len = SendMessage(hPrefSetsList, LB_GETTEXTLEN, index, 0);
	char *prefSet = new char[len + 1];

	SendMessage(hPrefSetsList, LB_GETTEXT, index, (LPARAM)prefSet);
	return prefSet;
}

char *LDViewPreferences::getSelectedPrefSet(void)
{
	int selectedIndex = SendMessage(hPrefSetsList, LB_GETCURSEL, 0, 0);

	if (selectedIndex != LB_ERR)
	{
		return getPrefSet(selectedIndex);
	}
	return NULL;
}

void LDViewPreferences::abandonChanges(void)
{
	int i;
	HWND hPage;

	doReset();
	for (i = 0; i < prefSetsPageNumber; i++)
	{
		hPage = hwndArray->pointerAtIndex(i);
		if (hPage)
		{
			disableApply(hPage);
			hwndArray->replacePointer(NULL, i);
		}
	}
	hPage = hwndArray->pointerAtIndex(prefSetsPageNumber);
	if (hPage)
	{
		disableApply(hPage);
	}
}

int LDViewPreferences::runPrefSetApplyDialog(void)
{
	int retValue = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PREFSET_APPLY),
		hWindow, staticDialogProc, (LPARAM)this);

	if (retValue == IDC_APPLY)
	{
		PropSheet_Apply(hPropSheet);
	}
	else if (retValue == IDC_ABANDON)
	{
		abandonChanges();
	}
	return retValue;
}

BOOL LDViewPreferences::doPrefSetSelected(bool force)
{
	char *selectedPrefSet = getSelectedPrefSet();
	bool needToReselect = false;

	if (checkAbandon && getApplyEnabled() && !force)
	{
		char *savedSession =
			TCUserDefaults::getSavedSessionNameFromKey(PREFERENCE_SET_KEY);

		if (!savedSession || !savedSession[0])
		{
			delete savedSession;
			savedSession = copyString(DEFAULT_PREF_SET);
		}
		if (strcmp(savedSession, selectedPrefSet) != 0)
		{
			needToReselect = true;
			selectPrefSet(NULL, true);
			if (runPrefSetApplyDialog() == IDCANCEL)
			{
				delete savedSession;
				return TRUE;
			}
		}
		delete savedSession;
	}
	if (selectedPrefSet)
	{
		BOOL enabled = TRUE;

		if (needToReselect)
		{
			selectPrefSet(selectedPrefSet);
		}
		if (strcmp(selectedPrefSet, DEFAULT_PREF_SET) == 0)
		{
			enabled = FALSE;
		}
		EnableWindow(hDeletePrefSetButton, enabled);
		delete selectedPrefSet;
	}
	enableApply(hPrefSetsPage);
	checkAbandon = false;
	return FALSE;
}

BOOL LDViewPreferences::doDialogThemeChanged(void)
{
	if (hBackgroundColorTheme)
	{
		CUIThemes::closeThemeData(hBackgroundColorTheme);
		hBackgroundColorTheme = NULL;
	}
	if (hDefaultColorTheme)
	{
		CUIThemes::closeThemeData(hDefaultColorTheme);
		hDefaultColorTheme = NULL;
	}
	if (CUIThemes::isThemeLibLoaded())
	{
		if (hBackgroundColorButton)
		{
			setupBackgroundColorButton();
//			hBackgroundColorTheme =
//				CUIThemes::openThemeData(hBackgroundColorButton, L"Button");
		}
		if (hDefaultColorButton)
		{
			setupDefaultColorButton();
//			hDefaultColorTheme =
//				CUIThemes::openThemeData(hDefaultColorButton, L"Button");
		}
	}
	return FALSE;
}

BOOL LDViewPreferences::doDialogCommand(HWND hDlg, int controlId,
										int notifyCode, HWND controlHWnd)
{
	char className[1024];

	GetClassName(controlHWnd, className, sizeof(className));
	if (strcmp(className, "ComboBox") == 0)
	{
		if (notifyCode == CBN_SELCHANGE)
		{
			return doComboSelChange(hDlg, controlId, controlHWnd);
		}
	}
	else if (notifyCode == BN_CLICKED)
	{
		return doClick(hDlg, controlId, controlHWnd);
	}
	else if (notifyCode == EN_CHANGE)
	{
		if (controlId == IDC_FS_RATE || controlId == IDC_FOV)
		{
			enableApply(hDlg);
			return 0;
		}
	}
	else if (notifyCode == LBN_SELCHANGE)
	{
		if (hDlg == hPrefSetsPage)
		{
			return doPrefSetSelected();
		}
	}
	return LDP_UNKNOWN_COMMAND;
}

void LDViewPreferences::setupBackgroundColorButton(void)
{
	setupColorButton(hGeneralPage, hBackgroundColorButton,
		IDC_BACKGROUND_COLOR, hBackgroundColorBitmap, backgroundColor,
		hBackgroundColorTheme);
}

void LDViewPreferences::setupDefaultColorButton(void)
{
	setupColorButton(hGeneralPage, hDefaultColorButton,
		IDC_DEFAULT_COLOR, hDefaultColorBitmap, defaultColor,
		hDefaultColorTheme);
}

LRESULT CALLBACK LDViewPreferences::staticColorButtonProc(HWND hWnd,
														  UINT message,
														  WPARAM wParam,
														  LPARAM lParam)
{
	LDViewPreferences *thisPtr =
		(LDViewPreferences *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	return thisPtr->colorButtonProc(hWnd, message, wParam, lParam);
}

LRESULT LDViewPreferences::colorButtonProc(HWND hWnd, UINT message,
										   WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_MOUSEMOVE:
		if (hMouseOverButton != hWnd)
		{
			if (hMouseOverButton)
			{
				InvalidateRect(hMouseOverButton, NULL, FALSE);
			}
			hMouseOverButton = hWnd;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	}
	return CallWindowProc((WNDPROC)previousButtonWindowProc, hWnd, message,
		wParam, lParam);
}

void LDViewPreferences::setupColorButton(HWND hPage, HWND &hColorButton,
										 int controlID, HBITMAP &hButtonBitmap,
										 COLORREF color, HTHEME &hTheme)
{
	int imageWidth;
	int imageHeight;
	HDC hdc;
	RECT clientRect;

	if (hButtonBitmap)
	{
		DeleteObject(hButtonBitmap);
		if (hTheme)
		{
			CUIThemes::closeThemeData(hTheme);
			hTheme = NULL;
			if (previousButtonWindowProc)
			{
				SetWindowLongPtr(hColorButton, GWLP_WNDPROC,
					(LONG)previousButtonWindowProc);
			}
		}
//		redrawColorBitmap(hColorButton, hButtonBitmap, color, hTheme);
//		return;
	}
	hColorButton = GetDlgItem(hPage, controlID);
	GetClientRect(hColorButton, &clientRect);
	if (CUIThemes::isThemeLibLoaded())
	{
		hTheme = CUIThemes::openThemeData(hColorButton, L"Button");
	}
	if (hTheme)
	{
		RECT contentRect;

		// subclass the Wizard button so we can know when the mouse has moved
		// over it
		SetWindowLongPtr(hColorButton, GWLP_USERDATA, (LONG)this);
		previousButtonWindowProc = SetWindowLongPtr(hColorButton, GWLP_WNDPROC,
			(LONG)staticColorButtonProc);
		CUIThemes::getThemeBackgroundContentRect(hTheme, NULL, BP_PUSHBUTTON,
			PBS_HOT, &clientRect, &contentRect);
		imageWidth = contentRect.right - contentRect.left - 6;
		imageHeight = contentRect.bottom - contentRect.top - 6;

	}
	else
	{
		imageWidth = clientRect.right - clientRect.left - 10;
		imageHeight = clientRect.bottom - clientRect.top - 10;
	}
	hdc = GetDC(hPage);
	if (!hButtonColorDC)
	{
		hButtonColorDC = CreateCompatibleDC(hdc);
	}
	hButtonBitmap = CreateCompatibleBitmap(hdc, imageWidth, imageHeight);
	ReleaseDC(hPage, hdc);
	SetBitmapDimensionEx(hButtonBitmap, imageWidth, imageHeight, NULL);
	redrawColorBitmap(hColorButton, hButtonBitmap, color, hTheme);
	SendDlgItemMessage(hPage, controlID, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
		(LPARAM)hButtonBitmap);
}

void LDViewPreferences::enableSeams(void)
{
	char seamWidthString[128];

	EnableWindow(hSeamSpin, TRUE);
	SendDlgItemMessage(hGeometryPage, IDC_SEAMS, BM_SETCHECK, 1, 0);
	sprintf(seamWidthString, "%0.2f", seamWidth / 100.0f);
	SendDlgItemMessage(hGeometryPage, IDC_SEAM_WIDTH_FIELD, WM_SETTEXT, 0,
		(LPARAM)seamWidthString);
}

void LDViewPreferences::disableSeams(void)
{
	EnableWindow(hSeamSpin, FALSE);
	SendDlgItemMessage(hGeometryPage, IDC_SEAMS, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hGeometryPage, IDC_SEAM_WIDTH_FIELD, WM_SETTEXT, 0,
		(LPARAM)"");
}

void LDViewPreferences::enableTextureFiltering(void)
{
	int activeTextureFilter;

	EnableWindow(hTextureNearestButton, TRUE);
	EnableWindow(hTextureBilinearButton, TRUE);
	EnableWindow(hTextureTrilinearButton, TRUE);
	SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_NEAREST, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_BILINEAR, BM_SETCHECK, 0,
		0);
	SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_TRILINEAR, BM_SETCHECK, 0,
		0);
	switch (textureFilterType)
	{
	case GL_NEAREST_MIPMAP_NEAREST:
		activeTextureFilter = IDC_TEXTURE_NEAREST;
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
		activeTextureFilter = IDC_TEXTURE_BILINEAR;
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		activeTextureFilter = IDC_TEXTURE_TRILINEAR;
		break;
	default:
		textureFilterType = GL_LINEAR_MIPMAP_LINEAR;
		activeTextureFilter = IDC_TEXTURE_TRILINEAR;
		break;
	}
	SendDlgItemMessage(hPrimitivesPage, activeTextureFilter, BM_SETCHECK, 1, 0);
}

void LDViewPreferences::disableTextureFiltering(void)
{
	EnableWindow(hTextureNearestButton, FALSE);
	EnableWindow(hTextureBilinearButton, FALSE);
	EnableWindow(hTextureTrilinearButton, FALSE);
	SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_NEAREST, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_BILINEAR, BM_SETCHECK, 0,
		0);
	SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_TRILINEAR, BM_SETCHECK, 0,
		0);
}

void LDViewPreferences::enablePrimitives(void)
{
	EnableWindow(hTextureStudsButton, TRUE);
	EnableWindow(hCurveQualityLabel, TRUE);
	EnableWindow(hCurveQualitySlider, TRUE);
	SendDlgItemMessage(hPrimitivesPage, IDC_PRIMITIVE_SUBSTITUTION, BM_SETCHECK,
		1, 0);
	SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_STUDS, BM_SETCHECK,
		textureStuds, 0);
	if (textureStuds)
	{
		enableTextureFiltering();
	}
	else
	{
		disableTextureFiltering();
	}
}

void LDViewPreferences::disablePrimitives(void)
{
	EnableWindow(hTextureStudsButton, FALSE);
	EnableWindow(hCurveQualityLabel, FALSE);
	EnableWindow(hCurveQualitySlider, FALSE);
	SendDlgItemMessage(hPrimitivesPage, IDC_PRIMITIVE_SUBSTITUTION, BM_SETCHECK,
		0, 0);
	SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_STUDS, BM_SETCHECK, 0, 0);
	disableTextureFiltering();
}

void LDViewPreferences::setupSeamWidth(void)
{
	UDACCEL accels[] = {{0, 1}, {1, 10}, {2, 20}};

	hSeamSpin = GetDlgItem(hGeometryPage, IDC_SEAM_SPIN);
	SendDlgItemMessage(hGeometryPage, IDC_SEAM_SPIN, UDM_SETRANGE, 0,
		MAKELONG((short)500, (short)0)); 
	SendDlgItemMessage(hGeometryPage, IDC_SEAM_SPIN, UDM_SETPOS, 0, seamWidth);
	SendDlgItemMessage(hGeometryPage, IDC_SEAM_SPIN, UDM_SETACCEL,
		sizeof(accels) / sizeof(UDACCEL), (long)accels);
	if (useSeams)
	{
		enableSeams();
	}
	else
	{
		disableSeams();
	}
}

void LDViewPreferences::setupFullScreenRefresh(void)
{
	hFullScreenRateField = GetDlgItem(hGeneralPage, IDC_FS_RATE);
	SendDlgItemMessage(hGeneralPage, IDC_FS_RATE, EM_SETLIMITTEXT, 3, 0);
	if (fullScreenRefresh)
	{
		char buf[128];

		sprintf(buf, "%d", fullScreenRefresh);
		buf[3] = 0;
		SendDlgItemMessage(hGeneralPage, IDC_FS_RATE, WM_SETTEXT, 0,
			(LPARAM)buf);
		SendDlgItemMessage(hGeneralPage, IDC_FS_REFRESH, BM_SETCHECK, 1, 0);
		EnableWindow(hFullScreenRateField, TRUE);
	}
	else
	{
		SendDlgItemMessage(hGeneralPage, IDC_FS_RATE, WM_SETTEXT, 0,
			(LPARAM)"");
		SendDlgItemMessage(hGeneralPage, IDC_FS_REFRESH, BM_SETCHECK, 0, 0);
		EnableWindow(hFullScreenRateField, FALSE);
	}
}

/*
void LDViewPreferences::drawButtonBorder(HDC hdc, COLORREF color1,
										 COLORREF color2, RECT rect)
{
	HPEN hPen1 = CreatePen(PS_SOLID, 1, color1);
	HPEN hPen2 = CreatePen(PS_SOLID, 1, color2);
	HPEN hOldPen;

	hOldPen = (HPEN)SelectObject(hdc, hPen1);
	MoveToEx(hBackgroundColorDC, rect.left, rect.bottom, NULL);
	LineTo(hBackgroundColorDC, rect.left, rect.top);
	LineTo(hBackgroundColorDC, rect.right, rect.top);
	SelectObject(hBackgroundColorDC, hPen2);
	LineTo(hBackgroundColorDC, rect.right, rect.bottom);
	LineTo(hBackgroundColorDC, rect.left, rect.bottom);
	MoveToEx(hBackgroundColorDC, rect.left, rect.bottom, NULL);
	LineTo(hBackgroundColorDC, rect.right, rect.bottom);
	SelectObject(hdc, hOldPen);
	DeleteObject(hPen1);
	DeleteObject(hPen2);
}
*/

void LDViewPreferences::redrawColorBitmap(HWND hColorButton,
										  HBITMAP hButtonBitmap, COLORREF color,
										  HTHEME hTheme)
{
	HBRUSH hBrush = CreateSolidBrush(color);
	RECT bitmapRect;
	SIZE bitmapSize;
	HBITMAP hOldBitmap;
	HBRUSH hOldBrush;

	GetBitmapDimensionEx(hButtonBitmap, &bitmapSize);
	bitmapRect.left = 0;
	bitmapRect.top = 0;
	bitmapRect.right = bitmapSize.cx;
	bitmapRect.bottom = bitmapSize.cy;
	hOldBrush = (HBRUSH)SelectObject(hButtonColorDC, hBrush);
	hOldBitmap = (HBITMAP)SelectObject(hButtonColorDC, hButtonBitmap);
	FillRect(hButtonColorDC, &bitmapRect, hBrush);
//	bitmapRect.top++;
//	bitmapRect.left++;
//	InflateRect(&bitmapRect, 1, 1);
	if (CUIThemes::isThemeLibLoaded() && hTheme)
	{
		CUIThemes::drawThemeEdge(hTheme, hButtonColorDC, BP_PUSHBUTTON,
			PBS_PRESSED, &bitmapRect, EDGE_SUNKEN, BF_SOFT | BF_RECT, NULL);
	}
	else
	{
		DrawEdge(hButtonColorDC, &bitmapRect, EDGE_SUNKEN,
			BF_BOTTOMLEFT | BF_TOPRIGHT);
	}
	SelectObject(hButtonColorDC, hOldBrush);
	SelectObject(hButtonColorDC, hOldBitmap);
	DeleteObject(hBrush);
	InvalidateRect(hColorButton, NULL, 1);
}

void LDViewPreferences::applyPrefSetsChanges(void)
{
	if (hPrefSetsPage)
	{
		TCStringArray *oldPrefSetNames = TCUserDefaults::getAllSessionNames();
		int i;
		int count = oldPrefSetNames->getCount();
		char *prefSetName;
		const char *sessionName = TCUserDefaults::getSessionName();
		bool changed = false;

		for (i = 0; i < count; i++)
		{
			char *oldPrefSetName = oldPrefSetNames->stringAtIndex(i);
			int index = SendMessage(hPrefSetsList, LB_FINDSTRINGEXACT, 1,
				(LPARAM)oldPrefSetName);

			if (index == LB_ERR)
			{
				TCUserDefaults::removeSession(oldPrefSetName);
			}
		}
		count = SendMessage(hPrefSetsList, LB_GETCOUNT, 0, 0);
		for (i = 1; i < count; i++)
		{
			prefSetName = getPrefSet(i);
			if (oldPrefSetNames->indexOfString(prefSetName) < 0)
			{
				TCUserDefaults::setSessionName(prefSetName);
			}
			delete prefSetName;
		}
		oldPrefSetNames->release();
		prefSetName = getSelectedPrefSet();
		if (strcmp(prefSetName, DEFAULT_PREF_SET) == 0)
		{
			if (sessionName && sessionName[0])
			{
				TCUserDefaults::setSessionName(NULL, PREFERENCE_SET_KEY);
				changed = true;
			}
		}
		else
		{
			if (!sessionName || strcmp(sessionName, prefSetName) != 0)
			{
				TCUserDefaults::setSessionName(prefSetName, PREFERENCE_SET_KEY);
				changed = true;
			}
		}
		if (changed)
		{
			loadSettings();
			applySettings();
			if (hGeneralPage)
			{
				setupPage(generalPageNumber);
			}
			if (hGeometryPage)
			{
				setupPage(geometryPageNumber);
			}
			if (hEffectsPage)
			{
				setupPage(effectsPageNumber);
			}
			if (hPrimitivesPage)
			{
				setupPage(primitivesPageNumber);
			}
		}
		delete prefSetName;
	}
}

void LDViewPreferences::applyGeneralChanges(void)
{
	if (hGeneralPage)
	{
		int iTemp;
		float fTemp;
		int i;
		char buf[128];

		TCUserDefaults::setLongForKey(fsaaMode, FSAA_MODE_KEY);
		lineSmoothing = SendDlgItemMessage(hGeneralPage, IDC_LINE_AA,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(lineSmoothing, LINE_SMOOTHING_KEY);
		TCUserDefaults::setLongForKey((long)backgroundColor,
			BACKGROUND_COLOR_KEY);
		TCUserDefaults::setLongForKey((long)defaultColor,
			DEFAULT_COLOR_KEY);
		for (i = 0; i < 16; i++)
		{
			if (customColors[i])
			{
				char key[128];

				sprintf(key, "%s/Color%02d", CUSTOM_COLORS_KEY, i);
				TCUserDefaults::setLongForKey((long)customColors[i], key,
					false);
			}
		}
		processLDConfig = SendDlgItemMessage(hGeneralPage, IDC_PROCESS_LDCONFIG,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(processLDConfig, PROCESS_LDCONFIG_KEY);
		showsFPS = SendDlgItemMessage(hGeneralPage, IDC_FRAME_RATE, BM_GETCHECK,
			0, 0) != 0;
		TCUserDefaults::setLongForKey(showsFPS, SHOW_FPS_KEY);
		showErrors = SendDlgItemMessage(hGeneralPage, IDC_SHOW_ERRORS,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(showErrors, SHOW_ERRORS_KEY, false);
		iTemp = SendDlgItemMessage(hGeneralPage, IDC_FS_REFRESH, BM_GETCHECK, 0,
			0);
		if (iTemp)
		{
			SendDlgItemMessage(hGeneralPage, IDC_FS_RATE, WM_GETTEXT, 4,
				(LPARAM)buf);
			if (sscanf(buf, "%d", &iTemp) == 1)
			{
				if (!iTemp)
				{
					iTemp = -1;
				}
			}
			else
			{
				iTemp = -1;
			}
		}
		if (iTemp >= 0)
		{
			fullScreenRefresh = iTemp;
			TCUserDefaults::setLongForKey(fullScreenRefresh,
				FULLSCREEN_REFRESH_KEY);
		}
		SendDlgItemMessage(hGeneralPage, IDC_FOV, WM_GETTEXT, 6, (LPARAM)buf);
		if (sscanf(buf, "%f", &fTemp) == 1)
		{
			if (fTemp >= getMinFov() && fTemp <= getMaxFov())
			{
				fov = fTemp;
				TCUserDefaults::setFloatForKey(fov, FOV_KEY);
			}
			else
			{
				setupFov(true);
			}
		}
		else
		{
			setupFov(true);
		}
		applyGeneralSettings();
	}
}

void LDViewPreferences::applyGeometryChanges(void)
{
	if (hGeometryPage)
	{
		useSeams = SendDlgItemMessage(hGeometryPage, IDC_SEAMS, BM_GETCHECK, 0,
			0);
		TCUserDefaults::setLongForKey(useSeams, SEAMS_KEY);
		if (useSeams)
		{
			TCUserDefaults::setLongForKey(seamWidth, SEAM_WIDTH_KEY);
		}
		else
		{
			seamWidth = TCUserDefaults::longForKey(SEAM_WIDTH_KEY);
		}
		drawWireframe = SendDlgItemMessage(hGeometryPage, IDC_WIREFRAME,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(drawWireframe, WIREFRAME_KEY);
		useWireframeFog = SendDlgItemMessage(hGeometryPage, IDC_WIREFRAME_FOG,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(useWireframeFog, WIREFRAME_FOG_KEY);
		removeHiddenLines = SendDlgItemMessage(hGeometryPage,
			IDC_REMOVE_HIDDEN_LINES, BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(removeHiddenLines,
			REMOVE_HIDDEN_LINES_KEY);
		wireframeThickness = SendDlgItemMessage(hGeometryPage,
			IDC_WIREFRAME_THICKNESS, TBM_GETPOS, 0, 0);
		TCUserDefaults::setLongForKey(wireframeThickness, 
			WIREFRAME_THICKNESS_KEY);
		showsHighlightLines = SendDlgItemMessage(hGeometryPage, IDC_HIGHLIGHTS,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(showsHighlightLines,
			SHOWS_HIGHLIGHT_LINES_KEY);
		if (showsHighlightLines)
		{
			edgesOnly = SendDlgItemMessage(hGeometryPage, IDC_EDGES_ONLY,
				BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(edgesOnly, EDGES_ONLY_KEY);
			drawConditionalHighlights = SendDlgItemMessage(hGeometryPage,
				IDC_CONDITIONAL_HIGHLIGHTS, BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(drawConditionalHighlights,
				CONDITIONAL_HIGHLIGHTS_KEY);
			if (drawConditionalHighlights)
			{
				showAllConditionalLines = SendDlgItemMessage(hGeometryPage,
					IDC_ALL_CONDITIONAL, BM_GETCHECK, 0, 0) != 0;
				TCUserDefaults::setLongForKey(showAllConditionalLines ? 1 : 0,
					SHOW_ALL_TYPE5_KEY);
				showConditionalControlPoints = SendDlgItemMessage(hGeometryPage,
					IDC_CONDITIONAL_CONTROLS, BM_GETCHECK, 0, 0) != 0;
				TCUserDefaults::setLongForKey(showConditionalControlPoints ?
					1 : 0, SHOW_TYPE5_CONTROL_POINTS_KEY);
			}
			usePolygonOffset = SendDlgItemMessage(hGeometryPage,
				IDC_QUALITY_LINES, BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(usePolygonOffset, POLYGON_OFFSET_KEY);
			blackHighlights = SendDlgItemMessage(hGeometryPage,
				IDC_ALWAYS_BLACK, BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(blackHighlights,
				BLACK_HIGHLIGHTS_KEY);
		}
		edgeThickness = SendDlgItemMessage(hGeometryPage, IDC_EDGE_THICKNESS,
			TBM_GETPOS, 0, 0);
		TCUserDefaults::setLongForKey(edgeThickness,  EDGE_THICKNESS_KEY);
		applyGeometrySettings();
	}
}

void LDViewPreferences::applyEffectsChanges(void)
{
	if (hEffectsPage)
	{
		useLighting = SendDlgItemMessage(hEffectsPage, IDC_LIGHTING,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(useLighting, LIGHTING_KEY);
		if (useLighting)
		{
			qualityLighting = SendDlgItemMessage(hEffectsPage,
				IDC_LIGHTING_QUALITY, BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(qualityLighting,
				QUALITY_LIGHTING_KEY);
			subduedLighting = SendDlgItemMessage(hEffectsPage,
				IDC_LIGHTING_SUBDUED, BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(subduedLighting,
				SUBDUED_LIGHTING_KEY);
			usesSpecular = SendDlgItemMessage(hEffectsPage, IDC_SPECULAR,
				BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(usesSpecular, SPECULAR_KEY);
			oneLight = SendDlgItemMessage(hEffectsPage,
				IDC_ALTERNATE_LIGHTING, BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(oneLight, ONE_LIGHT_KEY);
		}
		TCUserDefaults::setLongForKey(stereoMode, STEREO_MODE_KEY);
		stereoEyeSpacing = SendDlgItemMessage(hEffectsPage, IDC_STEREO_SPACING,
			TBM_GETPOS, 0, 0);
		TCUserDefaults::setLongForKey(stereoEyeSpacing, STEREO_SPACING_KEY);
		TCUserDefaults::setLongForKey(cutawayMode, CUTAWAY_MODE_KEY);
		cutawayAlpha = SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY_OPACITY,
			TBM_GETPOS, 0, 0);
		TCUserDefaults::setLongForKey(cutawayAlpha, CUTAWAY_ALPHA_KEY);
		cutawayThickness = SendDlgItemMessage(hEffectsPage,
			IDC_CUTAWAY_THICKNESS, TBM_GETPOS, 0, 0);
		TCUserDefaults::setLongForKey(cutawayThickness, CUTAWAY_THICKNESS_KEY);
		useStipple = SendDlgItemMessage(hEffectsPage, IDC_STIPPLE,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(useStipple, STIPPLE_KEY);
		sortTransparent = SendDlgItemMessage(hEffectsPage, IDC_SORT,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(sortTransparent, SORT_KEY);
		usesFlatShading = SendDlgItemMessage(hEffectsPage, IDC_FLAT_SHADING,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(usesFlatShading, FLAT_SHADING_KEY);
		performSmoothing = SendDlgItemMessage(hEffectsPage, IDC_SMOOTH_CURVES,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(performSmoothing, PERFORM_SMOOTHING_KEY);
		applyEffectsSettings();
	}
}

void LDViewPreferences::applyPrimitivesChanges(void)
{
	if (hPrimitivesPage)
	{
		allowPrimitiveSubstitution = SendDlgItemMessage(hPrimitivesPage,
			IDC_PRIMITIVE_SUBSTITUTION, BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(allowPrimitiveSubstitution,
			PRIMITIVE_SUBSTITUTION_KEY);
		if (allowPrimitiveSubstitution)
		{
			textureStuds = SendDlgItemMessage(hPrimitivesPage,
				IDC_TEXTURE_STUDS, BM_GETCHECK, 0, 0) != 0;
			TCUserDefaults::setLongForKey(textureStuds, TEXTURE_STUDS_KEY);
			TCUserDefaults::setLongForKey(textureFilterType,
				TEXTURE_FILTER_TYPE_KEY);
			curveQuality = SendDlgItemMessage(hPrimitivesPage,
				IDC_CURVE_QUALITY, TBM_GETPOS, 0, 0);
			TCUserDefaults::setLongForKey(curveQuality, CURVE_QUALITY_KEY);
		}
		qualityStuds = SendDlgItemMessage(hPrimitivesPage, IDC_STUD_QUALITY,
			BM_GETCHECK, 0, 0) == 0;
		TCUserDefaults::setLongForKey(qualityStuds, QUALITY_STUDS_KEY);
		hiResPrimitives = SendDlgItemMessage(hPrimitivesPage, IDC_HI_RES,
			BM_GETCHECK, 0, 0) != 0;
		TCUserDefaults::setLongForKey(hiResPrimitives, HI_RES_PRIMITIVES_KEY);
		applyPrimitivesSettings();
	}
}

void LDViewPreferences::applyChanges(void)
{
	applyGeneralChanges();
	applyGeometryChanges();
	applyEffectsChanges();
	applyPrimitivesChanges();
	applyPrefSetsChanges();	// Note that if there are any pref sets changes,
							// there can't be any other changes.
}

void LDViewPreferences::saveDefaultView(void)
{
	float matrix[16];
	float rotationMatrix[16];
	float otherMatrix[16] = {1,0,0,0,0,-1,0,0,0,0,-1,0,0,0,0,1};
	char matrixString[1024];

	memcpy(rotationMatrix, modelViewer->getRotationMatrix(),
		sizeof(rotationMatrix));
	TCVector::multMatrix(otherMatrix, rotationMatrix, matrix);
	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = 0.0f;
	sprintf(matrixString, "%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g",
		matrix[0], matrix[4], matrix[8],
		matrix[1], matrix[5], matrix[9],
		matrix[2], matrix[6], matrix[10]);
	TCUserDefaults::setStringForKey(matrixString, DEFAULT_MATRIX_KEY);
	modelViewer->setDefaultRotationMatrix(matrix);
}

void LDViewPreferences::resetDefaultView(void)
{
	TCUserDefaults::removeValue(DEFAULT_MATRIX_KEY);
	if (modelViewer)
	{
		modelViewer->setDefaultRotationMatrix(NULL);
	}
}

void LDViewPreferences::getRGB(int color, int &r, int &g, int &b)
{
	r = color & 0xFF;
	g = (color >> 8) & 0xFF;
	b = (color >> 16) & 0xFF;
}

void LDViewPreferences::chooseBackgroundColor(void)
{
	chooseColor(hBackgroundColorButton, hBackgroundColorBitmap,
		backgroundColor, hBackgroundColorTheme);
}

void LDViewPreferences::chooseDefaultColor(void)
{
	chooseColor(hDefaultColorButton, hDefaultColorBitmap, defaultColor,
		hDefaultColorTheme);
}

void LDViewPreferences::chooseColor(HWND hColorButton, HBITMAP hColorBitmap,
									COLORREF &color, HTHEME hTheme)
{
	CHOOSECOLOR chooseColor;

	memset(&chooseColor, 0, sizeof CHOOSECOLOR);
	chooseColor.lStructSize = sizeof CHOOSECOLOR;
	chooseColor.hwndOwner = hPropSheet;
	chooseColor.rgbResult = color;
	chooseColor.lpCustColors = customColors;
	chooseColor.Flags = CC_ANYCOLOR | CC_RGBINIT;
	EnableWindow(hPropSheet, FALSE);
	if (ChooseColor(&chooseColor))
	{
		color = chooseColor.rgbResult;
		redrawColorBitmap(hColorButton, hColorBitmap, color, hTheme);
	}
	EnableWindow(hPropSheet, TRUE);
}

void LDViewPreferences::doGeneralClick(int controlId, HWND /*controlHWnd*/)
{
	switch (controlId)
	{
		case IDC_FS_REFRESH:
			doFSRefresh();
			break;
		case IDC_BACKGROUND_COLOR:
			chooseBackgroundColor();
			break;
		case IDC_DEFAULT_COLOR:
			chooseDefaultColor();
			break;
		case IDC_GENERAL_RESET:
			loadDefaultGeneralSettings();
			setupGeneralPage();
			break;
	}
	enableApply(hGeneralPage);
}

void LDViewPreferences::doGeometryClick(int controlId, HWND /*controlHWnd*/)
{
	switch (controlId)
	{
		case IDC_HIGHLIGHTS:
			doHighlights();
			break;
		case IDC_CONDITIONAL_HIGHLIGHTS:
			doConditionals();
			break;
		case IDC_WIREFRAME:
			doWireframe();
			break;
		case IDC_SEAMS:
			doSeams();
			break;
		case IDC_GEOMETRY_RESET:
			loadDefaultGeometrySettings();
			setupGeometryPage();
			break;
	}
	enableApply(hGeometryPage);
}

void LDViewPreferences::doEffectsClick(int controlId, HWND /*controlHWnd*/)
{
	switch (controlId)
	{
		case IDC_LIGHTING:
			doLighting();
			break;
		case IDC_STIPPLE:
			doStipple();
			break;
		case IDC_SORT:
			doSort();
			break;
		case IDC_FLAT_SHADING:
			doFlatShading();
			break;
		case IDC_SMOOTH_CURVES:
			doSmoothCurves();
			break;
		case IDC_STEREO:
			doStereo();
			break;
		case IDC_HARDWARE_STEREO:
			stereoMode = LDVStereoHardware;
			break;
		case IDC_CROSS_EYED_STEREO:
			stereoMode = LDVStereoCrossEyed;
			break;
		case IDC_PARALLEL_STEREO:
			stereoMode = LDVStereoParallel;
			break;
		case IDC_CUTAWAY:
			doCutaway();
			break;
		case IDC_CUTAWAY_COLOR:
			cutawayMode = LDVCutawayWireframe;
			setupOpacitySlider();
			break;
		case IDC_CUTAWAY_MONOCHROME:
			cutawayMode = LDVCutawayStencil;
			setupOpacitySlider();
			break;
		case IDC_EFFECTS_RESET:
			loadDefaultEffectsSettings();
			setupEffectsPage();
			break;
	}
	enableApply(hEffectsPage);
}

void LDViewPreferences::doDeletePrefSet(void)
{
	char *selectedPrefSet = getSelectedPrefSet();

	if (selectedPrefSet)
	{
		int selectedIndex = SendMessage(hPrefSetsList, LB_FINDSTRINGEXACT, 0,
			(LPARAM)selectedPrefSet);

		if (checkAbandon && getApplyEnabled())
		{
			if (MessageBox(hWindow, "You have made changes to the current pref "
				"set that have not been applied.  If you delete this pref set, "
				"those changes will be lost.  Are you sure you want to delete "
				"it?", "Abandon changes?", MB_YESNO | MB_ICONQUESTION) ==
				IDYES)
			{
				abandonChanges();
			}
			else
			{
				delete selectedPrefSet;
				return;
			}
		}
		checkAbandon = false;
		SendMessage(hPrefSetsList, LB_DELETESTRING, selectedIndex, 0);
		delete selectedPrefSet;
		if (selectedIndex == SendMessage(hPrefSetsList, LB_GETCOUNT, 0, 0))
		{
			selectedIndex--;
		}
		selectedPrefSet = getPrefSet(selectedIndex);
		selectPrefSet(selectedPrefSet, true);
		delete selectedPrefSet;
	}
}

void LDViewPreferences::doNewPrefSet(void)
{
	newPrefSetName = NULL;
	if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_NEW_PREF_SET),
		hPropSheet, staticDialogProc, (LPARAM)this) == IDOK)
	{
		if (newPrefSetName)
		{
			SendMessage(hPrefSetsList, LB_ADDSTRING, 0, (LPARAM)newPrefSetName);
			selectPrefSet(newPrefSetName);
			delete newPrefSetName;
		}
	}
}

char *LDViewPreferences::getHotKey(int index)
{
	char key[128];

	sprintf(key, "%s/Key%d", HOT_KEYS_KEY, index);
	return TCUserDefaults::stringForKey(key, NULL, false);
}

int LDViewPreferences::getHotKey(const char *currentPrefSetName)
{
	int i;
	int retValue = -1;

	for (i = 0; i < 10 && retValue == -1; i++)
	{
		char *prefSetName = getHotKey(i);

		if (prefSetName)
		{
			if (strcmp(prefSetName, currentPrefSetName) == 0)
			{
				retValue = i;
			}
			delete prefSetName;
		}
	}
	return retValue;
}

int LDViewPreferences::getCurrentHotKey(void)
{
	char *currentPrefSetName = getSelectedPrefSet();
	int retValue = -1;

	if (currentPrefSetName)
	{
		retValue = getHotKey(currentPrefSetName);
		delete currentPrefSetName;
	}
	return retValue;
}

bool LDViewPreferences::performHotKey(int hotKeyIndex)
{
	char *hotKeyPrefSetName = getHotKey(hotKeyIndex);
	bool retValue = false;

	if (hotKeyPrefSetName && !hPropSheet)
	{
		const char *currentSessionName = TCUserDefaults::getSessionName();
		bool hotKeyIsDefault = strcmp(hotKeyPrefSetName, DEFAULT_PREF_SET) == 0;

		if (currentSessionName)
		{
			if (strcmp(currentSessionName, hotKeyPrefSetName) == 0)
			{
				retValue = true;
			}
		}
		else if (hotKeyIsDefault)
		{
			retValue = true;
		}
		if (!retValue)
		{
			bool changed = false;

			if (hotKeyIsDefault)
			{
				TCUserDefaults::setSessionName(NULL, PREFERENCE_SET_KEY);
				changed = true;
			}
			else
			{
				TCStringArray *sessionNames =
					TCUserDefaults::getAllSessionNames();

				if (sessionNames->indexOfString(hotKeyPrefSetName) != -1)
				{
					TCUserDefaults::setSessionName(hotKeyPrefSetName,
						PREFERENCE_SET_KEY);
					{
						changed = true;
					}
				}
				sessionNames->release();
			}
			if (changed)
			{
				loadSettings();
				applySettings();
				retValue = true;
			}
		}
		delete hotKeyPrefSetName;
	}
	return retValue;
}

void LDViewPreferences::saveCurrentHotKey(void)
{
	int currentHotKey = getCurrentHotKey();

	if (currentHotKey >= 0)
	{
		char key[128];

		sprintf(key, "%s/Key%d", HOT_KEYS_KEY, currentHotKey);
		TCUserDefaults::removeValue(key, false);
	}
	if (hotKeyIndex > 0)
	{
		char key[128];
		char *currentSessionName = getSelectedPrefSet();

		sprintf(key, "%s/Key%d", HOT_KEYS_KEY, hotKeyIndex % 10);
		TCUserDefaults::setStringForKey(currentSessionName, key, false);
		delete currentSessionName;
	}
}

void LDViewPreferences::doPrefSetHotKey(void)
{
	hotKeyIndex = getCurrentHotKey();
	
	if (hotKeyIndex == -1)
	{
		hotKeyIndex++;
	}
	else if (hotKeyIndex == 0)
	{
		hotKeyIndex = 10;
	}
	if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_HOTKEY), hPropSheet,
		staticDialogProc, (LPARAM)this) == IDOK)
	{
		if (hotKeyIndex != CB_ERR)
		{
			saveCurrentHotKey();
		}
	}
}

void LDViewPreferences::doPrefSetsClick(int controlId, HWND /*controlHWnd*/)
{
	switch (controlId)
	{
	case IDC_DELETE_PREF_SET:
		doDeletePrefSet();
		break;
	case IDC_NEW_PREF_SET:
		doNewPrefSet();
		break;
	case IDC_PREF_SET_HOT_KEY:
		doPrefSetHotKey();
		break;
	}
	enableApply(hPrefSetsPage);
}

void LDViewPreferences::doOtherClick(HWND hDlg, int controlId,
									 HWND /*controlHWnd*/)
{
	if (controlId == IDC_NEW_PREF_SET_CANCEL)
	{
		EndDialog(hDlg, IDCANCEL);
	}
	else if (controlId == IDC_NEW_PREF_SET_OK)
	{
		char editText[1024];

		SendDlgItemMessage(hDlg, IDC_NEW_PREF_SET_FIELD, WM_GETTEXT, 1024,
			(LPARAM)editText);
		if (strlen(editText))
		{
			int index = SendMessage(hPrefSetsList, LB_FINDSTRINGEXACT, 0,
				(LPARAM)editText);

			if (index == LB_ERR)
			{
				if (strchr(editText, '/') || strchr(editText, '\\'))
				{
				MessageBox(hDlg,
					"Preference set names cannot contain the characters '\\' "
					"or '/'.  ",
					"Invalid name", MB_OK | MB_ICONWARNING);
				}
				else
				{
					newPrefSetName = copyString(editText);
					EndDialog(hDlg, IDOK);
				}
			}
			else
			{
				MessageBox(hDlg,
					"A preference set already exists with that name.  "
					"You must enter a new unique name.",
					"Duplicate name", MB_OK | MB_ICONWARNING);
			}
		}
		else
		{
			MessageBox(hDlg,
				"You must enter a name for the new preference set.",
				"Empty name", MB_OK | MB_ICONWARNING);
		}
	}
	else if (controlId == IDC_HOTKEY_OK)
	{
		hotKeyIndex = SendDlgItemMessage(hDlg, IDC_HOTKEY_COMBO, CB_GETCURSEL,
			0, 0);
		EndDialog(hDlg, IDOK);
	}
	else if (controlId == IDC_APPLY || controlId == IDC_ABANDON ||
		controlId == IDCANCEL)
	{
		EndDialog(hDlg, controlId);
	}
}

void LDViewPreferences::doPrimitivesClick(int controlId, HWND /*controlHWnd*/)
{
	switch (controlId)
	{
		case IDC_PRIMITIVE_SUBSTITUTION:
			doPrimitives();
			break;
		case IDC_TEXTURE_STUDS:
			doTextureStuds();
			break;
		case IDC_TEXTURE_NEAREST:
			textureFilterType = GL_NEAREST_MIPMAP_NEAREST;
			break;
		case IDC_TEXTURE_BILINEAR:
			textureFilterType = GL_LINEAR_MIPMAP_NEAREST;
			break;
		case IDC_TEXTURE_TRILINEAR:
			textureFilterType = GL_LINEAR_MIPMAP_LINEAR;
			break;
		case IDC_PRIMITIVES_RESET:
			loadDefaultPrimitivesSettings();
			setupPrimitivesPage();
			break;
	}
	enableApply(hPrimitivesPage);
}

DWORD LDViewPreferences::doComboSelChange(HWND hPage, int controlId,
										  HWND /*controlHWnd*/)
{
	if (controlId == IDC_FSAA_COMBO)
	{
		char selectedString[1024];

		SendDlgItemMessage(hPage, controlId, WM_GETTEXT, sizeof(selectedString),
			(LPARAM)selectedString);
		if (strcmp(selectedString, "None") == 0)
		{
			fsaaMode = 0;
		}
		else
		{
			sscanf(selectedString, "%d", &fsaaMode);
			if (fsaaMode > 4)
			{
				fsaaMode = fsaaMode << 3;
			}
			else if (strstr(selectedString, "Enhanced"))
			{
				fsaaMode |= 1;
			}
		}
		enableApply(hPage);
	}
	return 0;
}

DWORD LDViewPreferences::doClick(HWND hPage, int controlId, HWND controlHWnd)
{
	bool wasPaused = true;

	if (modelViewer)
	{
		wasPaused = modelViewer->getPaused() != 0;
		if (!wasPaused)
		{
			modelViewer->pause();
		}
	}
	if (hPage == hGeneralPage)
	{
		doGeneralClick(controlId, controlHWnd);
	}
	else if (hPage == hGeometryPage)
	{
		doGeometryClick(controlId, controlHWnd);
	}
	else if (hPage == hEffectsPage)
	{
		doEffectsClick(controlId, controlHWnd);
	}
	else if (hPage == hPrimitivesPage)
	{
		doPrimitivesClick(controlId, controlHWnd);
	}
	else if (hPage == hPrefSetsPage)
	{
		doPrefSetsClick(controlId, controlHWnd);
	}
	else
	{
		doOtherClick(hPage, controlId, controlHWnd);
	}
	if (!wasPaused)
	{
		modelViewer->unpause();
	}
	return 0;
}

void LDViewPreferences::doFSRefresh(void)
{
	if (SendDlgItemMessage(hGeneralPage, IDC_FS_REFRESH, BM_GETCHECK, 0, 0))
	{
		EnableWindow(hFullScreenRateField, TRUE);
	}
	else
	{
		SendDlgItemMessage(hGeneralPage, IDC_FS_RATE, WM_SETTEXT, 0,
			(LPARAM)"");
		EnableWindow(hFullScreenRateField, FALSE);
	}
}

void LDViewPreferences::doStipple(void)
{
	if (SendDlgItemMessage(hEffectsPage, IDC_STIPPLE, BM_GETCHECK, 0, 0))
	{
		SendDlgItemMessage(hEffectsPage, IDC_SORT, BM_SETCHECK, 0, 0);
	}
}

void LDViewPreferences::doSort(void)
{
	if (SendDlgItemMessage(hEffectsPage, IDC_SORT, BM_GETCHECK, 0, 0))
	{
		SendDlgItemMessage(hEffectsPage, IDC_STIPPLE, BM_SETCHECK, 0, 0);
	}
}

void LDViewPreferences::doFlatShading(void)
{
	if (SendDlgItemMessage(hEffectsPage, IDC_FLAT_SHADING, BM_GETCHECK, 0, 0))
	{
		SendDlgItemMessage(hEffectsPage, IDC_SMOOTH_CURVES, BM_SETCHECK, 0, 0);
	}
}

void LDViewPreferences::doSmoothCurves(void)
{
	if (SendDlgItemMessage(hEffectsPage, IDC_SMOOTH_CURVES, BM_GETCHECK, 0, 0))
	{
		SendDlgItemMessage(hEffectsPage, IDC_FLAT_SHADING, BM_SETCHECK, 0, 0);
	}
}

void LDViewPreferences::doHighlights(void)
{
	if (SendDlgItemMessage(hGeometryPage, IDC_HIGHLIGHTS, BM_GETCHECK, 0, 0))
	{
		enableEdges();
	}
	else
	{
		disableEdges();
	}
}

void LDViewPreferences::doConditionals(void)
{
	if (SendDlgItemMessage(hGeometryPage, IDC_CONDITIONAL_HIGHLIGHTS,
		BM_GETCHECK, 0, 0))
	{
		enableConditionals();
	}
	else
	{
		disableConditionals();
	}
}

void LDViewPreferences::doWireframe(void)
{
	if (SendDlgItemMessage(hGeometryPage, IDC_WIREFRAME, BM_GETCHECK, 0, 0))
	{
		enableWireframe();
	}
	else
	{
		disableWireframe();
	}
}

void LDViewPreferences::doLighting(void)
{
	if (SendDlgItemMessage(hEffectsPage, IDC_LIGHTING, BM_GETCHECK, 0, 0))
	{
		enableLighting();
	}
	else
	{
		disableLighting();
	}
}

void LDViewPreferences::doStereo(void)
{
	if (SendDlgItemMessage(hEffectsPage, IDC_STEREO, BM_GETCHECK, 0, 0))
	{
		stereoMode = LDVStereoCrossEyed;
		enableStereo();
	}
	else
	{
		stereoMode = LDVStereoNone;
		disableStereo();
	}
}

void LDViewPreferences::doCutaway(void)
{
	if (SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY, BM_GETCHECK, 0,
		0))
	{
		cutawayMode = LDVCutawayWireframe;
		enableCutaway();
	}
	else
	{
		cutawayMode = LDVCutawayNormal;
		disableCutaway();
	}
}

void LDViewPreferences::doSeams(void)
{
	if (SendDlgItemMessage(hGeometryPage, IDC_SEAMS, BM_GETCHECK, 0, 0))
	{
		enableSeams();
	}
	else
	{
		disableSeams();
	}
}

void LDViewPreferences::doPrimitives(void)
{
	if (SendDlgItemMessage(hPrimitivesPage, IDC_PRIMITIVE_SUBSTITUTION,
		BM_GETCHECK, 0, 0))
	{
		enablePrimitives();
	}
	else
	{
		disablePrimitives();
	}
}

void LDViewPreferences::doTextureStuds(void)
{
	if (SendDlgItemMessage(hPrimitivesPage, IDC_TEXTURE_STUDS, BM_GETCHECK, 0,
		0))
	{
		enableTextureFiltering();
	}
	else
	{
		disableTextureFiltering();
	}
}

void LDViewPreferences::doReset(void)
{
	loadSettings();
/*
	backgroundColor =
		(COLORREF)TCUserDefaults::longForKey(BACKGROUND_COLOR_KEY);
	defaultColor = (COLORREF)TCUserDefaults::longForKey(DEFAULT_COLOR_KEY,
		0x999999);
	seamWidth = TCUserDefaults::longForKey(SEAM_WIDTH_KEY);
	fsaaMode = TCUserDefaults::longForKey(FSAA_MODE_KEY);
	stereoMode = (LDVStereoMode)TCUserDefaults::longForKey(STEREO_MODE_KEY);
	cutawayMode = (LDVCutawayMode)TCUserDefaults::longForKey(CUTAWAY_MODE_KEY);
	textureFilterType = TCUserDefaults::longForKey(TEXTURE_FILTER_TYPE_KEY,
		GL_LINEAR_MIPMAP_LINEAR);
*/
}

BOOL LDViewPreferences::doDialogVScroll(HWND hDlg, int scrollCode, int position,
										HWND hScrollBar)
{
	if (scrollCode == SB_THUMBPOSITION && hScrollBar == hSeamSpin)
	{
		char seamWidthString[128];

		seamWidth += (position - seamWidth);
		sprintf(seamWidthString, "%0.2f", seamWidth / 100.0f);
		SendDlgItemMessage(hDlg, IDC_SEAM_WIDTH_FIELD, WM_SETTEXT, 0,
			(LPARAM)seamWidthString);
		SendDlgItemMessage(hDlg, IDC_SEAM_SPIN, UDM_SETPOS, 0, seamWidth);
		if (seamWidth)
		{
			SendDlgItemMessage(hGeometryPage, IDC_SEAMS, BM_SETCHECK,
				1, 0);
		}
		else
		{
			SendDlgItemMessage(hGeometryPage, IDC_SEAMS, BM_SETCHECK,
				0, 0);
		}
		enableApply(hGeometryPage);
	}
	return TRUE;
}

void LDViewPreferences::setupPage(int pageNumber)
{
	if (pageNumber == generalPageNumber)
	{
		setupGeneralPage();
	}
	else if (pageNumber == geometryPageNumber)
	{
		setupGeometryPage();
	}
	else if (pageNumber == effectsPageNumber)
	{
		setupEffectsPage();
	}
	else if (pageNumber == primitivesPageNumber)
	{
		setupPrimitivesPage();
	}
	else if (pageNumber == prefSetsPageNumber)
	{
		setupPrefSetsPage();
	}
	disableApply(hwndArray->pointerAtIndex(pageNumber));
}

void LDViewPreferences::setupFov(bool warn)
{
	char buf[1024];
	float minFov = getMinFov();
	float maxFov = getMaxFov();

	SendDlgItemMessage(hGeneralPage, IDC_FOV, EM_SETLIMITTEXT, 5, 0);
	sprintf(buf, "%.4g", fov);
	SendDlgItemMessage(hGeneralPage, IDC_FOV, WM_SETTEXT, 0, (LPARAM)buf);
	sprintf(buf, "(%g - %g)", minFov, maxFov);
	SendDlgItemMessage(hGeneralPage, IDC_FOV_RANGE_LABEL, WM_SETTEXT, 0,
		(LPARAM)buf);
	if (warn)
	{
		sprintf(buf, "Field of view must be a number from %g to %g.  It has "
			"been reset to its previous value.", minFov, maxFov);
		MessageBox(hPropSheet, buf, "Invalid value",
			MB_OK | MB_ICONWARNING);
	}
}

void LDViewPreferences::setupGeneralPage(void)
{
	hGeneralPage = hwndArray->pointerAtIndex(generalPageNumber);
	setupAntialiasing();
	SendDlgItemMessage(hGeneralPage, IDC_PROCESS_LDCONFIG, BM_SETCHECK,
		processLDConfig ? 1 : 0, 0);
	SendDlgItemMessage(hGeneralPage, IDC_FRAME_RATE, BM_SETCHECK, showsFPS,
		0);
	SendDlgItemMessage(hGeneralPage, IDC_SHOW_ERRORS, BM_SETCHECK,
		showErrors, 0);
	setupFov();
	setupFullScreenRefresh();
	setupBackgroundColorButton();
	setupDefaultColorButton();
}

void LDViewPreferences::setupModelGeometry(void)
{
//	hCurveQualityLabel = GetDlgItem(hGeometryPage, IDC_CURVE_QUALITY_LABEL);
//	hCurveQualitySlider = GetDlgItem(hGeometryPage, IDC_CURVE_QUALITY);
//	SendDlgItemMessage(hGeometryPage, IDC_STUD_QUALITY, BM_SETCHECK,
//		!qualityStuds, 0);
//	setupDialogSlider(hGeometryPage, IDC_CURVE_QUALITY, 1, 6, 1,
//		curveQuality);
	setupSeamWidth();
/*
	if (allowPrimitiveSubstitution)
	{
		enablePrimitives();
	}
	else
	{
		disablePrimitives();
	}
*/
}

void LDViewPreferences::enableWireframe(void)
{
	EnableWindow(hWireframeFogButton, TRUE);
	EnableWindow(hRemoveHiddenLinesButton, TRUE);
	EnableWindow(hWireframeThicknessLabel, TRUE);
	EnableWindow(hWireframeThicknessSlider, TRUE);
}

void LDViewPreferences::disableWireframe(void)
{
	EnableWindow(hWireframeFogButton, FALSE);
	EnableWindow(hRemoveHiddenLinesButton, FALSE);
	EnableWindow(hWireframeThicknessLabel, FALSE);
	EnableWindow(hWireframeThicknessSlider, FALSE);
}

void LDViewPreferences::setupWireframe(void)
{
	hWireframeFogButton = GetDlgItem(hGeometryPage, IDC_WIREFRAME_FOG);
	hRemoveHiddenLinesButton = GetDlgItem(hGeometryPage,
		IDC_REMOVE_HIDDEN_LINES);
	hWireframeThicknessLabel = GetDlgItem(hGeometryPage,
		IDC_WIREFRAME_THICKNESS_LABEL);
	hWireframeThicknessSlider = GetDlgItem(hGeometryPage,
		IDC_WIREFRAME_THICKNESS);
	SendDlgItemMessage(hGeometryPage, IDC_WIREFRAME, BM_SETCHECK, drawWireframe,
		0);
	SendDlgItemMessage(hGeometryPage, IDC_WIREFRAME_FOG, BM_SETCHECK,
		useWireframeFog, 0);
	SendDlgItemMessage(hGeometryPage, IDC_REMOVE_HIDDEN_LINES, BM_SETCHECK,
		removeHiddenLines, 0);
	setupDialogSlider(hGeometryPage, IDC_WIREFRAME_THICKNESS, 1, 5, 1,
		wireframeThickness);
	if (drawWireframe)
	{
		enableWireframe();
	}
	else
	{
		disableWireframe();
	}
}

void LDViewPreferences::enableConditionals(void)
{
	EnableWindow(hShowAllConditionalButton, TRUE);
	EnableWindow(hShowConditionalControlsButton, TRUE);
	SendDlgItemMessage(hGeometryPage, IDC_ALL_CONDITIONAL, BM_SETCHECK,
		showAllConditionalLines ? 1 : 0, 0);
	SendDlgItemMessage(hGeometryPage, IDC_CONDITIONAL_CONTROLS, BM_SETCHECK,
		showConditionalControlPoints ? 1 : 0, 0);
}

void LDViewPreferences::disableConditionals(void)
{
	EnableWindow(hShowAllConditionalButton, FALSE);
	EnableWindow(hShowConditionalControlsButton, FALSE);
	SendDlgItemMessage(hGeometryPage, IDC_ALL_CONDITIONAL, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hGeometryPage, IDC_CONDITIONAL_CONTROLS, BM_SETCHECK, 0,
		0);
}

void LDViewPreferences::enableEdges(void)
{
	EnableWindow(hConditionalHighlightsButton, TRUE);
	EnableWindow(hEdgesOnlyButton, TRUE);
	EnableWindow(hQualityLinesButton, TRUE);
	EnableWindow(hAlwaysBlackButton, TRUE);
	EnableWindow(hEdgeThicknessLabel, TRUE);
	EnableWindow(hEdgeThicknessSlider, TRUE);
	if (drawConditionalHighlights)
	{
		enableConditionals();
	}
	else
	{
		disableConditionals();
	}
	SendDlgItemMessage(hGeometryPage, IDC_EDGES_ONLY, BM_SETCHECK,
		edgesOnly ? 1 : 0, 0);
	SendDlgItemMessage(hGeometryPage, IDC_CONDITIONAL_HIGHLIGHTS, BM_SETCHECK,
		drawConditionalHighlights ? 1 : 0, 0);
	SendDlgItemMessage(hGeometryPage, IDC_QUALITY_LINES, BM_SETCHECK,
		usePolygonOffset, 0);
	SendDlgItemMessage(hGeometryPage, IDC_ALWAYS_BLACK, BM_SETCHECK,
		blackHighlights, 0);
}

void LDViewPreferences::disableEdges(void)
{
	EnableWindow(hConditionalHighlightsButton, FALSE);
	EnableWindow(hEdgesOnlyButton, FALSE);
	EnableWindow(hQualityLinesButton, FALSE);
	EnableWindow(hAlwaysBlackButton, FALSE);
	EnableWindow(hEdgeThicknessLabel, FALSE);
	EnableWindow(hEdgeThicknessSlider, FALSE);
	disableConditionals();
	SendDlgItemMessage(hGeometryPage, IDC_EDGES_ONLY, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hGeometryPage, IDC_CONDITIONAL_HIGHLIGHTS, BM_SETCHECK,
		0, 0);
	SendDlgItemMessage(hGeometryPage, IDC_QUALITY_LINES, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hGeometryPage, IDC_ALWAYS_BLACK, BM_SETCHECK, 0, 0);
}

void LDViewPreferences::setupEdgeLines(void)
{
	hConditionalHighlightsButton = GetDlgItem(hGeometryPage,
		IDC_CONDITIONAL_HIGHLIGHTS);
	hShowAllConditionalButton = GetDlgItem(hGeometryPage, IDC_ALL_CONDITIONAL);
	hShowConditionalControlsButton = GetDlgItem(hGeometryPage,
		IDC_CONDITIONAL_CONTROLS);
	hEdgesOnlyButton = GetDlgItem(hGeometryPage, IDC_EDGES_ONLY);
	hQualityLinesButton = GetDlgItem(hGeometryPage, IDC_QUALITY_LINES);
	hAlwaysBlackButton = GetDlgItem(hGeometryPage, IDC_ALWAYS_BLACK);
	hEdgeThicknessLabel = GetDlgItem(hGeometryPage, IDC_EDGE_THICKNESS_LABEL);
	hEdgeThicknessSlider = GetDlgItem(hGeometryPage, IDC_EDGE_THICKNESS);
	SendDlgItemMessage(hGeometryPage, IDC_HIGHLIGHTS, BM_SETCHECK,
		showsHighlightLines, 0);
	setupDialogSlider(hGeometryPage, IDC_EDGE_THICKNESS, 1, 5, 1,
		edgeThickness);
	if (showsHighlightLines)
	{
		enableEdges();
	}
	else
	{
		disableEdges();
	}
}

void LDViewPreferences::setupGeometryPage(void)
{
	hGeometryPage = hwndArray->pointerAtIndex(geometryPageNumber);
	setupModelGeometry();
	setupWireframe();
	setupEdgeLines();
}

void LDViewPreferences::setupOpacitySlider(void)
{
	if (cutawayMode == LDVCutawayWireframe && !LDVExtensionsSetup::haveAlpha())
	{
		cutawayAlpha = 100;
		EnableWindow(hCutawayOpacitySlider, FALSE);
		EnableWindow(hCutawayOpacityLabel, FALSE);
		SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY_OPACITY, TBM_SETPOS, 1,
			100);
	}
	else
	{
		EnableWindow(hCutawayOpacitySlider, TRUE);
		EnableWindow(hCutawayOpacityLabel, TRUE);
	}
}

void LDViewPreferences::enableCutaway(void)
{
	int activeCutaway = 0;

	SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY, BM_SETCHECK, 1, 0);
	EnableWindow(hCutawayColorButton, TRUE);
	if (LDVExtensionsSetup::haveStencil())
	{
		EnableWindow(hCutawayMonochromButton, TRUE);
	}
	else
	{
		if (cutawayMode == LDVCutawayStencil)
		{
			cutawayMode = LDVCutawayWireframe;
		}
		EnableWindow(hCutawayMonochromButton, FALSE);
	}
	setupOpacitySlider();
	EnableWindow(hCutawayThicknessSlider, TRUE);
	EnableWindow(hCutawayThicknessLabel, TRUE);
	SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY_COLOR, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY_MONOCHROME, BM_SETCHECK, 0, 0);
	switch (cutawayMode)
	{
	case LDVCutawayWireframe:
		activeCutaway = IDC_CUTAWAY_COLOR;
		break;
	case LDVCutawayStencil:
		activeCutaway = IDC_CUTAWAY_MONOCHROME;
		break;
	}
	SendDlgItemMessage(hEffectsPage, activeCutaway, BM_SETCHECK, 1, 0);
}

void LDViewPreferences::disableCutaway(void)
{
	SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY, BM_SETCHECK, 0, 0);
	EnableWindow(hCutawayColorButton, FALSE);
	EnableWindow(hCutawayMonochromButton, FALSE);
	EnableWindow(hCutawayOpacityLabel, FALSE);
	EnableWindow(hCutawayOpacitySlider, FALSE);
	EnableWindow(hCutawayThicknessLabel, FALSE);
	EnableWindow(hCutawayThicknessSlider, FALSE);
	SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY_COLOR, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hEffectsPage, IDC_CUTAWAY_MONOCHROME, BM_SETCHECK, 0, 0);
}

void LDViewPreferences::setupCutaway(void)
{
	hCutawayColorButton = GetDlgItem(hEffectsPage, IDC_CUTAWAY_COLOR);
	hCutawayMonochromButton = GetDlgItem(hEffectsPage, IDC_CUTAWAY_MONOCHROME);
	hCutawayOpacityLabel = GetDlgItem(hEffectsPage, IDC_CUTAWAY_OPACITY_LABEL);
	hCutawayOpacitySlider = GetDlgItem(hEffectsPage, IDC_CUTAWAY_OPACITY);
	hCutawayThicknessLabel = GetDlgItem(hEffectsPage,
		IDC_CUTAWAY_THICKNESS_LABEL);
	hCutawayThicknessSlider = GetDlgItem(hEffectsPage, IDC_CUTAWAY_THICKNESS);
	setupDialogSlider(hEffectsPage, IDC_CUTAWAY_OPACITY, 1, 100, 10,
		cutawayAlpha);
	setupDialogSlider(hEffectsPage, IDC_CUTAWAY_THICKNESS, 1, 5, 1,
		cutawayThickness);
	if (cutawayMode == LDVCutawayNormal)
	{
		disableCutaway();
	}
	else
	{
		enableCutaway();
	}
}

void LDViewPreferences::enableStereo(void)
{
	int activeStereo = 0;

	SendDlgItemMessage(hEffectsPage, IDC_STEREO, BM_SETCHECK, 1, 0);
	EnableWindow(hHardwareStereoButton, TRUE);
	EnableWindow(hCrossEyedStereoButton, TRUE);
	EnableWindow(hParallelStereoButton, TRUE);
	EnableWindow(hStereoSpacingSlider, TRUE);
	EnableWindow(hStereoSpacingLabel, TRUE);
	SendDlgItemMessage(hEffectsPage, IDC_HARDWARE_STEREO, BM_SETCHECK, 0,
		0);
	SendDlgItemMessage(hEffectsPage, IDC_CROSS_EYED_STEREO, BM_SETCHECK, 0,
		0);
	SendDlgItemMessage(hEffectsPage, IDC_PARALLEL_STEREO, BM_SETCHECK, 0,
		0);
	switch (stereoMode)
	{
	case LDVStereoHardware:
		activeStereo = IDC_HARDWARE_STEREO;
		break;
	case LDVStereoCrossEyed:
		activeStereo = IDC_CROSS_EYED_STEREO;
		break;
	case LDVStereoParallel:
		activeStereo = IDC_PARALLEL_STEREO;
		break;
	}
	SendDlgItemMessage(hEffectsPage, activeStereo, BM_SETCHECK, 1, 0);
}

void LDViewPreferences::disableStereo(void)
{
	SendDlgItemMessage(hEffectsPage, IDC_STEREO, BM_SETCHECK, 0, 0);
	EnableWindow(hHardwareStereoButton, FALSE);
	EnableWindow(hCrossEyedStereoButton, FALSE);
	EnableWindow(hParallelStereoButton, FALSE);
	EnableWindow(hStereoSpacingSlider, FALSE);
	EnableWindow(hStereoSpacingLabel, FALSE);
	SendDlgItemMessage(hEffectsPage, IDC_HARDWARE_STEREO, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hEffectsPage, IDC_CROSS_EYED_STEREO, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hEffectsPage, IDC_PARALLEL_STEREO, BM_SETCHECK, 0, 0);
}

void LDViewPreferences::setupStereo(void)
{
	hHardwareStereoButton = GetDlgItem(hEffectsPage, IDC_HARDWARE_STEREO);
	hCrossEyedStereoButton = GetDlgItem(hEffectsPage, IDC_CROSS_EYED_STEREO);
	hParallelStereoButton = GetDlgItem(hEffectsPage, IDC_PARALLEL_STEREO);
	hStereoSpacingSlider = GetDlgItem(hEffectsPage, IDC_STEREO_SPACING);
	hStereoSpacingLabel = GetDlgItem(hEffectsPage, IDC_STEREO_SPACING_LABEL);
	setupDialogSlider(hEffectsPage, IDC_STEREO_SPACING, 0, 100, 10,
		stereoEyeSpacing);
	if (stereoMode == LDVStereoNone)
	{
		disableStereo();
	}
	else
	{
		enableStereo();
	}
}

void LDViewPreferences::enableLighting(void)
{
	EnableWindow(hLightQualityButton, TRUE);
	EnableWindow(hLightSubduedButton, TRUE);
	EnableWindow(hLightSpecularButton, TRUE);
	EnableWindow(hLightAlternateButton, TRUE);
	SendDlgItemMessage(hEffectsPage, IDC_LIGHTING, BM_SETCHECK, 1, 0);
	SendDlgItemMessage(hEffectsPage, IDC_LIGHTING_QUALITY, BM_SETCHECK,
		qualityLighting, 0);
	SendDlgItemMessage(hEffectsPage, IDC_LIGHTING_SUBDUED, BM_SETCHECK,
		subduedLighting, 0);
	SendDlgItemMessage(hEffectsPage, IDC_SPECULAR, BM_SETCHECK,
		usesSpecular, 0);
	SendDlgItemMessage(hEffectsPage, IDC_ALTERNATE_LIGHTING, BM_SETCHECK,
		oneLight, 0);
}

void LDViewPreferences::disableLighting(void)
{
	EnableWindow(hLightQualityButton, FALSE);
	EnableWindow(hLightSubduedButton, FALSE);
	EnableWindow(hLightSpecularButton, FALSE);
	EnableWindow(hLightAlternateButton, FALSE);
	SendDlgItemMessage(hEffectsPage, IDC_LIGHTING, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hEffectsPage, IDC_LIGHTING_QUALITY, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hEffectsPage, IDC_LIGHTING_SUBDUED, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hEffectsPage, IDC_SPECULAR, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hEffectsPage, IDC_ALTERNATE_LIGHTING, BM_SETCHECK, 0, 0);
}

void LDViewPreferences::setupLighting(void)
{
	hLightQualityButton = GetDlgItem(hEffectsPage, IDC_LIGHTING_QUALITY);
	hLightSubduedButton = GetDlgItem(hEffectsPage, IDC_LIGHTING_SUBDUED);
	hLightSpecularButton = GetDlgItem(hEffectsPage, IDC_SPECULAR);
	hLightAlternateButton = GetDlgItem(hEffectsPage, IDC_ALTERNATE_LIGHTING);
	if (useLighting)
	{
		enableLighting();
	}
	else
	{
		disableLighting();
	}
}

void LDViewPreferences::setupEffectsPage(void)
{
	hEffectsPage = hwndArray->pointerAtIndex(effectsPageNumber);
	setupLighting();
	setupStereo();
	setupCutaway();
	SendDlgItemMessage(hEffectsPage, IDC_STIPPLE, BM_SETCHECK,
		useStipple, 0);
	SendDlgItemMessage(hEffectsPage, IDC_SORT, BM_SETCHECK,
		sortTransparent, 0);
	SendDlgItemMessage(hEffectsPage, IDC_FLAT_SHADING, BM_SETCHECK,
		usesFlatShading, 0);
	SendDlgItemMessage(hEffectsPage, IDC_SMOOTH_CURVES, BM_SETCHECK,
		performSmoothing, 0);
}

void LDViewPreferences::setupSubstitution(void)
{
	hTextureStudsButton = GetDlgItem(hPrimitivesPage, IDC_TEXTURE_STUDS);
	hTextureNearestButton = GetDlgItem(hPrimitivesPage, IDC_TEXTURE_NEAREST);
	hTextureBilinearButton = GetDlgItem(hPrimitivesPage, IDC_TEXTURE_BILINEAR);
	hTextureTrilinearButton = GetDlgItem(hPrimitivesPage,
		IDC_TEXTURE_TRILINEAR);
	hCurveQualityLabel = GetDlgItem(hPrimitivesPage, IDC_CURVE_QUALITY_LABEL);
	hCurveQualitySlider = GetDlgItem(hPrimitivesPage, IDC_CURVE_QUALITY);
	setupDialogSlider(hPrimitivesPage, IDC_CURVE_QUALITY, 1, 12, 1,
		curveQuality);
	if (allowPrimitiveSubstitution)
	{
		enablePrimitives();
	}
	else
	{
		disablePrimitives();
	}
}

void LDViewPreferences::setupPrimitivesPage(void)
{
	hPrimitivesPage = hwndArray->pointerAtIndex(primitivesPageNumber);
	setupSubstitution();
	SendDlgItemMessage(hPrimitivesPage, IDC_STUD_QUALITY, BM_SETCHECK,
		!qualityStuds, 0);
	SendDlgItemMessage(hPrimitivesPage, IDC_HI_RES, BM_SETCHECK,
		hiResPrimitives, 0);
}

void LDViewPreferences::selectPrefSet(const char *prefSet, bool force)
{
	if (prefSet)
	{
		SendMessage(hPrefSetsList, LB_SELECTSTRING, 0, (LPARAM)prefSet);
		doPrefSetSelected(force);
	}
	else
	{
		char *savedSession =
			TCUserDefaults::getSavedSessionNameFromKey(PREFERENCE_SET_KEY);

		if (savedSession && savedSession[0])
		{
			selectPrefSet(savedSession, force);
		}
		else
		{
			selectPrefSet(DEFAULT_PREF_SET, force);
		}
		delete savedSession;
	}
}

void LDViewPreferences::setupPrefSetsList(void)
{
	TCStringArray *sessionNames = TCUserDefaults::getAllSessionNames();
	int i;
	int count = sessionNames->getCount();

	hPrefSetsList = GetDlgItem(hPrefSetsPage, IDC_PREFSETS_LIST);
	SendMessage(hPrefSetsList, LB_ADDSTRING, 0, (LPARAM)DEFAULT_PREF_SET);
	for (i = 0; i < count; i++)
	{
		SendMessage(hPrefSetsList, LB_ADDSTRING, 0,
			(LPARAM)sessionNames->stringAtIndex(i));
	}
	selectPrefSet();
	sessionNames->release();
}

void LDViewPreferences::setupPrefSetsPage(void)
{
	hPrefSetsPage = hwndArray->pointerAtIndex(prefSetsPageNumber);
	hDeletePrefSetButton = GetDlgItem(hPrefSetsPage, IDC_DELETE_PREF_SET);
	hNewPrefSetButton = GetDlgItem(hPrefSetsPage, IDC_NEW_PREF_SET);
	hPrefSetHotKeyButton = GetDlgItem(hPrefSetsPage, IDC_PREF_SET_HOT_KEY);
	setupPrefSetsList();
}

void LDViewPreferences::setupAntialiasing(void)
{
	TCIntArray *fsaaModes = LDVExtensionsSetup::getFSAAModes();
	char modeString[1024];

	// Remove all items from FSAA combo box list.
	SendDlgItemMessage(hGeneralPage, IDC_FSAA_COMBO, CB_RESETCONTENT, 0, 0);
	// Add "None" to FSAA combo box list as only item.
	SendDlgItemMessage(hGeneralPage, IDC_FSAA_COMBO, CB_ADDSTRING, 0,
		(LPARAM)"None");
	// Select "None", just in case something else doesn't get selected later.
	SendDlgItemMessage(hGeneralPage, IDC_FSAA_COMBO, CB_SETCURSEL, 0, 0);
	// The following array should always exist, even if it is empty, but check
	// just to be sure.
	if (fsaaModes)
	{
		int i;
		int count = fsaaModes->getCount();

		// Note that fsaaModes contains a sorted array of unique FSAA factors.
		for (i = 0; i < count; i++)
		{
			int value = (*fsaaModes)[i];

			sprintf(modeString, "%dx", value);
			SendDlgItemMessage(hGeneralPage, IDC_FSAA_COMBO, CB_ADDSTRING, 0,
				(LPARAM)modeString);
			// nVidia hardare supports Quincunx and 9-box pattern, so add an
			// "Enhanced" item to the list if the extension is supported and
			// the current factor is 2 or 4.
			if ((value == 2 || value == 4) &&
				LDVExtensionsSetup::haveNvMultisampleFilterHintExtension())
			{
				sprintf(modeString, "%dx Enhanced", value);
				SendDlgItemMessage(hGeneralPage, IDC_FSAA_COMBO, CB_ADDSTRING,
					0, (LPARAM)modeString);
			}
		}
	}
	if (fsaaMode)
	{
		sprintf(modeString, "%dx", getFSAAFactor());
		if (getUseNvMultisampleFilter())
		{
			strcat(modeString, " Enhanced");
		}
		if (SendDlgItemMessage(hGeneralPage, IDC_FSAA_COMBO, CB_SELECTSTRING, 0,
			(LPARAM)modeString) == CB_ERR)
		{
			// We didn't find the selected mode, so reset to none.
			fsaaMode = 0;
		}
	}
	SendDlgItemMessage(hGeneralPage, IDC_LINE_AA, BM_SETCHECK, lineSmoothing,
		0);
}

bool LDViewPreferences::doApply(void)
{
	applyChanges();
	checkAbandon = true;
	return true;
}

void LDViewPreferences::clear(void)
{
	if (hBackgroundColorBitmap)
	{
		DeleteObject(hBackgroundColorBitmap);
		hBackgroundColorBitmap = NULL;
	}
	if (hDefaultColorBitmap)
	{
		DeleteObject(hDefaultColorBitmap);
		hDefaultColorBitmap = NULL;
	}
	if (hButtonColorDC)
	{
		DeleteDC(hButtonColorDC);
		hButtonColorDC = NULL;
	}
	hGeneralPage = NULL;
	hGeometryPage = NULL;
	hEffectsPage = NULL;
	hPrimitivesPage = NULL;
	hPrefSetsPage = NULL;
	CUIPropertySheet::clear();
}

BOOL LDViewPreferences::doNewPrefSetInit(HWND /*hDlg*/, HWND hNewPrefSetField)
{
	SendMessage(hNewPrefSetField, EM_LIMITTEXT, 32, 0);
	return TRUE;
}

BOOL LDViewPreferences::doHotKeyInit(HWND hDlg, HWND /*hHotKeyCombo*/)
{
	int i;
	char *prefSetName = getSelectedPrefSet();

	if (prefSetName)
	{
		SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)prefSetName);
		delete prefSetName;
	}
	else
	{
		SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)"???");
	}
	SendDlgItemMessage(hDlg, IDC_HOTKEY_COMBO, CB_ADDSTRING, 0,
		(LPARAM)"<None>");
	for (i = 1; i <= 10; i++)
	{
		char numString[5];

		sprintf(numString, "%d", i % 10);
		SendDlgItemMessage(hDlg, IDC_HOTKEY_COMBO, CB_ADDSTRING, 0,
			(LPARAM)numString);
	}
	SendDlgItemMessage(hDlg, IDC_HOTKEY_COMBO, CB_SETCURSEL, hotKeyIndex, 0);
	return TRUE;
}

BOOL LDViewPreferences::doDialogInit(HWND hDlg, HWND /*hFocusWindow*/,
									 LPARAM /*lParam*/)
{
	HWND hNewPrefSetField = NULL;
	HWND hHotKeyCombo = NULL;

	hNewPrefSetField = GetDlgItem(hDlg, IDC_NEW_PREF_SET_FIELD);
	if (hNewPrefSetField)
	{
		return doNewPrefSetInit(hDlg, hNewPrefSetField);
	}
	hHotKeyCombo = GetDlgItem(hDlg, IDC_HOTKEY_COMBO);
	if (hHotKeyCombo)
	{
		return doHotKeyInit(hDlg, hHotKeyCombo);
	}
	return TRUE;
}

bool LDViewPreferences::shouldSetActive(int index)
{
	if (applyEnabledArray->itemAtIndex(prefSetsPageNumber) &&
		index != prefSetsPageNumber)
	{
		if (!setActiveWarned)
		{
			setActiveWarned = true;
			MessageBox(hWindow,
				"You must apply your changes before you can leave this tab.",
				"Error", MB_OK | MB_ICONWARNING);
		}
		return false;
	}
	setActiveWarned = false;
	return true;
}

int LDViewPreferences::getFSAAFactor(void)
{
	int fsaaMode = TCUserDefaults::longForKey(FSAA_MODE_KEY);

	if (fsaaMode && LDVExtensionsSetup::haveMultisampleExtension())
	{
		if (fsaaMode <= 5)
		{
			return fsaaMode & 0x6; // Mask off bottom bit
		}
		else
		{
			return fsaaMode >> 3;
		}
	}
	else
	{
		return 0;
	}
}

bool LDViewPreferences::getUseNvMultisampleFilter(void)
{
	int fsaaMode = TCUserDefaults::longForKey(FSAA_MODE_KEY);

	if ((fsaaMode & 0x1) &&
		LDVExtensionsSetup::haveNvMultisampleFilterHintExtension())
	{
		return true;
	}
	else
	{
		return false;
	}
}

BOOL LDViewPreferences::doDrawColorButton(HWND hDlg, HWND hWnd, HTHEME hTheme,
										  LPDRAWITEMSTRUCT drawItemStruct)
{
	HDC hdc = drawItemStruct->hDC;
	bool bIsPressed = (drawItemStruct->itemState & ODS_SELECTED) != 0;
	bool bIsFocused = (drawItemStruct->itemState & ODS_FOCUS) != 0;
	bool bDrawFocusRect = (drawItemStruct->itemState & ODS_NOFOCUSRECT) == 0;
	RECT itemRect = drawItemStruct->rcItem;
	HBITMAP hBitmap;

	SetBkMode(hdc, TRANSPARENT);
	// Prepare draw... paint button background
	if (CUIThemes::isThemeLibLoaded() && hTheme)
	{
		DWORD state = (bIsPressed) ? PBS_PRESSED : PBS_NORMAL;

		if (state == PBS_NORMAL)
		{
			if (bIsFocused)
			{
				state = PBS_DEFAULTED;
			}
			if (hMouseOverButton == hWnd)
			{
				state = PBS_HOT;
			}
		}
		CUIThemes::drawThemeBackground(hTheme, hdc, BP_PUSHBUTTON, state,
			&itemRect, NULL);
	}
	else
	{
		if (bIsFocused)
			{
			HBRUSH br = CreateSolidBrush(RGB(0,0,0));  
			FrameRect(hdc, &itemRect, br);
			InflateRect(&itemRect, -1, -1);
			DeleteObject(br);
			} // if		

		COLORREF crColor = GetSysColor(COLOR_BTNFACE);

		HBRUSH	brBackground = CreateSolidBrush(crColor);

		FillRect(hdc, &itemRect, brBackground);

		DeleteObject(brBackground);

		// Draw pressed button
		if (bIsPressed)
		{
			HBRUSH brBtnShadow = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
			FrameRect(hdc, &itemRect, brBtnShadow);
			DeleteObject(brBtnShadow);
		}

		else // ...else draw non pressed button
		{
			UINT uState = DFCS_BUTTONPUSH |
                  ((hMouseOverButton == hWnd) ? DFCS_HOT : 0) |
                  ((bIsPressed) ? DFCS_PUSHED : 0);

			DrawFrameControl(hdc, &itemRect, DFC_BUTTON, uState);
		} // else
	}

	// Draw the focus rect
	if (bIsFocused && bDrawFocusRect)
	{
		RECT focusRect = itemRect;

		InflateRect(&focusRect, -3, -3);
		DrawFocusRect(hdc, &focusRect);
	}

	// Draw the bitmap
	hBitmap = (HBITMAP)SendDlgItemMessage(hDlg, drawItemStruct->CtlID,
		BM_GETIMAGE, (WPARAM)IMAGE_BITMAP, 0);
	if (hBitmap)
	{
		SIZE bitmapSize;

		if (GetBitmapDimensionEx(hBitmap, &bitmapSize))
		{
			HBITMAP hOldBitmap = (HBITMAP)SelectObject(hButtonColorDC,
				hBitmap);
			RECT clientRect;
			RECT contentRect;
			int width;
			int height;

			GetClientRect(hWnd, &clientRect);
			if (CUIThemes::isThemeLibLoaded() && hTheme)
			{
				CUIThemes::getThemeBackgroundContentRect(hTheme, NULL,
					BP_PUSHBUTTON, PBS_HOT, &clientRect, &contentRect);
			}
			else
			{
				contentRect = clientRect;
			}
			width = contentRect.right - contentRect.left;
			height = contentRect.bottom - contentRect.top;
			BitBlt(hdc, contentRect.left + (width - bitmapSize.cx) / 2,
				contentRect.top + (height - bitmapSize.cy) / 2, bitmapSize.cx,
				bitmapSize.cy, hButtonColorDC, 0, 0, SRCCOPY);
			SelectObject(hButtonColorDC, hOldBitmap);
		}
	}
	return TRUE;
}

BOOL LDViewPreferences::dialogProc(HWND hDlg, UINT message, WPARAM wParam,
						   LPARAM lParam)
{
	switch (message)
	{
	case WM_MOUSEMOVE:
		if (CUIThemes::isThemeLibLoaded())
		{
			if (hMouseOverButton)
			{
				InvalidateRect(hMouseOverButton, NULL, TRUE);
				hMouseOverButton = NULL;
			}
			break;
		}
	case WM_DRAWITEM:
		if (wParam == IDC_DEFAULT_COLOR || wParam == IDC_BACKGROUND_COLOR)
		{
			HWND hWnd = GetDlgItem(hDlg, wParam);
			HTHEME hTheme = hDefaultColorTheme;

			if (wParam == IDC_BACKGROUND_COLOR)
			{
				hTheme = hBackgroundColorTheme;
			}
			return doDrawColorButton(hDlg, hWnd, hTheme,
				(LPDRAWITEMSTRUCT)lParam);
		}
		break;
	}
	return CUIPropertySheet::dialogProc(hDlg, message, wParam, lParam);
}

float LDViewPreferences::getMinFov(void)
{
	return 0.1f;
}

float LDViewPreferences::getMaxFov(void)
{
	return 90.0f;
}

void LDViewPreferences::setupDefaultRotationMatrix(void)
{
	char *value = TCUserDefaults::stringForKey(DEFAULT_LAT_LONG_KEY);

	if (value)
	{
		float latitude;
		float longitude;

		if (sscanf(value, "%f,%f", &latitude, &longitude) == 2)
		{
			float leftMatrix[16];
			float rightMatrix[16];
			float resultMatrix[16];
			float cosTheta;
			float sinTheta;

			TCVector::initIdentityMatrix(leftMatrix);
			TCVector::initIdentityMatrix(rightMatrix);
			latitude = (float)deg2rad(latitude);
			longitude = (float)deg2rad(longitude);

			// First, apply latitude by rotating around X.
			cosTheta = (float)cos(latitude);
			sinTheta = (float)sin(latitude);
			rightMatrix[5] = cosTheta;
			rightMatrix[6] = sinTheta;
			rightMatrix[9] = -sinTheta;
			rightMatrix[10] = cosTheta;
			TCVector::multMatrix(leftMatrix, rightMatrix, resultMatrix);

			memcpy(leftMatrix, resultMatrix, sizeof(leftMatrix));
			TCVector::initIdentityMatrix(rightMatrix);

			// Next, apply longitude by rotating around Y.
			cosTheta = (float)cos(longitude);
			sinTheta = (float)sin(longitude);
			rightMatrix[0] = cosTheta;
			rightMatrix[2] = -sinTheta;
			rightMatrix[8] = sinTheta;
			rightMatrix[10] = cosTheta;
			TCVector::multMatrix(leftMatrix, rightMatrix, resultMatrix);

			modelViewer->setDefaultRotationMatrix(resultMatrix);
		}
		delete value;
	}
	else
	{
		value = TCUserDefaults::stringForKey(DEFAULT_MATRIX_KEY);
		if (value)
		{
			float matrix[16];

/*
			if (sscanf(value, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
				&matrix[0], &matrix[4], &matrix[8], &matrix[12],
				&matrix[1], &matrix[5], &matrix[9], &matrix[13],
				&matrix[2], &matrix[6], &matrix[10], &matrix[14],
				&matrix[3], &matrix[7], &matrix[11], &matrix[15]) == 16)
			{
				modelViewer->setDefaultRotationMatrix(matrix);
			}
			else
			{
*/
			memset(matrix, 0, sizeof(matrix));
			matrix[15] = 1.0f;
			if (sscanf(value, "%f,%f,%f,%f,%f,%f,%f,%f,%f",
				&matrix[0], &matrix[4], &matrix[8],
				&matrix[1], &matrix[5], &matrix[9],
				&matrix[2], &matrix[6], &matrix[10]) == 9)
			{
				modelViewer->setDefaultRotationMatrix(matrix);
			}
//			}
			delete value;
		}
	}
}
